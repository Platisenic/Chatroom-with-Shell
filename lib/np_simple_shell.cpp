#include "np_simple_shell.hpp"

void simple_shell(){
	std::string line_input;
	int numberpipefd[2];
	std::vector<std::vector<std::string> > parsed_line_input;
	std::map<int, NumberPipeInfo> pipeManager;
    std::map<int, NumberPipeInfo>::iterator findit;
	std::map<int, NumberPipeInfo>::iterator findit2;
	std::deque<pid_t> pids;
	int totalline = 0;
	int stderr_fd = STDERR_FILENO;
	setenv("PATH", "bin:.", 1);

	while(true){
		// print prompt and get input from user
		fprintf(stdout, "%% ");
		std::getline(std::cin, line_input);

		if(std::cin.eof()) { fprintf(stdout, "\n"); return; }
		size_t countspace = 0;
		for(auto li: line_input){
			if(li == ' ') countspace++;
		}
		if(countspace == line_input.size() || (countspace == line_input.size() - 1 && line_input.back() == '\r')) continue;
		parsed_line_input.clear();
		LineInputInfo parse_status = ParseLineInput(line_input, parsed_line_input);
		// accroding to parse_status, handle the cases
		switch(parse_status.cases){
			case CASES::PRINTENV:
				PrintENV(parsed_line_input.front());
				break;
			case CASES::SETENV:
				SetENV(parsed_line_input.front());
				break;
			case CASES::EXIT:
				return;
			case CASES::STDOUTNUMBERPIPE:
			case CASES::STDOUTSTDERRNUMBERPIPE:
				findit = pipeManager.find(totalline);  // find if there exist pipes go to current line
				findit2 = pipeManager.find(totalline + parse_status.number); // find if there exist pipes go to the same destination
				if(findit != pipeManager.end() && findit2 != pipeManager.end()){
					stderr_fd = (parse_status.cases == CASES::STDOUTSTDERRNUMBERPIPE) ? findit2->second.m_pipe_write: STDERR_FILENO;
					pids = ExecCMD(pipeManager,
							   	   totalline,
							   	   parsed_line_input,
							   	   findit->second.m_pipe_read,
							   	   findit2->second.m_pipe_write,
							   	   stderr_fd,
							   	   "",
								   findit);
					for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
						findit2->second.m_wait_pids.push_front(*it);
					}
					for(auto it = pids.begin(); it != pids.end(); it++){
						findit2->second.m_wait_pids.push_back(*it);
					}
					pipeManager.erase(findit);

				}else if (findit != pipeManager.end()){ // there exist pipes go to current line
					while(pipe(numberpipefd) < 0){
						// fprintf(stderr, "%s\n", strerror(errno));
						usleep(1000);
					}
					stderr_fd = (parse_status.cases == CASES::STDOUTSTDERRNUMBERPIPE) ? numberpipefd[1]: STDERR_FILENO;
					pids = ExecCMD(pipeManager,
							   	   totalline,
							   	   parsed_line_input,
							   	   findit->second.m_pipe_read,
							   	   numberpipefd[1],
							   	   stderr_fd,
							   	   "",
								   findit);
					for(auto it = findit->second.m_wait_pids.rbegin(); it != findit->second.m_wait_pids.rend(); it++){
						pids.push_front(*it);
					}
					pipeManager.erase(findit);
					pipeManager[totalline + parse_status.number] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);

				}else if (findit2 != pipeManager.end()){ // there exist pipes go to the same destination
					stderr_fd = (parse_status.cases == CASES::STDOUTSTDERRNUMBERPIPE) ? findit2->second.m_pipe_write : STDERR_FILENO;
					pids = ExecCMD(pipeManager,
								   totalline,
								   parsed_line_input,
								   STDIN_FILENO,
								   findit2->second.m_pipe_write,
								   stderr_fd,
								   "",
								   findit);
					for(auto it = pids.begin(); it != pids.end(); it++){
						findit2->second.m_wait_pids.push_back(*it);
					}
				}else{ // none of above
					while(pipe(numberpipefd) < 0) { usleep(1000); }
					stderr_fd = (parse_status.cases == CASES::STDOUTSTDERRNUMBERPIPE) ? numberpipefd[1] : STDERR_FILENO;
					pids = ExecCMD(pipeManager,
							   	   totalline,
							   	   parsed_line_input,
							   	   STDIN_FILENO,
							   	   numberpipefd[1],
							   	   stderr_fd,
							   	   "",
								   findit);
					pipeManager[totalline + parse_status.number] = NumberPipeInfo(numberpipefd[0], numberpipefd[1], pids);
				}
				break;
			case CASES::ORDINARYPIPEORNOPIPE:
			case CASES::FILEOUTPUTREDIRECT:
				findit = pipeManager.find(totalline);
				if(findit != pipeManager.end()){
					ExecCMD(pipeManager,
							  totalline,
							  parsed_line_input,
							  findit->second.m_pipe_read,
							  STDOUT_FILENO,
							  STDERR_FILENO,
							  parse_status.fileName,
							  findit);
					pipeManager.erase(findit);
				}else{
					ExecCMD(pipeManager,
							  totalline,
							  parsed_line_input,
							  STDIN_FILENO,
							  STDOUT_FILENO,
							  STDERR_FILENO,
							  parse_status.fileName,
							  findit);
				}
				break;
		}
		// total line add by 1
		++totalline;
	}
	
	return;
}
