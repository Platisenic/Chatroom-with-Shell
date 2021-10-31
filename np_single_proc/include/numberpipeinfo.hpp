#pragma once

#include <queue>
#include <unistd.h>

class NumberPipeInfo{

public:
    int m_pipe_read;
    int m_pipe_write;
    std::deque<pid_t> m_wait_pids;

    NumberPipeInfo(int w, int r, std::deque<pid_t> wp):
        m_pipe_read(w),
        m_pipe_write(r),
        m_wait_pids(wp)
        {}
    NumberPipeInfo():
        m_pipe_read(-1),
        m_pipe_write(-1),
        m_wait_pids(std::deque<pid_t>(0))
        {}
};

