#ifndef TBOX_EVENT_ENGINS_EPOLL_TYPES_H
#define TBOX_EVENT_ENGINS_EPOLL_TYPES_H

#include <sys/epoll.h>
#include <vector>

namespace tbox {
namespace event {

class EpollFdEvent;

//! 同一个fd共享的数据
struct EpollFdSharedData {
    int ref = 0;    //! 引用计数
    struct epoll_event ev;
    std::vector<EpollFdEvent*> read_events;
    std::vector<EpollFdEvent*> write_events;
    std::vector<EpollFdEvent*> exception_events;
};

}
}

#endif