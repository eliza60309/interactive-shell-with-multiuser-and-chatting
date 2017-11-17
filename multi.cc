#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fstream>
#include <iostream>
//#include <streambuf>
#include "multi_shell.cc"

#define SERV_TCP_PORT 9487

#define PIPE 30
#define NAME 1024
#define MSGSIGN 30
#define PROCID 20
#define FDID 10

#define PIPE_SIZE 30 * PIPE
#define NAME_SIZE 30 * NAME
#define MSG_SIZE 2048
#define MSGSIGN_SIZE 1 * MSGSIGN
#define PROCID_SIZE 30 * PROCID
#define FDID_SIZE 30 * FDID
#define SHM_SIZE PIPE_SIZE + NAME_SIZE + MSG_SIZE + MSGSIGN_SIZE + PROCID_SIZE + FDID_SIZE
#define PIPE_OFFSET 0
#define NAME_OFFSET PIPE_OFFSET + PIPE_SIZE
#define MSG_OFFSET NAME_OFFSET + NAME_SIZE
#define MSGSIGN_OFFSET MSG_OFFSET + MSG_SIZE
#define PROCID_OFFSET MSGSIGN_OFFSET + MSGSIGN_SIZE
#define FDID_OFFSET PROCID_OFFSET + PROCID_SIZE

using namespace std;



class indiv;

int concurrent_connection_oriented(int sock, struct sockaddr *cli_addr);
int readint(char *c);
void waitfor(int sig);
void rcvmsg(int sig);
int broadcast(const char *c);
int wisper(int id, const char *c);
void waitterm(int sig);
int rstmmry(char *c, int size);
int writeint(char *c, int i);
void abortion(int sig);
int writemsg(char *c, const char *str);
int writemmry(char *c, const char *str);
string readmmry(char *c);
int cleanup();

char *ptr;
int mysock;
int myid;
int shmid;

