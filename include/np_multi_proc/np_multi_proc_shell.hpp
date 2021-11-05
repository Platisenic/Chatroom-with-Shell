#pragma once
#include "np_multi_proc/utils.hpp"
#include "np_multi_proc/numberpipeinfo.hpp"

#include <queue>
#include <iostream>
#include <map>

#include <signal.h>
#include <string.h>
#include <unistd.h>

void shell(ShareMemory* shmaddr, int userid, int serverlogfd){
	std::string line_input;
	int numberpipefd[2];
	std::vector<std::vector<std::string> > parsed_line_input;
	std::map<int, NumberPipeInfo> pipeManager;
    std::map<int, NumberPipeInfo>::iterator findit;
	std::map<int, NumberPipeInfo>::iterator findit2;
	std::deque<pid_t> pids;
	int stdin_fd = STDIN_FILENO;
	int stdout_fd = STDOUT_FILENO;
	int stderr_fd = STDERR_FILENO;
	int num = 0;
	std::string filename = "";
	int totalline = 0;

	setenv("PATH", "bin:.", 1);

	while(true){
		// print prompt and get input from user
		fprintf(stdout, "%% ");
		std::getline(std::cin, line_input);
		// dprintf(serverlogfd, "line_input: %s\n", line_input.c_str());
		if(std::cin.eof()) { fprintf(stdout, "\n"); return; }
		if(line_input.back() == '\r') line_input.pop_back();
		size_t countspace = 0;
		for(auto li: line_input){
			if(li == ' ') countspace++;
		}
		if (countspace == line_input.size()) { continue; }

		parsed_line_input.clear();
		ParseLineInput(line_input, parsed_line_input);
		std::string first = parsed_line_input.front().front();

		if(first == "printenv"){
			PrintENV(parsed_line_input.front());
		}else if(first == "setenv"){
			SetENV(parsed_line_input.front());
		}else if(first == "exit"){
			lock(semid);
			shmaddr->users[userid].conn = false;
			unlock(semid);
			return;
		}else if(first == "who"){
			who(shmaddr, userid);
		}else if(first == "tell"){ // TODO Handle Error
			if(parsed_line_input.front().size() >= 2){
				size_t found = line_input.find(parsed_line_input.front()[1]);
				found += parsed_line_input.front()[1].size() + 1;
				std::string tellmsg = line_input.substr(found);
				tell(shmaddr, userid, std::stoi(parsed_line_input.front()[1]), tellmsg);
			}
		}else if(first == "yell"){ // TODO Handle Error
			size_t found = line_input.find(parsed_line_input.front()[0]);
			found += parsed_line_input.front()[0].size() + 1;
			std::string yellmsg = line_input.substr(found);
			yell(shmaddr, userid, yellmsg);
		}else if(first == "name"){
			if(parsed_line_input.front().size() == 2){
				changename(shmaddr, userid, parsed_line_input.front()[1]);
			}
		}else{
			enum::CASES instream_case = STDIN_CASE;
			enum::CASES outstream_case = STDOUT_CASE;
			stdin_fd = STDIN_FILENO;
			stdout_fd = STDOUT_FILENO;
			stderr_fd = STDERR_FILENO;

			////////////////////////////////////////////////////////////////////////////////
			// check instream outstream conditions
			if(findandparsenumberpipeout(parsed_line_input, num)){
				findit = pipeManager.find(totalline);  // find if there exist pipes go to current line
				findit2 = pipeManager.find(totalline + num); // find if there exist pipes go to the same destination
				if(findit != pipeManager.end() && findit2 != pipeManager.end()){
					stdin_fd = findit->second.m_pipe_read;
					stdout_fd = findit2->second.m_pipe_write;
					instream_case = NUMBERPIPE_IN_CASES;
					outstream_case = NUMBERPIPE_OUT_CASE;
				}else if (findit != pipeManager.end()){ // there exist pipes go to current line
					while(pipe(numberpipefd) < 0) { usleep(1000); }
					stdin_fd = findit->second.m_pipe_read;
					stdout_fd = numberpipefd[1];
					instream_case = NUMBERPIPE_IN_CASES;
					outstream_case = NUMBERPIPE_OUT_CASE;
				}else if (findit2 != pipeManager.end()){ // there exist pipes go to the same destination
					stdout_fd = findit2->second.m_pipe_write;
					outstream_case = NUMBERPIPE_OUT_CASE;
				}else{ // none of above
					while(pipe(numberpipefd) < 0) { usleep(1000);}
					stdout_fd = numberpipefd[1];
					outstream_case = NUMBERPIPE_OUT_CASE;
				}
			}else if(findandparsenumberpipeouterr(parsed_line_input, num)){
				findit = pipeManager.find(totalline);  // find if there exist pipes go to current line
				findit2 = pipeManager.find(totalline + num); // find if there exist pipes go to the same destination
				if(findit != pipeManager.end() && findit2 != pipeManager.end()){
					stdin_fd = findit->second.m_pipe_read;
					stdout_fd = findit2->second.m_pipe_write;
					stderr_fd = findit2->second.m_pipe_write;
					instream_case = NUMBERPIPE_IN_CASES;
					outstream_case = NUMBERPIPE_OUT_ERR_CASE;
				}else if (findit != pipeManager.end()){ // there exist pipes go to current line
					while(pipe(numberpipefd) < 0) { usleep(1000); }
					stdin_fd = findit->second.m_pipe_read;
					stdout_fd = numberpipefd[1];
					stderr_fd = numberpipefd[1];
					instream_case = NUMBERPIPE_IN_CASES;
					outstream_case = NUMBERPIPE_OUT_ERR_CASE;
				}else if (findit2 != pipeManager.end()){ // there exist pipes go to the same destination
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
				findit = pipeManager.find(totalline);  // find if there exist pipes go to current line
				if(findit != pipeManager.end()){
					stdin_fd = findit->second.m_pipe_read;
					instream_case = NUMBERPIPE_IN_CASES;
				}
			}
			// end of check instream outstream conditions
			////////////////////////////////////////////////////////////////////////////////
			// execute command
			pids = ExecCMD (pipeManager,
							totalline,
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
			if((instream_case == NUMBERPIPE_IN_CASES) && (outstream_case == NUMBERPIPE_OUT_CASE || outstream_case == NUMBERPIPE_OUT_ERR_CASE)){
				close(findit->second.m_pipe_read);
				close(findit->second.m_pipe_write);
				if(findit2 != pipeManager.end()){	
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
					pipeManager[totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);
				}
				pipeManager.erase(findit);
			}else if((instream_case == STDIN_CASE) && (outstream_case == NUMBERPIPE_OUT_CASE || outstream_case == NUMBERPIPE_OUT_ERR_CASE)){
				if (findit2 !=pipeManager.end()){ // there exist pipes go to the same destination
					for(auto it = pids.begin(); it != pids.end(); it++){
						findit2->second.m_wait_pids.push_back(*it);
					}
				}else{
					pipeManager[totalline + num] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);
				}
			}else if((instream_case == NUMBERPIPE_IN_CASES) && ((outstream_case == STDOUT_CASE) || (outstream_case == FILEOUTPUT_CASE))){
				close(findit->second.m_pipe_read);
				close(findit->second.m_pipe_write);
				for(auto its = findit->second.m_wait_pids.begin(); its != findit->second.m_wait_pids.end(); its++){
					waitpid(*its, nullptr, 0);
				}
				for(pid_t pid: pids){
					waitpid(pid, nullptr, 0);
				}
				pipeManager.erase(findit);
			}else if((instream_case == STDIN_CASE) && ((outstream_case == STDOUT_CASE) || (outstream_case == FILEOUTPUT_CASE))){
				// just ordinary command
				for(pid_t pid: pids){
					waitpid(pid, nullptr, 0);
				}
			}else{
				dprintf(serverlogfd, "error cases\n");
			}
			////////////////////////////////////////////////////////////////////////////////
			// end of dealing with close, wait
		}

		totalline++;
	}
	
	return;
}

