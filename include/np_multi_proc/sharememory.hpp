#pragma once

#include "sys/shm.h"
#define MAX_USERS 40
#define MAX_MSG_NUM 100 //TODO
#define MAX_MSG_SIZE 1500

struct UserInfo{
    pid_t pid;
    int userid;
    char name[50];
    char ip[50];
    unsigned short port;
    bool conn;
    char msgbuffer[MAX_MSG_NUM][MAX_MSG_SIZE];
    int readEnd;
    int writeEnd;
    
};

struct ShareMemory{
    UserInfo users[MAX_USERS];
};