int main()
{
	int serv_tcp_port = SERV_TCP_PORT;
	struct sockaddr_in cli_addr, serv_addr;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	mysock = sock;
	if(sock < 0)
	{
		cout << "[ERR] Cannot open socket" << endl;
		return 0;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	while(1)
	{
		serv_addr.sin_port = htons(serv_tcp_port);
		int bin = bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
		if(bin >= 0)break;
		serv_tcp_port++;
	}
	cout << "[LOG] Bind port: " << serv_tcp_port << endl;
	listen(sock, 5);

	concurrent_connection_oriented(sock, (struct sockaddr *) &cli_addr);
}

int concurrent_connection_oriented(int sock, struct sockaddr *cli_addr)
{
	shmid = shmget(0, SHM_SIZE, IPC_CREAT | 0666);
	cout << "[LOG] SHMID = " << shmid << endl;
	ptr = (char *)shmat(shmid, NULL, 0);
//	int ret = fork();
//	if(ret == 0)cleanup();
	signal(SIGCHLD, waitfor);
	signal(SIGTERM, abortion);
	signal(SIGINT, abortion);
	memset(ptr, 0, SHM_SIZE);
	while(1)
	{
		unsigned int clilen = sizeof(cli_addr);
		int newsock = accept(sock, cli_addr, &clilen);
		int child;
		if((child = fork()) < 0)
		{
			cout << "[ERR] Accept error" << endl;
			return 0;
		}
		else if(child == 0) 
		{
			signal(SIGUSR1, rcvmsg);
			signal(SIGCHLD, waitterm);
			signal(SIGINT, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			close(sock);
			for(int i = 0; i < 30; i++)
			{
				//if((ptr + MSGSIGN_OFFSET)[i] == 0 && (ptr + PROCID_OFFSET + i * PROCID)[0] != 0)
				if((ptr + MSGSIGN_OFFSET)[i] == 0)
				{
				//	cout << "[LOG] User " << i + 1 << " disconnected." << endl;
					(ptr + MSGSIGN_OFFSET)[i] = 1;
					cout << "[LOG] User " << i + 1 << " (" << getpid() << ") connected." << endl;
					rstmmry(ptr + FDID_OFFSET + i * FDID, FDID);
					rstmmry(ptr + PROCID_OFFSET + i * PROCID, PROCID);
					rstmmry(ptr + NAME_OFFSET + i * NAME, NAME);
					rstmmry(ptr + PIPE_OFFSET + i * PIPE, PIPE);
					for(int j = 1; j < 31; j++)
					{
						char b[10] = {};
						sprintf(b, "%d-%d", j, i + 1);
						string str;
						str = "/tmp/_wush60309-" + string(b);
						struct stat exist;
						if(stat(str.c_str(), &exist) == 0)
						{
							remove(str.c_str());
							cout << "removed" << str << endl;
						}
					}
			/*	}
				if((ptr + PROCID_OFFSET + i * PROCID)[0] == 0)
				{
					rstmmry(ptr + FDID_OFFSET + i * FDID, FDID);
					rstmmry(ptr + PROCID_OFFSET + i * PROCID, PROCID);
					rstmmry(ptr + NAME_OFFSET + i * NAME, NAME);
					rstmmry(ptr + PIPE_OFFSET + i * PIPE, PIPE);
					for(int j = 1; j < 31; j++)
					{
						char b[10] = {};
						sprintf(b, "%d-%d", j, i + 1);
						string str;
						str = "/tmp/_wush60309-" + string(b);
						struct stat exist;
						if(stat(str.c_str(), &exist) == 0)
						{
							remove(str.c_str());
							cout << "removed" << str << endl;
						}
					}*/
					myid = i;
					mysock = newsock;
					writeint(ptr + PROCID_OFFSET + i * PROCID, getpid());
					(ptr + MSGSIGN_OFFSET)[i] = 1;
					greeting(newsock);
					broadcast("*** User '(no name)' entered from CGILAB/511. ***\n");
					writemmry(ptr + NAME_OFFSET + i * NAME, "(no name)");
					break;
				}
			}
			indiv user;
			write(newsock, "% ", 2);
			while(1)
			{
				int ret = process_handler(newsock, user, ptr, myid);
				if(ret == 0)
				{
					string str = string() + "*** User '" + (ptr + NAME_OFFSET + NAME * myid) + "' left. ***\n";
					cout << "[LOG] User " << myid + 1 << " disconnected." << endl;
					(ptr + MSGSIGN_OFFSET)[myid] = 0;
					broadcast(str.c_str());
					close(newsock);
					exit(0);
				}
				write(newsock, "% ", 2);
			}
		}
		close(newsock);
	}
}

void abortion(int sig)
{
	shmdt(ptr);
	int k = shmctl(shmid, IPC_RMID, NULL);
	if(k == -1)cout << "[WRN] SHM delete failed." << endl; 
	else cout << "[LOG] SHM deleted." << endl;
	exit(0);
}

int readint(char *c)
{
	int i = -1;
	while(1)
	{
		if(c[0] > '9' || c[0] < '0')return i;
		if(i < 0)i = 0;
		i *= 10;
		i += c[0] - '0';
		c++;
	}
}

int writeint(char *c, int i)
{
	char b[20] = {};
	sprintf(b, "%d", i);
	for(int i = 0; i < 20; i++)
	{
		if(b[i] == 0)break;
		else(c[i] = b[i]);
	}
	return 0;
}

int rstmmry(char *c, int size)
{
	for(int i = 0; i < size; i++)c[i] = 0;
}

int writemsg(char *c, const char *str)
{
	int i = 0;
	for(int k = 0; k < MSG_SIZE; k++)c[k] = 0;
	while(str[0] != 0)
	{
		c[0] = str[0];
		c++;
		str++;
		i++;
		if(i >= MSG_SIZE - 10)
		{
			cout << "[WRN] MSG size is dangerous" << endl;
			break;
		}
	}
	return 0;
}

string readmmry(char *c)
{
	char buf[10000] = {};
	int i = 0;
	while(c[i] != 0)
	{
		buf[i] = c[i];
		i++;
	}
	return string(buf);
}

int writemmry(char *c, const char *str)
{
	for(int i = 0;; i++)
	{
		if(str[i] == 0)return 0;
		c[i] = str[i];
	}
}

void waitterm(int sig)
{
	int wstat;
	int pid = wait(&wstat);
}

void waitfor(int sig)
{
	int wstat;
	int pid = wait(&wstat);
}

int cleanup()
{
	while(1)
	{
		for(int i = 0; i < 30; i++)
		{
			if((ptr + MSGSIGN_OFFSET)[i] == 0 && (ptr + PROCID_OFFSET + i * PROCID)[0] != 0)
			{
				cout << "[LOG] User " << i + 1 << " disconnected." << endl;
				rstmmry(ptr + FDID_OFFSET + i * FDID, FDID);
				rstmmry(ptr + PROCID_OFFSET + i * PROCID, PROCID);
				rstmmry(ptr + NAME_OFFSET + i * NAME, NAME);
				rstmmry(ptr + PIPE_OFFSET + i * PIPE, PIPE);
				for(int j = 1; j < 31; j++)
				{
					char b[10] = {};
					sprintf(b, "%d-%d", j, i + 1);
					string str;
					str = "/tmp/_wush60309-" + string(b);
					struct stat exist;
					if(stat(str.c_str(), &exist) == 0)
					{
						remove(str.c_str());
						cout << "removed" << str << endl;
					}
				}
			}
		}
	}
}

void rcvmsg(int sig)
{
	char c[MSG_SIZE] = {};
	for(int i = 0; i < MSG_SIZE - 1; i++)
	{
		if((ptr + MSG_OFFSET)[i] == 0)break;
		else c[i] = (ptr + MSG_OFFSET)[i];
	}
	string str = c;
	write(mysock, str.c_str(), str.size());
}

int broadcast(const char *c)
{
	/*for(int i = 0; i < MSG_SIZE - 1; i++)
	{
		if(c[i] == 0)break;
		else (ptr + MSG_OFFSET)[i] = c[i];
	}*/
	writemsg(ptr + MSG_OFFSET, c);
	for(int i = 0; i < 30; i++)
	{
		int pid = readint(ptr + PROCID_OFFSET + i * PROCID);
		if(pid == -1)continue;
		kill(pid, SIGUSR1);
	}
	return 1;
}

int wisper(int id, const char *c)
{
	/*for(int i = 0; i < MSG_SIZE - 1; i++)
	{
		if(c[i] == 0)break;
		else (ptr + MSG_OFFSET)[i] = c[i];
	}*/
	writemsg(ptr + MSG_OFFSET, c);
	int pid = readint(ptr + PROCID_OFFSET + id * PROCID);
	if(pid == -1)
	{
		cout << "[WRN]" << "Wisper to unknown userid " << id << endl;
		return 0;
	}
	kill(pid, SIGUSR1);
	return 1;
}
