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

int main()
{
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
	while(1)
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
/*			cout << "Logger: Redirected to " + newsock << endl;
			write(newsock, "hello\n", sizeof("hello\n"));
			cout << "Logger: \"" << newsock << "\": welcoming message" << endl;
			char c[1000] = {};
			read(newsock, (void *)c, sizeof(c));
			cout << c << endl;
*/
			exit(0);
		}
		close(newsock);
	}
}

