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
	std::vector<UserPipeInfo>::iterator findit3;
	std::vector<UserPipeInfo>::iterator findit4;
	std::deque<pid_t> pids;
	int senderid = 0;
	int recvid = 0;
	int stdin_fd = STDIN_FILENO;
	int stdout_fd = STDOUT_FILENO;
	int stderr_fd = STDERR_FILENO;

    UserInfo &user = users[userid];
	clearenv();
	for(auto env : user.env){
        setenv(env.first.c_str(), env.second.c_str(), 1);
    }

	std::getline(std::cin, line_input);

	if(std::cin.eof()) { fprintf(stdout, "\n"); user.conn = false;  return;}
	if(line_input.back() == '\r') line_input.pop_back();
	size_t countspace = 0;
	for(auto li: line_input){
		if(li == ' ') countspace++;
	}
	if (countspace == line_input.size()) { return; }

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
			close(findit->second.m_pipe_read);
			close(findit->second.m_pipe_write);
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
			close(findit->second.m_pipe_read);
			close(findit->second.m_pipe_write);
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
	}else if(finduserpipeCMD(parsed_line_input, senderid, recvid)){
		if(senderid != 0 && recvid != 0){
			// TODO
		}else if(senderid != 0){ // receive message
			if(!checkuserexist(users, senderid)){
				fprintf(stdout, "*** Error: user #%d does not exist yet. ***\n", senderid);
				user.totalline ++; // not sure TODO
				return;
			}
			recvid = userid;
			findit3 = checkuserpipeexists(user.userPipeManager, senderid, recvid);
			if(findit3 == user.userPipeManager.end()){
				fprintf(stdout, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", senderid, recvid);
				user.totalline ++; //not sure TODO
				return;
			}
			broadcastmsg(users, userpiperecvmsg(users, senderid, recvid, line_input));
			pids = ExecCMD(user.pipeManager,
					user.totalline,
					parsed_line_input,
					findit3->m_pipe_read,
					STDOUT_FILENO,
					STDERR_FILENO,
					"",
					findit);
			for(auto its = findit3->m_wait_pids.begin(); its != findit3->m_wait_pids.end(); its++){
				waitpid(*its, nullptr, 0);
			}
			for(pid_t pid: pids){
				waitpid(pid, nullptr, 0);
			}
			close(findit3->m_pipe_read);
			user.userPipeManager.erase(findit3);
		}else{ // recvid != 0, send message
			if(!checkuserexist(users, recvid)){
				fprintf(stdout, "*** Error: user #%d does not exist yet. ***\n", recvid);
				user.totalline ++; // not sure TODO
				return;
			}
			senderid = userid;
			findit3 = checkuserpipeexists(users[recvid].userPipeManager, senderid, recvid);
			if(findit3 != users[recvid].userPipeManager.end()){
				fprintf(stdout, "*** Error: the pipe #%d->#%d already exists. ***\n", senderid, recvid);
				user.totalline ++; //not sure TODO
				return;
			}
			broadcastmsg(users, userpipesendmsg(users, senderid, recvid, line_input));
			while(pipe(numberpipefd) < 0) { usleep(1000); }
			pids = ExecCMD(user.pipeManager,
					user.totalline,
					parsed_line_input,
					STDIN_FILENO,
					numberpipefd[1],
					STDERR_FILENO,
					"",
					findit);

			users[recvid].userPipeManager.push_back(UserPipeInfo(
				numberpipefd[0],
				numberpipefd[1],
				senderid,
				recvid,
				pids
			));
			close(numberpipefd[1]);
		}

	}else{
		std::string filename = getFileName(parsed_line_input.back());
		if(filename != ""){
			parsed_line_input.back().pop_back();
			parsed_line_input.back().pop_back();
		}
		findit = user.pipeManager.find(user.totalline);
		if(findit != user.pipeManager.end()){
			pids = ExecCMD(user.pipeManager,
					user.totalline,
					parsed_line_input,
					findit->second.m_pipe_read,
					STDOUT_FILENO,
					STDERR_FILENO,
					filename,
					findit);
			close(findit->second.m_pipe_read);
			close(findit->second.m_pipe_write);
			for(auto its = findit->second.m_wait_pids.begin(); its != findit->second.m_wait_pids.end(); its++){
				waitpid(*its, nullptr, 0);
			}
			for(pid_t pid: pids){
				waitpid(pid, nullptr, 0);
			}
			user.pipeManager.erase(findit);
		}else{
		    pids = ExecCMD(user.pipeManager,
					user.totalline,
					parsed_line_input,
					STDIN_FILENO,
					STDOUT_FILENO,
					STDERR_FILENO,
					filename,
					findit);
			for(pid_t pid: pids){
				waitpid(pid, nullptr, 0);
			}
		}
	}
	
	user.totalline++;
	return;
}
