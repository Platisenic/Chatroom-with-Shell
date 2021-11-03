#pragma once
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include "np_simple/numberpipeinfo.hpp"

enum CASES{
	STDIN_CASE = 0,
	STDOUT_CASE,
	FILEOUTPUT_CASE,
	NUMBERPIPE_IN_CASES,
	NUMBERPIPE_OUT_CASE,
	NUMBERPIPE_OUT_ERR_CASE
};

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
