#pragma once
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "np_multi_proc/numberpipeinfo.hpp"
#include "np_multi_proc/sharememory.hpp"
#include "np_multi_proc/semaphore.hpp"

int shmid;
int semid;
ShareMemory* shmaddr;

enum CASES{
	STDIN_CASE = 0,
	STDOUT_CASE,
	FILEOUTPUT_CASE,
	NUMBERPIPE_IN_CASES,
	NUMBERPIPE_OUT_CASE,
	NUMBERPIPE_OUT_ERR_CASE
};

std::string welcomemsg(){
	return "****************************************\n"
	       "** Welcome to the information server. **\n"
	       "****************************************\n";
}

std::string loginmsg(ShareMemory *shmaddr, int userid){
	lock(semid);
	std::string name(shmaddr->users[userid].name);
	std::string ip(shmaddr->users[userid].ip);
	std::string port = std::to_string(shmaddr->users[userid].port);
	std::string msg = "*** User '"+ name + "' entered from " + ip + ":"+ port + ". ***\n";
	unlock(semid);
	return msg;
}

void sendmessages(ShareMemory *shmaddr, int recvid, std::string msg){
	lock(semid);
	strcpy(shmaddr->users[recvid].msgbuffer[(shmaddr->users[recvid].writeEnd)% MAX_MSG_NUM], msg.c_str());
	shmaddr->users[recvid].writeEnd++;
	unlock(semid);
	kill(shmaddr->users[recvid].pid, SIGUSR1);
}

void broadcastmsg(ShareMemory *shmaddr, std::string msg){
	lock(semid);
	std::vector<pid_t> pids;
	for(int i=1;i<MAX_USERS;i++){
		if(shmaddr->users[i].conn == true){
			strcpy(shmaddr->users[i].msgbuffer[(shmaddr->users[i].writeEnd)% MAX_MSG_NUM], msg.c_str());
			shmaddr->users[i].writeEnd++;
			pids.push_back(shmaddr->users[i].pid);
		}
	}
	unlock(semid);
	for(auto pid: pids){
		kill(pid, SIGUSR1);
	}
}

bool checkuserexist(ShareMemory *shmaddr, int userid){
	if(userid == 0) return false;
	if(userid >= MAX_USERS) return false;
	lock(semid);
	bool conn = shmaddr->users[userid].conn;
	unlock(semid);
	return conn;
}

bool checkuniname(ShareMemory *shmaddr, std::string newname){
	lock(semid);
	for(int i=1;i<MAX_USERS;i++){
		if( shmaddr->users[i].conn == true &&
			shmaddr->users[i].name == newname){
				unlock(semid); 
				return false; 
			}
	}
	unlock(semid);
	return true;
}

void changename(ShareMemory *shmaddr, int userid, std::string newname){
	bool isuni = checkuniname(shmaddr, newname);
	lock(semid);
	if(isuni){
		strcpy(shmaddr->users[userid].name, newname.c_str());
		std::string ip(shmaddr->users[userid].ip);
		std::string port = std::to_string(shmaddr->users[userid].port);
		std::string msg = "*** User from " + ip + ":" + port + " is named '" + newname + "'. ***\n";
		unlock(semid);
		broadcastmsg(shmaddr, msg);
	}else{
		unlock(semid);
		fprintf(stdout, "*** User '%s' already exists. ***\n", newname.c_str());
	}
}


