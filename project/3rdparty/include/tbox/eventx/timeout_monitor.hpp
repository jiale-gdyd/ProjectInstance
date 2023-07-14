#ifndef TBOX_EVENTX_TIMEOUT_MONITOR_HPP
#define TBOX_EVENTX_TIMEOUT_MONITOR_HPP

#include <vector>
#include "../event/loop.h"
#include "../event/timer_event.h"

namespace tbox {
namespace eventx {

/**
 * 超时监控器
 *
 * 该类通常配合cabinet::Cabinet使用，实现请求池功能
 * 用于管理请求的超时自动处理功能
 */
template <typename T>
class TimeoutMonitor {
  public:
    using Duration = std::chrono::milliseconds;
    using Callback = std::function<void(const T&)>;

  public:
    explicit TimeoutMonitor(event::Loop *wp_loop);
    virtual ~TimeoutMonitor();

    /**
     * \brief   初始化
     *
     * \param   check_interval  指定检查时间间隔
     * \param   check_times     指定检查次数
     * \param   timeout_action  指定超时的动作
     *
     * \return  bool    成功与否，通常都不会失败
     * \note    尽要权衡，不要让check_times太大
     */
    bool initialize(const Duration &check_interval, int check_times);
    void setCallback(const Callback &cb) { cb_ = cb; }

    void add(const T &value);

    void cleanup();

  protected:
    void onTimerTick();

    struct PollItem {
        PollItem *next = nullptr;
        std::vector<T> items;
    };

  private:
    event::TimerEvent *sp_timer_;
    Callback    cb_;
    int         cb_level_ = 0;
    PollItem   *curr_item_ = nullptr;
    int         value_number_ = 0;
};

}
}

/// Template implementations
#include "timeout_monitor_impl.hpp"

#endif //TBOX_UTIL_TIMEOUT_MONITOR_HPP_20230218
