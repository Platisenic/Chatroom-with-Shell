#pragma once

#include <string>
#include <map>

class UserInfo{
public:
    UserInfo() = default;
    UserInfo(int u, bool c){
        userid = u;
        sockfd = 0;
        name = "";
        ip = "";
        port = 0;
        conn = c;
        env["PATH"] = "bin:.";
    }
    void setinfo(int s, std::string n, std::string i, unsigned short p, bool c){
        sockfd = s;
        name = n;
        ip = i;
        port = p;
        conn = c;
        env.clear();
        env["PATH"] = "bin:.";
    }

    int userid;
    int sockfd;
    std::string name;
    std::string ip;
    unsigned short port;
    std::map<std::string, std::string> env;
    bool conn;
};
