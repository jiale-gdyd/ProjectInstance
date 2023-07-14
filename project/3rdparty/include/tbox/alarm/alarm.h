#ifndef TBOX_ALARM_ALARM_H
#define TBOX_ALARM_ALARM_H

#include <cstdint>
#include <functional>
#include "../event/forward.h"

namespace tbox {
namespace alarm {

/**
 * 定时器基类
 */
class Alarm
{
  public:
    explicit Alarm(event::Loop *wp_loop);
    virtual ~Alarm();

    using Callback = std::function<void()>;
    void setCallback(const Callback &cb) { cb_ = cb; }

    /**
     * \brief 设置时区
     *
     * \param offset_minutes  相对于0时区的分钟偏移。东区为正，西区为负。
     *                        如东8区为: (8 * 60) = 480。
     *
     * \note  如果不设置，默认随系统时区
     */
    void setTimezone(int offset_minutes);

    bool isEnabled() const; //! 定时器是否已使能

    bool enable();  //! 使能定时器

    bool disable(); //! 关闭定时器

    void cleanup();

    /**
     * \brief 刷新
     *
     * 为什么需要刷新呢？
     * 有时系统时钟不准，在这种情况下 enable() 的定时任务是不准的。
     * 在时钟同步完成之后应当刷新一次
     */
    void refresh();

    //! \brief  获取剩余秒数
    uint32_t remainSeconds() const;

  protected:
    /**
     * \brief 计算下一个定时触发时间距当前的秒数
     *
     * \param curr_local_ts   当前时区的时间戳，单位：秒
     *
     * \return  >=0， 下一个定时触发时间距当前的秒数
     *          -1，  没有找到下一个定时的触发时间点
     *
     * \note  该函数为虚函数，需要由子类对实现。具体不同类型的定时器有不同的算法
     */
    virtual int calculateWaitSeconds(uint32_t curr_local_ts) = 0;

    //! 定时到期动作
    virtual void onTimeExpired();

    //! 激活定时器
    bool activeTimer();

    virtual bool onEnable() { return true; }
    virtual bool onDisable() { return true; }

  protected:
    event::Loop *wp_loop_;
    event::TimerEvent *sp_timer_ev_;

    bool using_independ_timezone_ = false;  //! 是否使用独立的时区
    int timezone_offset_seconds_ = 0;       //! 设置的时区距0时区的秒数偏移

    int cb_level_ = 0;  //!< 防回函析构计数
    Callback cb_;

    //! 状态定义
    enum class State {
      kNone,    //!< 未初始化
      kInited,  //!< 已初始化，未启动
      kRunning  //!< 已启动
    };
    State state_ = State::kNone;  //!< 当前状态

    uint32_t target_utc_ts_ = 0;
};

}
}

#endif
