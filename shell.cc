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

class indiv
{
	public: 
	vector<vector<string> > cmd_args;
	vector<int> cmd_out;
	vector<string> cmd_in;
	vector<bool> cmd_defined;
	vector<bool> cmd_tmp;
	vector<bool> cmd_stdin;
	//string env;
	vector<string> env_var;
	vector<string> env_value;
	string rawcmd;
	int cur;
	indiv()
	{
		cmd_args = vector<vector<string> >(5000, vector<string>());
		cmd_out = vector<int>(5000, -2);
		cmd_in = vector<string>(5000, string());
		cmd_defined = vector<bool>(5000, 0);
		cmd_tmp = vector<bool>(5000, 0);
		cmd_stdin = vector<bool>(5000, 1);
		env_var = vector<string>(1, "PATH");
		env_value = vector<string>(1, "bin:.");
		rawcmd = string();
		cur = 0;
	}
};

int process_handler(int sock, indiv &user, void *ptr);
int rl(int sock, char *cstr);
int greeting(int sock);

int process_handler(int sock, indiv &user, void *ptr)
{
/*	vector<vector<string> > cmd_args(5000, vector<string>());
	vector<int> cmd_out(5000, -2);
	vector<string> cmd_in(5000, string());
	vector<bool> cmd_defined(5000, 0);
	vector<bool> cmd_tmp(5000, 0);
	vector<bool> cmd_stdin(5000, 1);
	chdir("ras");
	clearenv();
	setenv("PATH", "bin:.", 1);
	//setenv("PATH", );*/
	vector<vector<string> > &cmd_args = user.cmd_args;
	vector<int> &cmd_out = user.cmd_out;
	vector<string> &cmd_in = user.cmd_in;
	vector<bool> &cmd_defined = user.cmd_defined;
	vector<bool> &cmd_tmp = user.cmd_tmp;
	vector<bool> &cmd_stdin = user.cmd_stdin;
	vector<string> &env_var = user.env_var;
	vector<string> &env_value = user.env_value;
	chdir("ras");
	clearenv();
	//setenv("PATH", "bin:.", 1);
	for(int i = 0; i < env_var.size(); i++)setenv(env_var[i].c_str(), env_value[i].c_str(), 1);
	int &cur = user.cur;
//	while(1)//per line operation
//	{	
	int forks = 0;
	char c[10000] = {};
	int size = rl(sock, c);
	if(size == -1)return 0;
	if(size == 0)return 1;
	for(int i = 0; c[i] != '\0'; i++)
	{
		if(c[i] == '/')
		{
			cout << "Illegal character /" << endl;
			return 1;
		}
		if(c[i] < 0)
		{
			cout << "Client Released: PID = " << getpid() << endl;
			return 0;
		}
	}
	vector<string> toks;
	char *tok = NULL;
	tok = strtok(c, " \r\n\0");
	if(tok == NULL)return 1;
	do toks.push_back(string(tok));
	while((tok = strtok(NULL, " \r\n\0")) != NULL);
	string tmp;
	vector<string> rawargs;
	int fur = 0;
	int cmds = 0;
	for(int i = 0; i < toks.size(); i++)//token operation
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
			int pos = -1;
			for(int i = 0; i < env_var.size(); i++)
			{
				if(cmd_args[cur][1] == env_var[i])
				{
					pos = i;
					break;
				}
			}
			if(pos == -1)
			{
				env_var.push_back(cmd_args[cur][1]);
				env_value.push_back(cmd_args[cur][2]);
			}
			else env_value[pos] = cmd_args[cur][2];
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
		cout << "Execute(" << cur << "): ";
		for(int i = 0; i < argc.size() - 1; i++)cout << argc[i] << " ";
		if(cmd_out[cur] == -1)cout << "=>(file) " << file;
		else if(cmd_out[cur] == -2)cout << "=>(stdout)";
		else cout << "=>(pipe) " << cmd_out[cur];
		cout << endl;
		int shmid = shmget(0, sizeof(int), IPC_CREAT | 0666);
		int *ptr = (int *)shmat(shmid, NULL, 0);
		ptr[0] = 1;
		int p = fork();
		if(p == -1)
		{
			cout << "NO FORK" << endl;
			return 0;
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
/////////////////////
				exit(0);
			}
		}
		close(tochild[0]);
		close(fromchild[1]);
		close(errchild[1]);
		write(tochild[1], cmd_in[cur].c_str() , cmd_in[cur].size());
		close(tochild[1]);
		char errbuffer[100000] = {};
		char pipebuffer[100000] = {};
		read(fromchild[0], pipebuffer, 100000);
		read(errchild[0], errbuffer, 100000);
		string s(pipebuffer);
		string err(errbuffer);
		write(sock, err.c_str(), err.size());
		if(cmd_out[cur] == -2)
		{
			int i = 0;
			while(1)
			{
				int k = write(sock, s.c_str() + i, s.size());
				i += k;
				if(i == s.size())break;
			}
		}
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
	return 1;
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

int greeting(int sock)
{
	cout << "Client connected: PID = " << getpid() << endl;
	string greet = 	"****************************************\n"
					"** Welcome to the information server. **\n"
					"****************************************\n";
	write(sock, greet.c_str(), greet.length());
}
