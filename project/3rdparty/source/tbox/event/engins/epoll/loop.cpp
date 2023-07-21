#include <sys/epoll.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>

#include <vector>
#include <algorithm>

#include <tbox/event/engins/epoll/loop.h>
#include <tbox/event/engins/epoll/fd_event.h>

#include <tbox/base/log.h>
#include <tbox/base/defines.h>
#include <tbox/base/assert.h>

namespace tbox {
namespace event {

EpollLoop::EpollLoop() :
    epoll_fd_(epoll_create1(EPOLL_CLOEXEC))
{
    TBOX_ASSERT(epoll_fd_ >= 0);
}

EpollLoop::~EpollLoop()
{
    cleanupDeferredTasks();

    CHECK_CLOSE_RESET_FD(epoll_fd_);
}

void EpollLoop::runLoop(Mode mode)
{
    if (epoll_fd_ < 0)
        return;

    std::vector<struct epoll_event> events;
    /*
     * Why not events.reserve()?
     * The reserve() method only allocates memory, but leaves it uninitialized,
     * it only affects capacity(), but size() will be unchanged.
     * The standard only guarantees that std::vector::data returns a pointer and [data(), data() + size()] is a valid range,
     * the capacity is not concerned. So we need use resize and ensure the [data(), data() + size()] is a valid range whitch used by epoll_wait.
     */
    events.resize(max_loop_entries_);

    runThisBeforeLoop();

    keep_running_ = (mode == Loop::Mode::kForever);
    do {
        int fds = epoll_wait(epoll_fd_, events.data(), events.size(), getWaitTime());

        beginLoopProcess();

        handleExpiredTimers();

        for (int i = 0; i < fds; ++i) {
            epoll_event &ev = events.at(i);
            EpollFdEvent::OnEventCallback(ev.data.fd, ev.events, ev.data.ptr);
        }

        //handleRunInLoopFunc();
        handleNextFunc();

        /// If the receiver array size is full, increase its size with 1.5 times.
        if (UNLIKELY(fds >= max_loop_entries_)) {
            max_loop_entries_ = (max_loop_entries_ + max_loop_entries_ / 2);
            events.resize(max_loop_entries_);
        }

        endLoopProcess();

    } while (keep_running_);

    runThisAfterLoop();
}

EpollFdSharedData* EpollLoop::refFdSharedData(int fd)
{
    EpollFdSharedData *fd_shared_data = nullptr;

    auto it = fd_data_map_.find(fd);
    if (it != fd_data_map_.end())
        fd_shared_data = it->second;

    if (fd_shared_data == nullptr) {
        fd_shared_data = fd_shared_data_pool_.alloc();
        TBOX_ASSERT(fd_shared_data != nullptr);

        ::memset(&fd_shared_data->ev, 0, sizeof(fd_shared_data->ev));
        fd_shared_data->ev.data.ptr = static_cast<void *>(fd_shared_data);

        fd_data_map_.insert(std::make_pair(fd, fd_shared_data));
    }

    ++fd_shared_data->ref;
    return fd_shared_data;
}

void EpollLoop::unrefFdSharedData(int fd)
{
    auto it = fd_data_map_.find(fd);
    if (it != fd_data_map_.end()) {
        auto fd_shared_data = it->second;
        --fd_shared_data->ref;
        if (fd_shared_data->ref == 0) {
            fd_data_map_.erase(fd);
            fd_shared_data_pool_.free(fd_shared_data);
        }
    }
}

FdEvent* EpollLoop::newFdEvent(const std::string &what)
{
    return new EpollFdEvent(this, what);
}

}
}
