#ifndef XOP_EPOLL_TASK_SCHEDULER_H
#define XOP_EPOLL_TASK_SCHEDULER_H

#include <mutex>
#include <unordered_map>
#include "TaskScheduler.h"

namespace xop {
class EpollTaskScheduler : public TaskScheduler {
public:
    EpollTaskScheduler(int id = 0);
    virtual ~EpollTaskScheduler();

    void UpdateChannel(ChannelPtr channel);
    void RemoveChannel(ChannelPtr &channel);

    bool HandleEvent(int timeout);

private:
    void Update(int operation, ChannelPtr &channel);

    int                                 epollfd_ = -1;
    std::mutex                          mutex_;
    std::unordered_map<int, ChannelPtr> channels_;
};
}

#endif
