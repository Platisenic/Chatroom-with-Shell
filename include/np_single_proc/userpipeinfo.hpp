#pragma once

#include <queue>
#include <unistd.h>

class UserPipeInfo{

public:
    int m_pipe_read;
    int m_pipe_write;
    int send_userid;
    int recv_userid;
    std::deque<pid_t> m_wait_pids;

    UserPipeInfo(int r, int w, int su, int ru, std::deque<pid_t> wp):
        m_pipe_read(r),
        m_pipe_write(w),
        send_userid(su),
        recv_userid(ru),
        m_wait_pids(wp)
        {}
    UserPipeInfo():
        m_pipe_read(-1),
        m_pipe_write(-1),
        send_userid(-1),
        recv_userid(-1),
        m_wait_pids(std::deque<pid_t>(0))
        {}
};

