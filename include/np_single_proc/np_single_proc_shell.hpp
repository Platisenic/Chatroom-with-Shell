#pragma once
#include "np_single_proc/utils.hpp"
#include "np_single_proc/numberpipeinfo.hpp"
#include "np_single_proc/UserInfo.hpp"
#include <queue>
#include <iostream>
#include <map>

#include <signal.h>
#include <string.h>
#include <unistd.h>

void shell(std::vector<UserInfo> &users, int userid, int serverlogfd){
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
	int num = 0;
	std::string filename = "";
	std::string devnull = "/dev/null";
    UserInfo &user = users[userid];
	clearenv();
	for(auto env : user.env){
        setenv(env.first.c_str(), env.second.c_str(), 1);
    }

	std::getline(std::cin, line_input);
	// dprintf(serverlogfd, "line_input: %s\n", line_input.c_str());
	if(std::cin.eof()) { fprintf(stdout, "\n"); user.conn = false;  return;}
	if(line_input.back() == '\r') line_input.pop_back();
	size_t countspace = 0;
	for(auto li: line_input){
		if(li == ' ') countspace++;
	}
	if (countspace == line_input.size()) { return; }

	ParseLineInput(line_input, parsed_line_input);

	std::string first = parsed_line_input.front().front();
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
		if(parsed_line_input.front().size() >= 2){
			size_t found = line_input.find(parsed_line_input.front()[1]);
			found += parsed_line_input.front()[1].size() + 1;
			std::string tellmsg = line_input.substr(found);
			tell(users, userid, std::stoi(parsed_line_input.front()[1]), tellmsg);
		}
	}else if(first == "yell"){
        size_t found = line_input.find(parsed_line_input.front()[0]);
        found += parsed_line_input.front()[0].size() + 1;
        std::string yellmsg = line_input.substr(found);
        yell(users, userid, yellmsg);
	}else if(first == "name"){
		if(parsed_line_input.front().size() == 2){
        	changename(users, userid, parsed_line_input.front()[1]);
		}
	}else{
		enum::CASES instream_case = STDIN_CASE;
		enum::CASES outstream_case = STDOUT_CASE;
		stdin_fd = STDIN_FILENO;
		stdout_fd = STDOUT_FILENO;
		stderr_fd = STDERR_FILENO;
		int devnullfd = open(devnull.c_str(), O_RDWR);

		////////////////////////////////////////////////////////////////////////////////
		// check instream outstream conditions
		if(findandparseuserpipeCMD(parsed_line_input, senderid, recvid)){
			if(senderid != 0){ // receive message
				findit3 = checkuserpipeexists(user.userPipeManager, senderid, userid);
				if(!checkuserexist(users, senderid)){
					sendmessages(users[userid].sockfd, userpipeerrorusernotexist(senderid));
					stdin_fd = devnullfd;
				}else if(findit3 == user.userPipeManager.end()){
					sendmessages(users[userid].sockfd, userpipeerrorpipenotexist(senderid, userid));
					stdin_fd = devnullfd;
				}else{
					stdin_fd = findit3->m_pipe_read;
					instream_case = USERPIPE_CASE;
					broadcastmsg(users, userpiperecvmsg(users, senderid, userid, line_input));
				}
			}
			if(recvid != 0){ // send message
				if(recvid < MAX_USERS) findit4 = checkuserpipeexists(users[recvid].userPipeManager, userid, recvid);
				if(!checkuserexist(users, recvid)){
					sendmessages(users[userid].sockfd, userpipeerrorusernotexist(recvid));
					stdout_fd = devnullfd;
				}else if(findit4 != users[recvid].userPipeManager.end()){
					sendmessages(users[userid].sockfd, userpipeerrorpipeexist(userid, recvid));
					stdout_fd = devnullfd;
				}else{
					while(pipe(numberpipefd) < 0) { usleep(1000); }
					stdout_fd = numberpipefd[1];
					outstream_case = USERPIPE_CASE;
					broadcastmsg(users, userpipesendmsg(users, userid, recvid, line_input));
				}
			}
		}
		
		if(findandparsenumberpipeout(parsed_line_input, num)){
			findit = user.pipeManager.find(user.totalline);  // find if there exist pipes go to current line
			findit2 = user.pipeManager.find(user.totalline + num); // find if there exist pipes go to the same destination
			if(findit != user.pipeManager.end() && findit2 != user.pipeManager.end()){
				stdin_fd = findit->second.m_pipe_read;
				stdout_fd = findit2->second.m_pipe_write;
				instream_case = NUMBERPIPE_IN_CASES;
				outstream_case = NUMBERPIPE_OUT_CASE;
			}else if (findit != user.pipeManager.end()){ // there exist pipes go to current line
				while(pipe(numberpipefd) < 0) { usleep(1000); }
				stdin_fd = findit->second.m_pipe_read;
				stdout_fd = numberpipefd[1];
				instream_case = NUMBERPIPE_IN_CASES;
				outstream_case = NUMBERPIPE_OUT_CASE;
			}else if (findit2 != user.pipeManager.end()){ // there exist pipes go to the same destination
				stdout_fd = findit2->second.m_pipe_write;
				outstream_case = NUMBERPIPE_OUT_CASE;
			}else{ // none of above
				while(pipe(numberpipefd) < 0) { usleep(1000);}
				stdout_fd = numberpipefd[1];
				outstream_case = NUMBERPIPE_OUT_CASE;
			}
		}else if(findandparsenumberpipeouterr(parsed_line_input, num)){
			findit = user.pipeManager.find(user.totalline);  // find if there exist pipes go to current line
			findit2 = user.pipeManager.find(user.totalline + num); // find if there exist pipes go to the same destination
			if(findit != user.pipeManager.end() && findit2 != user.pipeManager.end()){
				stdin_fd = findit->second.m_pipe_read;
				stdout_fd = findit2->second.m_pipe_write;
				stderr_fd = findit2->second.m_pipe_write;
				instream_case = NUMBERPIPE_IN_CASES;
				outstream_case = NUMBERPIPE_OUT_ERR_CASE;
			}else if (findit != user.pipeManager.end()){ // there exist pipes go to current line
				while(pipe(numberpipefd) < 0) { usleep(1000); }
				stdin_fd = findit->second.m_pipe_read;
				stdout_fd = numberpipefd[1];
				stderr_fd = numberpipefd[1];
				instream_case = NUMBERPIPE_IN_CASES;
				outstream_case = NUMBERPIPE_OUT_ERR_CASE;
			}else if (findit2 != user.pipeManager.end()){ // there exist pipes go to the same destination
				stdout_fd = findit2->second.m_pipe_write;
				stderr_fd = findit2->second.m_pipe_write;
				outstream_case = NUMBERPIPE_OUT_ERR_CASE;
			}else{ // none of above
				while(pipe(numberpipefd) < 0) { usleep(1000);}
				stdout_fd = numberpipefd[1];
				stderr_fd = numberpipefd[1];
				outstream_case = NUMBERPIPE_OUT_ERR_CASE;
			}
		}

		if(findandparsefileoutredirect(parsed_line_input, filename)){
			stdout_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
			outstream_case = FILEOUTPUT_CASE;
		}

		if(instream_case == STDIN_CASE){
			findit = user.pipeManager.find(user.totalline);  // find if there exist pipes go to current line
			if(findit != user.pipeManager.end()){
				stdin_fd = findit->second.m_pipe_read;
				instream_case = NUMBERPIPE_IN_CASES;
			}
		}
		// end of check instream outstream conditions
		////////////////////////////////////////////////////////////////////////////////
		// execute command
		pids = ExecCMD(user.pipeManager,
					   user.totalline,
					   parsed_line_input,
					   stdin_fd,
					   stdout_fd,
					   stderr_fd);
		// end of execut command
		////////////////////////////////////////////////////////////////////////////////
		// dealing with close, wait
		if(outstream_case == FILEOUTPUT_CASE){
			close(stdout_fd);
		}
		close(devnullfd);

		if((instream_case == NUMBERPIPE_IN_CASES) && (outstream_case == NUMBERPIPE_OUT_CASE || outstream_case == NUMBERPIPE_OUT_ERR_CASE)){
			close(findit->second.m_pipe_read);
			close(findit->second.m_pipe_write);
			if(findit2 != user.pipeManager.end()){	
				for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
					findit2->second.m_wait_pids.push_front(*it);
				}
				for(auto it = pids.begin(); it != pids.end(); it++){
					findit2->second.m_wait_pids.push_back(*it);
				}
			}else{
				for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
					pids.push_front(*it);
				}
				user.pipeManager[user.totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);
			}
			user.pipeManager.erase(findit);
		}else if((instream_case == STDIN_CASE) && (outstream_case == NUMBERPIPE_OUT_CASE || outstream_case == NUMBERPIPE_OUT_ERR_CASE)){
			if (findit2 != user.pipeManager.end()){ // there exist pipes go to the same destination
				for(auto it = pids.begin(); it != pids.end(); it++){
					findit2->second.m_wait_pids.push_back(*it);
				}
			}else{
				user.pipeManager[user.totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);
			}
		}else if((instream_case == USERPIPE_CASE) && (outstream_case == NUMBERPIPE_OUT_CASE || outstream_case == NUMBERPIPE_OUT_ERR_CASE)){
			close(findit3->m_pipe_read);
			if(findit2 != user.pipeManager.end()){ // there exist pipes go to the same destination
				for(auto it = findit3->m_wait_pids.rbegin(); it != findit3->m_wait_pids.rend(); it++){
					findit2->second.m_wait_pids.push_front(*it);
				}
				for(auto it = pids.begin(); it != pids.end(); it++){
					findit2->second.m_wait_pids.push_back(*it);
				}
			}else{
				for(auto it = findit3->m_wait_pids.rbegin(); it != findit3->m_wait_pids.rend(); it++){
					pids.push_front(*it);
				}
				user.pipeManager[user.totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);
			}
			user.userPipeManager.erase(findit3);
		}else if((instream_case == USERPIPE_CASE) && (outstream_case == USERPIPE_CASE)){
			// both send and receive
			close(findit3->m_pipe_read);
			close(numberpipefd[1]);
			for(auto it = findit3->m_wait_pids.rbegin(); it!=findit3->m_wait_pids.rend(); it++){
				pids.push_front(*it);
			}
			user.userPipeManager.erase(findit3);
			users[recvid].userPipeManager.push_back(UserPipeInfo(
				numberpipefd[0],
				numberpipefd[1],
				userid,
				recvid,
				pids
			));
		}else if((instream_case == STDIN_CASE) && (outstream_case == USERPIPE_CASE)){ 
			// just send
			users[recvid].userPipeManager.push_back(UserPipeInfo(
				numberpipefd[0],
				numberpipefd[1],
				userid,
				recvid,
				pids
			));
			close(numberpipefd[1]);
		}else if((instream_case == NUMBERPIPE_IN_CASES) && (outstream_case == USERPIPE_CASE)){
			close(findit->second.m_pipe_read);
			close(findit->second.m_pipe_write);
			for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
				pids.push_front(*it);
			}
			user.pipeManager.erase(findit);
			users[recvid].userPipeManager.push_back(UserPipeInfo(
				numberpipefd[0],
				numberpipefd[1],
				userid,
				recvid,
				pids
			));
			close(numberpipefd[1]);
		}else if((instream_case == NUMBERPIPE_IN_CASES) && ((outstream_case == STDOUT_CASE) || (outstream_case == FILEOUTPUT_CASE))){
			close(findit->second.m_pipe_read);
			close(findit->second.m_pipe_write);
			for(auto its = findit->second.m_wait_pids.begin(); its != findit->second.m_wait_pids.end(); its++){
				waitpid(*its, nullptr, 0);
			}
			for(pid_t pid: pids){
				waitpid(pid, nullptr, 0);
			}
			user.pipeManager.erase(findit);
		}else if((instream_case == USERPIPE_CASE) && ((outstream_case == STDOUT_CASE) || (outstream_case == FILEOUTPUT_CASE))){
			// just receive
			close(findit3->m_pipe_read);
			for(auto its = findit3->m_wait_pids.begin(); its != findit3->m_wait_pids.end(); its++){
				waitpid(*its, nullptr, 0);
			}
			for(pid_t pid: pids){
				waitpid(pid, nullptr, 0);
			}
			user.userPipeManager.erase(findit3);
		}else if((instream_case == STDIN_CASE) && ((outstream_case == STDOUT_CASE) || (outstream_case == FILEOUTPUT_CASE))){
			// just ordinary command
			for(pid_t pid: pids){
				waitpid(pid, nullptr, 0);
			}
		}else{
			fprintf(stderr, "error cases\n");
		}
		////////////////////////////////////////////////////////////////////////////////
	}
	user.totalline++;
	return;
}
