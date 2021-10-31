#pragma once
#include "utils.hpp"
#include "numberpipeinfo.hpp"
#include "UserInfo.hpp"
#include <queue>
#include <iostream>
#include <map>

#include <signal.h>
#include <string.h>
#include <unistd.h>

void single_proc_shell(std::vector<UserInfo> &users, int userid){
	int numberpipefd[2];
	std::string line_input;
	std::vector<std::vector<std::string> > parsed_line_input;
	std::map<int, NumberPipeInfo>::iterator findit;
	std::map<int, NumberPipeInfo>::iterator findit2;
	int stderr_fd = STDERR_FILENO;

    UserInfo &user = users[userid];
	clearenv();
	for(auto env : user.env){
        setenv(env.first.c_str(), env.second.c_str(), 1);
    }

	std::getline(std::cin, line_input);

	if(std::cin.eof()) { fprintf(stdout, "\n"); user.conn = false;  return;}
	size_t countspace = 0;
	for(auto li: line_input){
		if(li == ' ') countspace++;
	}
	if (countspace == line_input.size() ||
	   (countspace == line_input.size() - 1 && line_input.back() == '\r')) 
	   { return; }

	ParseLineInput(line_input, parsed_line_input);

	std::string first = parsed_line_input.front().front();
	std::string last = parsed_line_input.back().back();
	if(first == "printenv"){
		PrintENV(parsed_line_input.front());
	}else if(first == "setenv"){
		SetENV(parsed_line_input.front(), user);
	}else if(first == "exit"){
		user.conn = false;
		return;
	}else if(first == "who"){
		who(users, userid);
	}else if(first == "tell"){
        size_t found = line_input.find(parsed_line_input.front()[1]);
        found += parsed_line_input.front()[1].size() + 1;
        std::string tellmsg = line_input.substr(found);
        tell(users, userid, std::stoi(parsed_line_input.front()[1]), tellmsg);
	}else if(first == "yell"){
        size_t found = line_input.find(parsed_line_input.front()[0]);
        found += parsed_line_input.front()[0].size() + 1;
        std::string yellmsg = line_input.substr(found);
        yell(users, userid, yellmsg);
	}else if(first == "name"){
        changename(users, userid, parsed_line_input.front()[1]);
	}else if(last.front() == '|' || last.front() == '!'){ // numberpipe
		last.erase(0, 1);
		int num = std::stoi(last);
		parsed_line_input.back().pop_back();
		findit = user.pipeManager.find(user.totalline);  // find if there exist pipes go to current line
		findit2 = user.pipeManager.find(user.totalline + num); // find if there exist pipes go to the same destination
		if(findit != user.pipeManager.end() && findit2 != user.pipeManager.end()){
			stderr_fd = (last.front() == '!') ? findit2->second.m_pipe_write: STDERR_FILENO;
			user.pids = ExecCMD( user.pipeManager,
								 user.totalline,
								 parsed_line_input,
								 findit->second.m_pipe_read,
								 findit2->second.m_pipe_write,
								 stderr_fd,
								 "",
								 findit);
			for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
				findit2->second.m_wait_pids.push_front(*it);
			}
			for(auto it = user.pids.begin(); it != user.pids.end(); it++){
				findit2->second.m_wait_pids.push_back(*it);
			}
			user.pipeManager.erase(findit);

		}else if (findit != user.pipeManager.end()){ // there exist pipes go to current line
			while(pipe(numberpipefd) < 0){
				// fprintf(stderr, "%s\n", strerror(errno));
				usleep(1000);
			}
			stderr_fd = (last.front() == '!') ? numberpipefd[1]: STDERR_FILENO;
			user.pids = ExecCMD(user.pipeManager,
								user.totalline,
								parsed_line_input,
								findit->second.m_pipe_read,
								numberpipefd[1],
								stderr_fd,
								"",
								findit);
			for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
				user.pids.push_front(*it);
			}
			user.pipeManager.erase(findit);
			user.pipeManager[user.totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], user.pids);

		}else if (findit2 != user.pipeManager.end()){ // there exist pipes go to the same destination
			stderr_fd = (last.front() == '!') ? findit2->second.m_pipe_write : STDERR_FILENO;
			user.pids = ExecCMD(user.pipeManager,
								user.totalline,
								parsed_line_input,
								STDIN_FILENO,
								findit2->second.m_pipe_write,
								stderr_fd,
								"",
								findit);
			for(auto it = user.pids.begin(); it != user.pids.end(); it++){
				findit2->second.m_wait_pids.push_back(*it);
			}
		}else{ // none of above
			while(pipe(numberpipefd) < 0){
				// fprintf(stderr, "%s\n", strerror(errno));
				usleep(1000);
			}
			stderr_fd = (last.front() == '!') ? numberpipefd[1] : STDERR_FILENO;
			user.pids = ExecCMD(user.pipeManager,
								user.totalline,
								parsed_line_input,
								STDIN_FILENO,
								numberpipefd[1],
								stderr_fd,
								"",
								findit);
			user.pipeManager[user.totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], user.pids);
			// fprintf(stderr, "insert \n");
			// std::cerr << user.pipeManager[user.totalline + num].m_pipe_read << std::endl;
		}
	}else{
		std::string filename = getFileName(parsed_line_input.back());
		if(filename != ""){
			parsed_line_input.back().pop_back();
			parsed_line_input.back().pop_back();
		}
		findit = user.pipeManager.find(user.totalline);
		if(findit != user.pipeManager.end()){
			ExecCMD(user.pipeManager,
					user.totalline,
					parsed_line_input,
					findit->second.m_pipe_read,
					STDOUT_FILENO,
					STDERR_FILENO,
					filename,
					findit);
			user.pipeManager.erase(findit);
		}else{
		    ExecCMD(user.pipeManager,
					user.totalline,
					parsed_line_input,
					STDIN_FILENO,
					STDOUT_FILENO,
					STDERR_FILENO,
					filename,
					findit);
		}
	}
	user.totalline++;
	
	return;
}
