#ifndef TBOX_EVENTX_TIMER_POOL_H
#define TBOX_EVENTX_TIMER_POOL_H

#include <string>
#include <functional>
#include <chrono>

#include "../base/cabinet_token.h"
#include "../base/defines.h"

#include "../event/forward.h"

namespace tbox {
namespace eventx {

//! 定时任务管理器
//! 让开发者轻松创建定时任务而不必关心定时器的生命期
class TimerPool {
  public:
    using TimerToken = cabinet::Token;
    using Callback = std::function<void()>;
    using Milliseconds = std::chrono::milliseconds;
    using TimePoint = std::chrono::system_clock::time_point;

  public:
    explicit TimerPool(event::Loop *wp_loop);
    virtual ~TimerPool();

    NONCOPYABLE(TimerPool);
    IMMOVABLE(TimerPool);

  public:
    TimerToken doEvery(const Milliseconds &m_sec, Callback &&cb);
    TimerToken doAfter(const Milliseconds &m_sec, Callback &&cb);
    TimerToken doAt(const TimePoint &time_point, Callback &&cb);
    //! NOTICE:
    //! 使用时一定要小心对象生命期倒挂问题！
    //! 如果 Callback 持有了短生命期的对象，在该对象消亡时记得 cancel 该定时器

    bool cancel(const TimerToken &token);

    void cleanup();

  private:
    class Impl;
    Impl *impl_ = nullptr;
};

}
}

#endif //TBOX_EVENTX_TIMER_POOL_H_20220119
