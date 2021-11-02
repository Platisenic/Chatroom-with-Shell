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
#include "np_simple/lineinputinfo.hpp"


LineInputInfo ParseLineInput(std::string line_input, std::vector<std::vector<std::string> > &parsed_line_input){
	std::stringstream ss;
	std::string word;
	std::vector<std::string> cmd;
	int number;
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

	ss.str(""); 
	ss.clear();

	std::string first = cmd.front();
	std::string last = cmd.back();
	std::string last2 = (cmd.size() < 2 ) ?  "" : cmd.end()[-2];

	if(first == "printenv"){
		return LineInputInfo(CASES::PRINTENV, 0, "");
	}else if(first == "setenv"){
		return LineInputInfo(CASES::SETENV, 0, "");
	}else if(first == "exit"){
		return LineInputInfo(CASES::EXIT, 0, "");
	}else if(last[0] == '|'){
		last.erase(0, 1);
		ss << last;
		ss >> number;
		parsed_line_input.back().pop_back();
		return LineInputInfo(CASES::STDOUTNUMBERPIPE, number, "");
	}else if(last[0] == '!'){
		last.erase(0, 1);
		ss << last;
		ss >> number;
		parsed_line_input.back().pop_back();
		return LineInputInfo(CASES::STDOUTSTDERRNUMBERPIPE, number, "");
	}else if(last2 == ">"){
		parsed_line_input.back().pop_back();
		parsed_line_input.back().pop_back();
		return LineInputInfo(CASES::FILEOUTPUTREDIRECT, 0, last);
	}else{
		return LineInputInfo(CASES::ORDINARYPIPEORNOPIPE, 0, "");
	}
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
				while(pipe(pipefds[i]) < 0){
					// fprintf(stderr, "%s\n", strerror(errno));
					usleep(1000);
				}
			}
		}
		while((pids[i] = fork()) < 0){
			// fprintf(stderr, "%s\n", strerror(errno));
			usleep(1000);
		}
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
	if(stdin_fd != STDIN_FILENO){
		close(findit->second.m_pipe_read);
		close(findit->second.m_pipe_write);
	}
	if(stdout_fd == STDOUT_FILENO){
		for(auto its = findit->second.m_wait_pids.begin(); its != findit->second.m_wait_pids.end(); its++){
			waitpid(*its, nullptr, 0);
		}
		for(pid_t pid: pids){
			waitpid(pid, nullptr, 0);
		}
	}
	return pids;
}

