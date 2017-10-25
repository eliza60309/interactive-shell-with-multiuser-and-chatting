#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <iostream>

using namespace std;

int process_handler(int sock);
int rl(int sock, char *cstr);
int check_integrity(char *cstr, int size);

int process_handler(int sock)
{
	string greet = 	"****************************************\n"
					"** Welcome to the information server. **\n"
					"****************************************\n";
	write(sock, greet.c_str(), greet.length());
	while(1)
	{
		char c[10000];
		write(sock, "%", 1);
		int size = rl(sock, c);
		if(size == -1)return -1;
		if(size == 0);
		else
		{
			for(int i = 0; c[i] != '\0'; i++)
			{
				if(c[i] == '/')
				{
					cout << "Illegal character /" << endl;
					break;
				}
			}
			char *tok = NULL;
			tok = strtok(c, " \n");
			while(1)
			{
				cout << tok << endl;
				if((tok = strtok(NULL, " \n\0")) == NULL)break;
			}
		}
	}
}

int rl(int sock, char *cstr)
{
	int sum = 0;
	while(1)
	{
		char c;
		int ret = read(sock, &c, 1);
		if(ret)
		{
			if(c == '\n')
			{
				cstr[sum] = '\0';
				return sum;
			}
			else
			{
				cstr[sum] = c;
				sum++;
			}
		}
		//write(sock, c, size);
		else if(ret == -1 && errno != EINTR)return -1;
	}
}

int check_integrity(char *cstr, int begin, int size)
{
	for(int i = begin;; i++)
	{
		if(cstr[i] == '|')return i;
		else if(cstr[i] == '/')return -2;
		if(i == size)break;
	}
	return size;
}
