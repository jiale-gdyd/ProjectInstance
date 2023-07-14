#include <tbox/eventx/timer_fd.h>

#include <unistd.h>
#include <sys/timerfd.h>

#include <tbox/base/log.h>
#include <tbox/base/assert.h>
#include <tbox/base/defines.h>
#include <tbox/event/loop.h>
#include <tbox/event/fd_event.h>

namespace tbox {
namespace eventx {

namespace {
constexpr auto NANOSECS_PER_SECOND = 1000000000ul;
}

#ifndef TFD_TIMER_CANCEL_ON_SET
#define TFD_TIMER_CANCEL_ON_SET (1 << 1)
#endif

struct TimerFd::Data {
    Callback cb;
    tbox::event::Loop *loop { nullptr };
    tbox::event::FdEvent *timer_fd_event { nullptr };
    int timer_fd { -1 };
    bool is_inited { false };
    bool is_enabled { false };
    struct itimerspec ts { 0 };
    int cb_level {0};
};

TimerFd::TimerFd(tbox::event::Loop *loop, const std::string &what)
    : d_(new Data)
{
    d_->loop = loop;
    d_->timer_fd_event = loop->newFdEvent(what);
    d_->timer_fd_event->setCallback(std::bind(&TimerFd::onEvent, this, std::placeholders::_1));
}

TimerFd::~TimerFd()
{
    TBOX_ASSERT(d_->cb_level == 0);

    cleanup();
    CHECK_DELETE_RESET_OBJ(d_->timer_fd_event);
    CHECK_DELETE_RESET_OBJ(d_);
}

bool TimerFd::initialize(const std::chrono::nanoseconds first, 
                         const std::chrono::nanoseconds repeat)
{
    cleanup();

    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd < 0)
        return false;

    d_->timer_fd = timer_fd;

    auto first_nanosec = first.count();
    auto repeat_nanosec = repeat.count();

    d_->ts.it_value.tv_sec = first_nanosec / NANOSECS_PER_SECOND;
    d_->ts.it_value.tv_nsec = first_nanosec % NANOSECS_PER_SECOND;
    d_->ts.it_interval.tv_sec = repeat_nanosec / NANOSECS_PER_SECOND; 
    d_->ts.it_interval.tv_nsec = repeat_nanosec % NANOSECS_PER_SECOND;

    auto mode = repeat_nanosec != 0 ? event::Event::Mode::kPersist : event::Event::Mode::kOneshot;

    d_->timer_fd_event->initialize(d_->timer_fd, tbox::event::FdEvent::kReadEvent, mode);

    d_->is_inited = true;
    return true;
}

void TimerFd::cleanup()
{
    if (!d_->is_inited)
        return;

    disable();
    CHECK_CLOSE_RESET_FD(d_->timer_fd);
    d_->cb = nullptr;
    d_->is_inited = false;
}

bool TimerFd::isEnabled() const
{
    return d_->is_enabled;
}

bool TimerFd::enable()
{
    if (!d_->is_inited)
        return false;

    if (d_->is_enabled)
        return true;

    if (!d_->timer_fd_event->enable())
        return false;

    if (timerfd_settime(d_->timer_fd, TFD_TIMER_CANCEL_ON_SET, &d_->ts, NULL) < 0) {
        LogWarn("timerfd_settime() failed: errno=%d", errno);
        return false;
    }

    d_->is_enabled = true;
    return true;
}

bool TimerFd::disable()
{
    if (!d_->is_inited)
        return false;

    if (!d_->is_enabled)
        return true;

    struct itimerspec ts = {0};
    if (timerfd_settime(d_->timer_fd, TFD_TIMER_CANCEL_ON_SET, &ts, NULL) < 0) {
        LogWarn("timerfd_settime() failed: errno=%d", errno);
        return false;
    }

    if (!d_->timer_fd_event->disable())
        return false;

    d_->is_enabled = false;
    return true;
}

std::chrono::nanoseconds TimerFd::remainTime() const
{
    std::chrono::nanoseconds remain_time = std::chrono::nanoseconds::zero();

    struct itimerspec ts;
    int ret = ::timerfd_gettime(d_->timer_fd, &ts);
    if (ret == 0) {
        remain_time += std::chrono::seconds(ts.it_value.tv_sec);
        remain_time += std::chrono::nanoseconds(ts.it_value.tv_nsec);
    } else {
        LogErr("timerfd_gettime() fail, ret: %d", ret);
    }

    return remain_time;
}

void TimerFd::onEvent(short events)
{
    if (events & tbox::event::FdEvent::kReadEvent) {
        uint64_t exp = 0;
        int len  = ::read(d_->timer_fd, &exp, sizeof(exp));
        if (len <= 0)
            return;

        if (d_->cb) {
            ++d_->cb_level;
            d_->cb();
            --d_->cb_level;
        }
    }
}

void TimerFd::setCallback(Callback &&cb)
{
    d_->cb = std::move(cb);
}

}
}
