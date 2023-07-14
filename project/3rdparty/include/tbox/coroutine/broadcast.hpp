#ifndef TBOX_COROUTINE_BROADCAST_HPP
#define TBOX_COROUTINE_BROADCAST_HPP

#include <vector>
#include "scheduler.h"

namespace tbox {
namespace coroutine {

//! 广播信号
class Broadcast {
  public:
    Broadcast(Scheduler &sch) : sch_(sch) { }

    bool wait() {
        wait_tokens_.push_back(sch_.getToken());
        sch_.wait();
        return !sch_.isCanceled();
    }

    void post() {
        for (auto t : wait_tokens_)
            sch_.resume(t);
        wait_tokens_.clear();
    }

  private:
    Scheduler &sch_;
    std::vector<RoutineToken> wait_tokens_;
};

}
}

#endif //TBOX_COROUTINE_BROADCAST_HPP_20180527
