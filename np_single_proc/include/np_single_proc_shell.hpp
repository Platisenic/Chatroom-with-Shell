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
    UserInfo &user = users[userid];
	clearenv();
	for(auto env : user.env){
        setenv(env.first.c_str(), env.second.c_str(), 1);
    }
	std::string line_input;
	std::vector<std::vector<std::string> > parsed_line_input;
	std::getline(std::cin, line_input);

	if(std::cin.eof()) { fprintf(stdout, "\n"); return;}
	size_t countspace = 0;
	for(auto li: line_input){
		if(li == ' ') countspace++;
	}
	if(countspace == line_input.size() || (countspace == line_input.size() - 1 && line_input.back() == '\r')) { return; }
	ParseLineInput(line_input, parsed_line_input);

	std::string first = parsed_line_input.front().front();
	if(first == "printenv"){
		PrintENV(parsed_line_input.front());
	}else if(first == "setenv"){
		SetENV(parsed_line_input.front(), user);
	}else if(first == "exit"){
		user.conn = false;
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
	}else{
		
	}
	
	return;
}
