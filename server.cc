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
#include "shell_single.cc"

#define SERV_TCP_PORT 9487

#define PIPE 30 * 30 * 1024
#define NAME 30 * 1024
#define MESSAGE 30 * 1024 * 10
#define SHM_SIZE PIPE + NAME + MESSAGE
#define PIPE_OFFSET 0
#define NAME_OFFSET PIPE
#define MESSAGE_OFFSET PIPE + NAME
// pipe | name | message
using namespace std;

void waitfor(int sig)
{
	int wstat;
	int pid = wait(&wstat);
}

class indiv;

int concurrent_connection_oriented(int sock, struct sockaddr *cli_addr);
int single_process_concurrent(int sock, struct sockaddr *cli_addr);

int main()
{
	signal(SIGCHLD, waitfor);
//	signal(SIGINT, abortion);
	int serv_tcp_port = SERV_TCP_PORT;
	struct sockaddr_in cli_addr, serv_addr;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		cout << "Error: Cannot open socket" << endl;
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
//		cout << "Error: Can't bind " << serv_tcp_port << endl;
		serv_tcp_port++;
	}
	cout << "Bind port: " << serv_tcp_port << endl;
	listen(sock, 5);

	single_process_concurrent(sock, (struct sockaddr *) &cli_addr);
	//concurrent_connection_oriented(sock, (struct sockaddr *) &cli_addr);
}
/*
int concurrent_connection_oriented(int sock, struct sockaddr *cli_addr)
{
	int shmid = shmget(0, SHM_SIZE, IPC_CREAT | 0666);
	char *ptr = (char *)shmat(shmid, NULL, 0);
	memset(ptr, 0, SHM_SIZE);
	while(1)
	{
		unsigned int clilen = sizeof(cli_addr);
		int newsock = accept(sock, cli_addr, &clilen);
		int child;
		if((child = fork()) < 0)
		{
			cout << "Error: Accept error" << endl;
			return 0;
		}
		else if(child == 0) 
		{
			close(sock);
			indiv user;
			//void *ptr = NULL;
			greeting(newsock);
			write(newsock, "% ", 2);
			while(process_handler(newsock, user, ptr, vect))write(newsock, "% ", 2);
			exit(0);
		}
		close(newsock);
	}
}
*/


int single_process_concurrent(int sock, struct sockaddr *cli_addr)
{
	//int shmid = shmget(0, SHM_SIZE, IPC_CREAT | 0666);
	//char *ptr = (char *)shmat(shmid, NULL, 0);
	char ptr[SHM_SIZE] = {};
	memset(ptr, 0, SHM_SIZE);
	fd_set rfds;
	fd_set afds;
	int nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(sock, &afds);
	vector<indiv> vect(30, indiv());
	vector<string> hstr(30, string());
	vector<string> pstr(30, string());
	vector<int> defined(30, -1);//-1 not def
	while(1)
	{
		memcpy(&rfds, &afds, sizeof(rfds));
		while(select(nfds, &rfds, (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL) < 0)
		{
			if(errno == EINTR);
			else
			{
				cout << "Error: Select error: " << strerror(errno) << endl;
				return 0;
			}
		}
		if(FD_ISSET(sock, &rfds))
		{
			unsigned int clilen = sizeof(cli_addr);
			int newsock = accept(sock, cli_addr, &clilen);
			if(newsock < 0)
			{
				cout << "Error: Accept error" << endl;
				return 0;
			}
			FD_SET(newsock, &afds);
			int flag = false;
			for(int i = 0; i < 30; i++)
			{
				if(defined[i] == -1)
				{
					flag = true;
					vect[i] = indiv();
					defined[i] = newsock;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
					getnameinfo(cli_addr, clilen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
//					hstr[i] = hbuf;
//					pstr[i] = sbuf;
					hstr[i] = "CGILAB";
					pstr[i] = "511";
					greeting(newsock);
					string s = string() + "*** User '(no name)' entered from " + hstr[i] + "/" + pstr[i] + ". ***\n";
					cout << "New user: Host(" << hstr[i] << "), Port(" << pstr[i] << ")" << endl;
					for(int j = 0; j < 30; j++)
					{
						if(defined[j] != -1)write(defined[j], s.c_str(), s.size());
					}
					string name = "(no name)";
					for(int j = 0; j < name.size(); j++)
					{
						ptr[NAME_OFFSET + 30 * i + j] = name[j];
					}
					write(newsock, "% ", 2);
					break;
				}
			}
			if(!flag)
			{
				write(newsock, "connection refused FULL HOUSE", 20);
				(void) close(newsock);
				FD_CLR(newsock, &afds);
			}
		}
		for(int i = 0; i < nfds; i++)
		{
			if(i == sock)continue;
			if(!FD_ISSET(i, &rfds))continue;
			int id = -1;
			for(int j = 0; j < 30; j++)
			{
				if(defined[j] == i)
				{
					id = j;
					break;
				}
			}
			if(id == -1)
			{
				cout << "Error: find" << endl;
				break;
			}
//			void *ptr = NULL;
			int ret = process_handler(i, vect[id], ptr, defined, hstr, pstr);
			if(!ret)
			{
				string s = string() + "*** User '" + (ptr + NAME_OFFSET + 30 * id) + "' left. ***\n";
				for(int i = 0; i < 30; i++)
				{
					if(defined[id] != -1)write(defined[i], s.c_str(), s.size());
				}
				defined[id] = -1;
				vect[id] = indiv();
				(void) close(i);
				FD_CLR(i, &afds);
				memset(ptr + NAME_OFFSET + 30 * id, 0, 30);
				for(int i = 1; i < 31; i++)
				{
					char b[10] = {};
					sprintf(b, "%d-%d", i, id + 1);
					string str;
					str = "/tmp/wush60309-" + string(b);
					struct stat exist;
					if(stat(str.c_str(), &exist) == 0)
					{
						remove(str.c_str());
						cout << "removed" << str << endl;
					}
				}
				memset(ptr + PIPE_OFFSET + 1024 * 30 * id, 0, 1024 * 30);
			}
			else write(i, "% ", 2);
		}
	}
}

