#pragma once
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "numberpipeinfo.hpp"
#include "UserInfo.hpp"
#define MAX_USERS 40
std::vector<UserPipeInfo>::iterator checkuserpipeexists(std::vector<UserPipeInfo> &userPipeManager, int senderid, int recvid){
	std::vector<UserPipeInfo>::iterator it;
	for(it=userPipeManager.begin(); it!= userPipeManager.end(); it++){
		if(it->send_userid == senderid && it->recv_userid == recvid){
			return it;
		}
	}
	return it;
}

bool finduserpipeCMD(std::vector<std::vector<std::string> > &ParseLineInput, int &senderid, int &recvid){
	recvid = 0;
	for(size_t i=0;i<ParseLineInput.size();i++){
		// for(auto &s: ParseLineInput[i]){
		for(auto iter=ParseLineInput[i].begin(); iter!=ParseLineInput[i].end(); iter++){
			if(iter->front() == '>' && iter->size() != 1){
				iter->erase(0, 1);
				recvid = std::stoi(*iter);
				ParseLineInput[i].erase(iter);
				break;
			}
		}
	}
	senderid = 0;
	for(size_t i=0;i<ParseLineInput.size();i++){
		// for(auto &s: ParseLineInput[i]){
		for(auto iter=ParseLineInput[i].begin(); iter!=ParseLineInput[i].end(); iter++){
			if(iter->front() == '<' && iter->size() != 1){
				iter->erase(0, 1);
				senderid = std::stoi(*iter);
				ParseLineInput[i].erase(iter);
				break;
			}
		}
	}
	return ((senderid != 0) || (recvid != 0)) ;
}

std::string getFileName(std::vector<std::string> last){
	if(last.size() < 2) return "";
	if(last[last.size()-2] == ">"){
		return last.back();
	}
	return "";

}

bool checkuserexist(std::vector<UserInfo> &users, int userid){
	return ((userid != 0) && (userid < MAX_USERS) && (users[userid].conn));
}

int findminUserId(std::vector<UserInfo> &users){
	for(auto user: users){
		if(user.userid != 0 && user.conn == false) return user.userid;
	}
	return 39;
}

int findUserIdBySock(std::vector<UserInfo> &users, int sock){
	for(auto user: users){
		if( user.userid != 0 &&
		    user.sockfd == sock
		  ) { return user.userid; }
	}
	return 39;
}

std::string welcomemsg(){
	return "****************************************\n"
	       "** Welcome to the information server. **\n"
	       "****************************************\n";
}

std::string loginmsg(std::vector<UserInfo> &users, int userid){
	UserInfo user = users[userid];
	return "*** User '"+ user.name + "' entered from " + user.ip + ":"+ std::to_string(user.port) + ". ***\n";
}


std::string logoutmsg(std::vector<UserInfo> &users, int userid){
	UserInfo user = users[userid];
	return "*** User '" + user.name + "' left. ***\n";
}

std::string userpipesendmsg(std::vector<UserInfo> &users, int senderid, int recevid, std::string cmd){
	UserInfo sender = users[senderid];
	UserInfo recever = users[recevid];
	return "*** " + sender.name + " (#" + std::to_string(senderid) + ") just piped '" + cmd + "' to " + recever.name + " (#" + std::to_string(recevid) + ") ***\n";
}

std::string userpiperecvmsg(std::vector<UserInfo> &users, int senderid, int recevid, std::string cmd){
	UserInfo sender = users[senderid];
	UserInfo recever = users[recevid];
	return "*** " + recever.name + " (#" + std::to_string(recevid) + ") just received from " + sender.name + " (#" + std::to_string(senderid) + ") by '" + cmd + "' ***\n";
}

void sendmessages(int sockfd, std::string msg){
	if(send(sockfd, msg.c_str(), msg.size(), 0) < 0) { perror("send error");}
}

