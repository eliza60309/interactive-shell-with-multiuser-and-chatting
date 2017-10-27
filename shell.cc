#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <iostream>
#include <vector>
#define NOWHERE 0
#define PIPE 1
#define FS -1

//cmd_out 0 is stdout -1 is file other is pipe
//cmd_tmp 0 is pending 1 is this turn

using namespace std;

int process_handler(int sock);
int rl(int sock, char *cstr);

int process_handler(int sock)
{
	string greet = 	"****************************************\n"
					"** Welcome to the information server. **\n"
					"****************************************\n";
	write(sock, greet.c_str(), greet.length());
	vector<vector<string> > cmd_args(1000, vector<string>());
	vector<int> cmd_out(1000, 0);
	vector<string> cmd_in(1000, string());
	vector<bool> cmd_defined(1000, 0);
	vector<bool> cmd_tmp(1000, 0);
	vector<bool> cmd_stdin(1000, 1);
	
	int cur = 0;
	while(1)//per line operation
	{
		int forks = 0;
		char c[10000] = {};
		write(sock, "% ", 2);
		int size = rl(sock, c);
		if(size == -1)return -1;
		if(size == 0)continue;
		for(int i = 0; c[i] != '\0'; i++)
		{
			if(c[i] == '/')
			{
				cout << "Illegal character /" << endl;
				continue;
			}
		}
		vector<string> toks;
		char *tok = NULL;
		tok = strtok(c, " \r\n");
		if(tok == NULL)continue;
		do toks.push_back(string(tok));
		while((tok = strtok(NULL, " \r\n")) != NULL);
		string tmp;
		vector<string> rawargs;
		int fur = 0;
		int cmds = 0;
		for(int i = 0; i < toks.size(); i++)//per token operation
		{
//			cout << "tok: (" << toks[i] << ") ";
			if(toks[i] == "printenv");
			if(toks[i] == "setenv");
			if(toks[i] == "exit")
			{
				cout << "client released" << endl;
				return 0;
			}
			if(toks[i].find('|') != -1)
			{
				tmp = "";
				int cnt = 0;
//				cout << "Pipe found: " << toks[i] << endl;
				for(int j = 1; j < toks[i].size(); j++)
				{
					cnt *= 10;
					cnt += toks[i][j] - '0';
				}
				if(cnt == 0)cnt = 1;
				cmd_args[(fur + cur) % 1000] = rawargs;
				cmd_out[(fur + cur) % 1000] = (cnt + fur + cur) % 1000;
				cmd_defined[(fur + cur) % 1000] = 1;
				cmd_tmp[(fur + cur) % 1000] = 1;
				rawargs.clear();
				fur++;
				cmds++;
				continue;
			}
			if(1)
			{
				rawargs.push_back(toks[i]);
//				cout << "Command body: " << toks[i] << endl;
			}
		}
//		cout << endl;
		if(!rawargs.empty())
		{
			cmd_args[(fur + cur) % 1000] = rawargs;
			cmd_out[(fur + cur) % 1000] = 0;
			cmd_defined[(fur + cur) % 1000] = 1;
			cmd_tmp[(fur + cur) % 1000] = 1;
			fur++;
			cmds++;
			rawargs.clear();
		}

		//this area is just testing
		
/*		for(int i = cur; i < cur + cmds; i++)
		{
			cout << "Command: ";
			for(int j = 0; j < cmd_args[i].size(); j++)cout << "\"" << cmd_args[i][j] << "\" ";
		//	cout << "pipe to: " << cmd_out[i] << endl;
		}
		cout << endl;*/

		//executing
		bool suc = 0;
		while(1)
		{
			if(cmd_defined[cur] == 0)break;
			int tochild[2];//1 for in and 0 for out
			int fromchild[2];
			pipe(tochild);
			pipe(fromchild);
			vector<char *>argc;
			string file = "";
			if(cmd_args[cur][cmd_args[cur].size() - 2] == string(">"))
			{
				file = cmd_args[cur][cmd_args[cur].size() - 1];
				cout << "file redirection: " << file << endl;
				cmd_args[cur].pop_back();
				cmd_args[cur].pop_back();
				cmd_out[cur] = -1;
			}
			for(int i = 0; i < cmd_args[cur].size(); i++)argc.emplace_back(const_cast<char *>(cmd_args[cur][i].c_str()));
			argc.push_back(NULL);
			cout << "Execute(" << cur << "): ";
			for(int i = 0; i < argc.size() - 1; i++)cout << argc[i] << " ";
			if(cmd_out[cur] < 0)cout << "=>(file) " << file;
			else if(cmd_out[cur] == 0)cout << "=>(stdout)";
			else cout << "=>(pipe) " << cmd_out[cur];
			cout << endl;
	//		char pipebuffer_in[10000] = {};
			int shmid = shmget(0, sizeof(int), IPC_CREAT | 0666);
			int *ptr = (int *)shmat(shmid, NULL, 0);
			ptr[0] = 1;
			int p = fork();
			if(p == -1)
			{
				cout << "NO FORK" << endl;
				return -1;
			}
			if(p == 0)
			{
				close(tochild[1]);
				close(fromchild[0]);
				dup2(tochild[0], 0);//0 for stdin
				dup2(fromchild[1], 1);//1 for stdout
				if(execvp(argc.data()[0], argc.data()) == -1)
				{
					cout << "Exec error: " << argc.data()[0] << endl;
					ptr[0] = 0;
					shmdt(ptr);
					return 0;
				}
			}
			close(tochild[0]);
			close(fromchild[1]);
			write(tochild[1], cmd_in[cur].c_str() , cmd_in[cur].size());
			close(tochild[1]);
			int pid;
			wait(&pid);
			char pipebuffer[10000] = {};
			read(fromchild[0], pipebuffer, 9999);
			string s(pipebuffer);
			cout << "input:" << cmd_in[cur] << endl;
			cout << "output:" << s << endl;
			//cout << "s" << s << endl;
			if(cmd_out[cur] == 0)write(sock, s.c_str(), s.size());
	//		else if(cmd_out[cur] == -1)
	//		{
	//			cout << "file redirection? ok, to " << file << endl;
	//		}
			else cmd_in[cmd_out[cur]] = pipebuffer;
			close(fromchild[0]);
			if(ptr[0])suc = 1;
			else 
			{
				for(int i = 0; i < 1000; i++)
				{
					if(cmd_tmp[i])
					{
						if(suc)cur--;
						if(cur < 0)cur = 0;
						cmd_args[i].clear();
						cmd_defined[i] = 0;
						cmd_out[i] = 0;
						cmd_in[i] = "";
						cmd_stdin[i] = 1;
						cmd_tmp[i] = 0;
					}
				}
			}
			shmdt(ptr);
			shmctl(shmid, IPC_RMID, NULL);
			cmd_args[cur].clear();
			cmd_defined[cur] = 0;
			cmd_out[cur] = 0;
			cmd_in[cur] = "";
			cmd_stdin[cur] = 1;
			cmd_tmp[cur] = 0;
			cur = (cur + 1) % 1000;
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
		else if(ret == -1 && errno != EINTR)return -1;
	}
}

//to do 
// pipe
// fork
// setenv
// exit
