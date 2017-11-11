#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fstream>
#include <iostream>
//#include <streambuf>
#include "shell.cc"

#define SERV_TCP_PORT 9487

using namespace std;

void waitfor(int sig)
{
	int wstat;
	int pid = wait(&wstat);
}

class indiv;
//void abortion(int sig)
//{
//	abort();
//}
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

	concurrent_connection_oriented(sock, (struct sockaddr *) &cli_addr);
/*	while(1)
	{
		unsigned int clilen = sizeof(cli_addr);
		int newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
		int child;
		if((child = fork()) < 0)
		{
			cout << "Error: Accept error" << endl;
			return 0;
		}
		else if(child == 0) 
		{
			close(sock);
			process_handler(newsock);
			exit(0);
		}

		close(newsock);
	}*/
}

int concurrent_connection_oriented(int sock, struct sockaddr *cli_addr)
{
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
			void *ptr = NULL;
			greeting(newsock);
			while(process_handler(newsock, user, ptr));
			exit(0);
		}
		close(newsock);
	}
}
/*
int single_process_concurrent(int sock, struct sockaddr *cli_addr)
{
	fd_set rfds;
	fd_set afds;
	int nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(sock, &afds);
	while(1)
	{
		memcpy(&rfds, &afds, sizeof(rfds));
		if(select(nfds, &rfds, (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL) < 0)
		{
			cout << "Error: Select error" << endl;
			return 0;
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
		}
		for(int i = 0; i < nfds; i++)
		{
			if(i == sock)break;
			if(FD_ISSET(fd, &rfds))
			{
				
			}
		}
	}
}

*/
