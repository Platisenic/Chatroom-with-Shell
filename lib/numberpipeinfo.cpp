#include "numberpipeinfo.hpp"

NumberPipeInfo::NumberPipeInfo(int w, int r, std::deque<int> wp):
    m_pipe_read(w),
    m_pipe_write(r),
    m_wait_pids(wp)
    {}

NumberPipeInfo::NumberPipeInfo():
    m_pipe_read(-1),
    m_pipe_write(-1),
    m_wait_pids(std::deque<pid_t>(0))
    {}

