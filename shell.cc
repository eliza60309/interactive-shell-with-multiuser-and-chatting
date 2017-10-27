#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <vector>

//cmd_out -2 is stdout -1 is file other is pipe
//cmd_tmp 0 is pending 1 is this turn

using namespace std;

int process_handler(int sock);
int rl(int sock, char *cstr);

int process_handler(int sock)
{
	cout << "Client connected: PID = " << getpid() << endl;
	string greet = 	"****************************************\n"
					"** Welcome to the information server. **\n"
					"****************************************\n";
	write(sock, greet.c_str(), greet.length());
	vector<vector<string> > cmd_args(5000, vector<string>());
	vector<int> cmd_out(5000, -2);
	vector<string> cmd_in(5000, string());
	vector<bool> cmd_defined(5000, 0);
	vector<bool> cmd_tmp(5000, 0);
	vector<bool> cmd_stdin(5000, 1);
	chdir("ras");
	clearenv();
	setenv("PATH", "bin:.", 1);
	//setenv("PATH", );
	int cur = 0;
	while(1)//per line operation
	{	
		int forks = 0;
		char c[10000] = {};
		write(sock, "% ", 2);
		int size = rl(sock, c);
		if(size == -1)return -1;
		if(size == 0)continue;
		int flag = 0;
		for(int i = 0; c[i] != '\0'; i++)
		{
			if(c[i] == '/')
			{
				cout << "Illegal character /" << endl;
				flag = 1;
				break;
			}
			if(c[i] < 0)
			{
	//			flag = 1;
				cout << "Client Released: PID = " << getpid() << endl;
				return 0;
			}
		}
		if(flag)continue;
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
			if(toks[i] == "exit")
			{
				cout << "Client Released: PID = " << getpid() << endl;
				return 0;
			}
			if(toks[i].find('|') != -1)
			{
				tmp = "";
				int cnt = 0;
				for(int j = 1; j < toks[i].size(); j++)
				{
					cnt *= 10;
					cnt += toks[i][j] - '0';
				}
				if(cnt == 0)cnt = 1;
				cmd_args[(fur + cur) % 5000] = rawargs;
				cmd_out[(fur + cur) % 5000] = (cnt + fur + cur) % 5000;
//				cout << "//pipe" << (fur + cur + cnt) % 5000 << endl;
				cmd_defined[(fur + cur) % 5000] = 1;
				cmd_tmp[(fur + cur) % 5000] = 1;
				rawargs.clear();
				fur++;
				cmds++;
				continue;
			}
			rawargs.push_back(toks[i]);
		}
		if(!rawargs.empty())
		{
//			cout << "////stdout" << endl;
			cmd_args[(fur + cur) % 5000] = rawargs;
			cmd_out[(fur + cur) % 5000] = -2;
			cmd_defined[(fur + cur) % 5000] = 1;
			cmd_tmp[(fur + cur) % 5000] = 1;
			fur++;
			cmds++;
			rawargs.clear();
		}

		int ln = 0;
		while(1)
		{
			if(cmd_defined[cur] == 0)break;
			if(cmd_args[cur][0] == "setenv")
			{
				setenv(cmd_args[cur][1].c_str(), cmd_args[cur][2].c_str(), 1);
				cur++;
				break;
			}
			if(cmd_args[cur][0] == "printenv")
			{
				char *st = getenv(cmd_args[cur][1].c_str());
				
				string env = (st == NULL? "": st); 
				env = cmd_args[cur][1] + "=" + env + "\n";
				write(sock, env.c_str(), env.size());
				cur++;
				break;
			}
			int tochild[2];//1 for in and 0 for out
			int fromchild[2];
			int errchild[2];
			pipe(tochild);
			pipe(fromchild);
			pipe(errchild);
			vector<char *>argc;
			string file = "";
			if(cmd_args[cur].size() >= 2 && cmd_args[cur][cmd_args[cur].size() - 2] == ">")
			{
				file = cmd_args[cur][cmd_args[cur].size() - 1];
				cmd_args[cur].pop_back();
				cmd_args[cur].pop_back();
				cmd_out[cur] = -1;
			}
			for(int i = 0; i < cmd_args[cur].size(); i++)argc.emplace_back(const_cast<char *>(cmd_args[cur][i].c_str()));
			argc.push_back(NULL);
			if(ln <= 10)
			{
				cout << "Execute(" << cur << "): ";
				for(int i = 0; i < argc.size() - 1; i++)cout << argc[i] << " ";
				if(cmd_out[cur] == -1)cout << "=>(file) " << file;
				else if(cmd_out[cur] == -2)cout << "=>(stdout)";
				else cout << "=>(pipe) " << cmd_out[cur];
				cout << endl;
			}
			if(ln == 10)cout << "  ..." << endl;
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
				dup2(errchild[1], 2);//2 for stderr
				if(execvp(argc.data()[0], argc.data()) == -1)
				{
					string err = string("Unknown command: [") + cmd_args[cur][0] + string("].\n");
					write(sock, err.c_str(), err.size());
					ptr[0] = 0;
					shmdt(ptr);
					return 0;
				}
			}
			close(tochild[0]);
			close(fromchild[1]);
			close(errchild[1]);
			write(tochild[1], cmd_in[cur].c_str() , cmd_in[cur].size());
			close(tochild[1]);
	//		int pid;
	//		wait(&pid);
			char pipebuffer[10000] = {};
			char errbuffer[10000] = {};
			read(fromchild[0], pipebuffer, 9999);
			read(errchild[0], errbuffer, 9999);
			string s(pipebuffer);
			string err(errbuffer);
			write(sock, err.c_str(), err.size());
			if(cmd_out[cur] == -2)write(sock, s.c_str(), s.size());
			else if(cmd_out[cur] == -1)
			{
				fstream stream;
				stream.open(file.c_str(), fstream::out);
				stream << s;
			}
			else cmd_in[cmd_out[cur]] += s;
			close(fromchild[0]);
			string save = "";
			if(!ptr[0]) 
			{
				cout << "Exec error: " << argc.data()[0] << endl;
				if(ln)save = cmd_in[cur] + cmd_in[cur + 1];
				for(int i = 0; i < 5000; i++)
				{
					if(cmd_tmp[i])
					{
						cmd_args[i].clear();
						cmd_defined[i] = 0;
						cmd_out[i] = -2;
						cmd_stdin[i] = 1;
						cmd_tmp[i] = 0;
					}
				}
			}
			cmd_args[cur].clear();
			cmd_defined[cur] = 0;
			cmd_out[cur] = -2;
			cmd_in[cur] = save;
			cmd_stdin[cur] = 1;
			cmd_tmp[cur] = 0;
			if(ptr[0])cur = (cur + 1) % 5000;
			else if(!ln)cur = (cur + 1) % 5000;
			shmdt(ptr);
			shmctl(shmid, IPC_RMID, NULL);
			ln++;
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
