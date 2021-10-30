#pragma once
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unistd.h>
#include "numberpipeinfo.hpp"
#include "lineinputinfo.hpp"


LineInputInfo ParseLineInput(std::string , std::vector<std::vector<std::string> > &);
void PrintENV(std::vector<std::string> &);
void SetENV(std::vector<std::string> &);
void SIGCHLD_handler(int);
std::deque<pid_t> ExecCMD(std::map<int, NumberPipeInfo> &, 
						   int,
						   std::vector<std::vector<std::string> > &,
						   int,
						   int,
						   int,
						   std::string,
						   std::map<int, NumberPipeInfo>::iterator);


