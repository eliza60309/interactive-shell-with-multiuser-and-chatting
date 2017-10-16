#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <iostream>

#define SERV_TCP_PORT 5566

using namespace std;

int main()
{
	struct sockaddr_in cli_addr, serv_addr;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		cout << "Error not open socket" << endl;
		return 0;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_TCP_PORT);
	int bin = bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if(bin < 0)
	{
		cout << "server: can't bind local address" << endl;
		return 0;
	}
	listen(sock, 5);
	while(1)
	{
		unsigned int clilen = sizeof(cli_addr);
		int newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
		int child;
		if((child = fork()) < 0)
		{
			cout << "server: accept error" << endl;
			return 0;
		}
		else if(child == 0) 
		{
			cout << "suc" << endl;
			close(sock);
			exit(0);
		}
		close(newsock);
	}
}