void who(ShareMemory* shmaddr, int userid){
	lock(semid);
	fprintf(stdout, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
	for(int i=1;i<MAX_USERS;i++){
		if(shmaddr->users[i].conn == true){
			fprintf(stdout, "%d\t%s\t%s:%d", i, shmaddr->users[i].name, shmaddr->users[i].ip, shmaddr->users[i].port);
			if(i == userid){
				fprintf(stdout, "\t<-me");
			}
			fprintf(stdout, "\n");
		}
	}
	unlock(semid);
}

void yell(ShareMemory* shmaddr, int sendId, std::string msg){
	lock(semid);
	std::string name(shmaddr->users[sendId].name);
	unlock(semid);
	std::string wholemsg = "*** " + name + " yelled ***: " + msg + "\n";
	broadcastmsg(shmaddr, wholemsg);
}

void tell(ShareMemory* shmaddr, int sendId, int recvId, std::string msg){
	bool recvExist = checkuserexist(shmaddr, recvId);
	if(recvExist){
		lock(semid);
		std::string sendername(shmaddr->users[sendId].name);
		std::string wholemsg = "*** " + sendername + " told you ***: " + msg + "\n";
		unlock(semid);
		sendmessages(shmaddr, recvId, wholemsg);
	}else{
		fprintf(stdout, "*** Error: user #%d does not exist yet. ***\n", recvId);
	}
}

void userSetInfo(
	ShareMemory *shmaddr,
	pid_t pid,
	int userid,
	char *name,
	char *ip,
	unsigned short port,
	bool conn,
	int readEnd,
	int writeEnd){
	lock(semid);
	shmaddr->users[userid].pid = pid;
	shmaddr->users[userid].userid = userid;
	strcpy(shmaddr->users[userid].name, name);
	strcpy(shmaddr->users[userid].ip, ip);
	shmaddr->users[userid].port = port;
	shmaddr->users[userid].conn = true;
	unlock(semid);
}

int findminUserId(ShareMemory *shmaddr){
	lock(semid);
	for(int i=1;i<MAX_USERS;i++){
		if(shmaddr->users[i].conn == false){
			unlock(semid);
			return i;
		}
	}
	unlock(semid);
	return 39;
}

void ParseLineInput(std::string line_input, std::vector<std::vector<std::string> > & parsed_line_input){
	std::stringstream ss;
	std::string word;
	std::vector<std::string> cmd;
	ss << line_input;
	while(ss >> word){
		if(word == "|"){
			parsed_line_input.push_back(cmd);
			cmd.clear();
		}else{
			cmd.push_back(word);
		}
	}
	parsed_line_input.push_back(cmd);
}

bool findandparsefileoutredirect(std::vector<std::vector<std::string> > &parsed_line_input, std::string &filename){
	filename = "";
	std::vector<std::string> last = parsed_line_input.back();
	if(last.size() < 2) return false;

	if(last[last.size()-2] != ">") return false;
	filename = last.back();
	parsed_line_input.back().pop_back();
	parsed_line_input.back().pop_back();
	return true;
}

bool findandparsenumberpipeouterr(std::vector<std::vector<std::string> > &parsed_line_input, int &num){
	num = 0;
	std::string last = parsed_line_input.back().back();
	if(last.front() != '!') return false;
	last.erase(0, 1);
	num = std::stoi(last);
	parsed_line_input.back().pop_back();
	return true;
}


bool findandparsenumberpipeout(std::vector<std::vector<std::string> > &parsed_line_input, int &num){
	num = 0;
	std::string last = parsed_line_input.back().back();
	if(last.front() != '|') return false;
	last.erase(0, 1);
	num = std::stoi(last);
	parsed_line_input.back().pop_back();
	return true;
}

void PrintENV(std::vector<std::string> &parsed_line_input){
	char *pathvalue;
	if(parsed_line_input.size() == 2){
		pathvalue = getenv(parsed_line_input[1].c_str());
		if(pathvalue){
			fprintf(stdout, "%s\n", pathvalue);
		}
	}
}

void SetENV(std::vector<std::string> &parsed_line_input){
	if(parsed_line_input.size() == 3){
		setenv(parsed_line_input[1].c_str(), parsed_line_input[2].c_str(), 1);
	}
}

void SIGCHLD_handler(int signo){
    int status;
    while ((waitpid(-1, &status, WNOHANG)) > 0) {}
}

void SIGINT_handler(int signo){
	shmctl(shmid, IPC_RMID, NULL) ;
    semctl(semid, 0, IPC_RMID, NULL);
	exit(EXIT_SUCCESS);
}

void SIGUSR1_handler(int signo){ // printout message
	lock(semid);
	pid_t pid = getpid();
	int userid;
	for(userid=1; userid<MAX_USERS; userid++){
		if(shmaddr->users[userid].conn && shmaddr->users[userid].pid == pid){
			break;
		}
	}
	while(shmaddr->users[userid].readEnd < shmaddr->users[userid].writeEnd){
		fprintf(stdout, "%s", shmaddr->users[userid].msgbuffer[(shmaddr->users[userid].readEnd) % MAX_MSG_NUM]);
		fflush(stdout);
		shmaddr->users[userid].readEnd++;
	}
	unlock(semid);
}

std::deque<pid_t> ExecCMD(std::map<int, NumberPipeInfo> & pipeManager,
			   int totalline,
			   std::vector<std::vector<std::string> > & parsed_line_input,
			   int stdin_fd,
			   int stdout_fd,
			   int stderr_fd){
	int ret;
	char** execvp_str;
	std::vector<char*> ptr_list;
	int processNum = parsed_line_input.size();
	int pipeNum = processNum - 1;
	std::deque<pid_t> pids(processNum);
	int pipefds[pipeNum][2];

	for(int i=0;i<processNum;i++){ // iterative child
		if(processNum > 1){
			if(i < pipeNum){
				while(pipe(pipefds[i]) < 0) { usleep(1000); }
			}
		}
		while((pids[i] = fork()) < 0) { usleep(1000); }
		if(pids[i] == 0){ // child
			if(i == 0){
				if(stdin_fd != STDIN_FILENO){
					close(STDIN_FILENO);
					dup2(stdin_fd, STDIN_FILENO);
				}
				if(processNum > 1){
					close(STDOUT_FILENO);
					dup2(pipefds[i][1], STDOUT_FILENO);
				}
			}
			if(i == processNum - 1){
				if(processNum > 1){
					close(STDIN_FILENO);
					dup2(pipefds[i-1][0], STDIN_FILENO);
				}
				if(stdout_fd != STDOUT_FILENO){
					close(STDOUT_FILENO);
					dup2(stdout_fd, STDOUT_FILENO);
				}
				if(stderr_fd != STDERR_FILENO){
					close(STDERR_FILENO);
					dup2(stderr_fd, STDERR_FILENO);
				}
			}
			if( i != 0 && i != processNum - 1 ){
				close(STDOUT_FILENO);
				dup2(pipefds[i][1], STDOUT_FILENO);
				close(STDIN_FILENO);
				dup2(pipefds[i-1][0], STDIN_FILENO);
			}

			// close child unuse pipes (it's for parent numbered piped)
			for(auto it=pipeManager.begin(); it != pipeManager.end(); it++){
				close(it->second.m_pipe_read);
				close(it->second.m_pipe_write);
			}

			// close child unuse pipes
			if(processNum > 1){
				if(i == 0){
					close(pipefds[i][0]);
					close(pipefds[i][1]);
				}else if(i == processNum -1){
					close(pipefds[i-1][0]);
					close(pipefds[i-1][1]);
				}else{
					close(pipefds[i][0]);
					close(pipefds[i][1]);	
					close(pipefds[i-1][0]);
					close(pipefds[i-1][1]);
				}
			}

			// execute command
			ptr_list.clear();
			for (size_t j = 0; j < parsed_line_input[i].size(); ++j) {
				ptr_list.push_back(&parsed_line_input[i][j][0]);
			}
			ptr_list.push_back(NULL);
			execvp_str = &ptr_list[0];
			ret = execvp(execvp_str[0], execvp_str);
			if(ret == -1){
				fprintf(stderr, "Unknown command: [%s].\n", execvp_str[0]);
				_exit(EXIT_FAILURE);
			}
			// exit
			_exit(EXIT_SUCCESS); // end of child
		}else{ // parent
			if(i > 0){
				close(pipefds[i-1][0]);
				close(pipefds[i-1][1]);
			}

		}
	} // end of for
	return pids;
}
