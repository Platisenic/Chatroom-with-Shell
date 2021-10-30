#pragma once

#include <queue>
#include <unistd.h>

class NumberPipeInfo{

public:
    int m_pipe_read;
    int m_pipe_write;
    std::deque<pid_t> m_wait_pids;

    NumberPipeInfo(int, int, std::deque<pid_t>);
    NumberPipeInfo();
};

