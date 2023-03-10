#include <errno.h>
#include <sys/epoll.h>
#include "EpollTaskScheduler.h"

namespace xop {
EpollTaskScheduler::EpollTaskScheduler(int id) : TaskScheduler(id)
{
    epollfd_ = epoll_create(1024);
    this->UpdateChannel(wakeup_channel_);
}

EpollTaskScheduler::~EpollTaskScheduler()
{
    if (epollfd_ >= 0) {
        close(epollfd_);
        epollfd_ = -1;
    }
}

void EpollTaskScheduler::UpdateChannel(ChannelPtr channel)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int fd = channel->GetSocket();
    if (channels_.find(fd) != channels_.end()) {
        if (channel->IsNoneEvent()) {
            Update(EPOLL_CTL_DEL, channel);
            channels_.erase(fd);
        } else {
            Update(EPOLL_CTL_MOD, channel);
        }
    } else {
        if (!channel->IsNoneEvent()) {
            channels_.emplace(fd, channel);
            Update(EPOLL_CTL_ADD, channel);
        }
    }
}

void EpollTaskScheduler::Update(int operation, ChannelPtr &channel)
{
    struct epoll_event event = {0};

    if (operation != EPOLL_CTL_DEL) {
        event.data.ptr = channel.get();
        event.events = channel->GetEvents();
    }

    if (::epoll_ctl(epollfd_, operation, channel->GetSocket(), &event) < 0) {

    }
}

void EpollTaskScheduler::RemoveChannel(ChannelPtr &channel)
{
    std::lock_guard<std::mutex> lock(mutex_);

    int fd = channel->GetSocket();
    if (channels_.find(fd) != channels_.end()) {
        Update(EPOLL_CTL_DEL, channel);
        channels_.erase(fd);
    }
}

bool EpollTaskScheduler::HandleEvent(int timeout)
{
    int num_events = -1;
    struct epoll_event events[512] = {0};

    num_events = epoll_wait(epollfd_, events, 512, timeout);
    if (num_events < 0)  {
        if (errno != EINTR) {
            return false;
        }
    }

    for (int n = 0; n < num_events; n++) {
        if (events[n].data.ptr) {
            ((Channel *)events[n].data.ptr)->HandleEvent(events[n].events);
        }
    }

    return true;
}
}