void broadcastmsg(std::vector<UserInfo> &users, std::string msg){
	for(auto user: users){
		if(user.userid != 0 && user.conn == true){
			sendmessages(user.sockfd, msg);
		}
	}
}

bool checkuniname(std::vector<UserInfo> &users, std::string newname){
	for(auto user: users){
		if( user.userid != 0 &&
			user.conn == true &&
			user.name == newname
		  ) { return false; }
	}
	return true;
}

void who(std::vector<UserInfo> &users, int userid){
	fprintf(stdout, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
	for(auto user: users){
		if( user.userid != 0 && user.conn == true){
			fprintf(stdout, "%d\t%s\t%s:%d", user.userid, user.name.c_str(), user.ip.c_str(), user.port);
			if(user.userid == userid){
				fprintf(stdout, "\t<-me");
			}
			fprintf(stdout, "\n");
		}
	}
}

void tell(std::vector<UserInfo> &users, int sendId, int recvId, std::string msg){
	bool recvExist = users[recvId].conn;
	if(recvExist){
		sendmessages(users[recvId].sockfd, "*** " + users[sendId].name + " told you ***: " + msg + "\n");
	}else{
		fprintf(stdout, "*** Error: user #%d does not exist yet. ***\n", recvId);
	}
}

void yell(std::vector<UserInfo> &users, int sendId, std::string msg){
	broadcastmsg(users, "*** " + users[sendId].name + " yelled ***: " + msg + "\n");
}

void changename(std::vector<UserInfo> &users, int userid, std::string newname){
	UserInfo user = users[userid];
	if(checkuniname(users, newname)){
		users[userid].name = newname;
		broadcastmsg(users, "*** User from " + user.ip + ":" + std::to_string(user.port) + " is named '" + newname + "'. ***\n");
	}else{
		fprintf(stdout, "*** User '%s' already exists. ***\n", newname.c_str());
	}
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
void PrintENV(std::vector<std::string> &parsed_line_input){
	char *pathvalue;
	if(parsed_line_input.size() == 2){
		pathvalue = getenv(parsed_line_input[1].c_str());
		if(pathvalue){
			fprintf(stdout, "%s\n", pathvalue);
		}
	}
}
void SetENV(std::vector<std::string> &parsed_line_input, UserInfo &user){
	if(parsed_line_input.size() == 3){
		setenv(parsed_line_input[1].c_str(), parsed_line_input[2].c_str(), 1);
		user.env[parsed_line_input[1].c_str()] = parsed_line_input[2].c_str();
	}
}
void SIGCHLD_handler(int signo){
    int status;
    while ((waitpid(-1, &status, WNOHANG)) > 0) {}
}


std::deque<pid_t> ExecCMD(std::map<int, NumberPipeInfo> & pipeManager,
			   int totalline,
			   std::vector<std::vector<std::string> > & parsed_line_input,
			   int stdin_fd,
			   int stdout_fd,
			   int stderr_fd,
			   std::string filename,
			   std::map<int, NumberPipeInfo>::iterator findit){
	int ret;
	int filefd;
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
				}else if(filename != ""){
					// fprintf(stderr, "Here\n");
					filefd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
					// fprintf(stderr, "filefd:%d\n", filefd);
					close(STDOUT_FILENO);
					dup2(filefd, STDOUT_FILENO);
					close(filefd);
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
	
	// parent close stdin pipe
	// if(stdin_fd != STDIN_FILENO){
	// 	close(findit->second.m_pipe_read);
	// 	close(findit->second.m_pipe_write);
	// }

	// if(stdout_fd == STDOUT_FILENO){
	// 	for(auto its = findit->second.m_wait_pids.begin(); its != findit->second.m_wait_pids.end(); its++){
	// 		waitpid(*its, nullptr, 0);
	// 	}
	
	// 	// wait children
	// 	for(pid_t pid: pids){
	// 		waitpid(pid, nullptr, 0);
	// 	}
	// }

	return pids;
	
}
