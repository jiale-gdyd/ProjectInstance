#ifndef TBOX_ALARM_WEEKLY_ALARM_H
#define TBOX_ALARM_WEEKLY_ALARM_H

#include <string>

#include "alarm.h"

namespace tbox {
namespace alarm {

/*
 * @brief The weekly alarm.
 *
 * WeeklyAlarm allow the user to make plans weekly.
 *
 * code example:
 *
 * Loop *loop = Loop::New();
 *
 * WeeklyAlarm tmr(loop);
 * tmr.initialize(30600, "1111111"); // everyday at 08:30
 * tmr.setCallback([] { std::cout << "time is up" << endl;});
 * tmr.enable();
 *
 * loop->runLoop();
 */

class WeeklyAlarm : public Alarm
{
  public:
    using Alarm::Alarm;

    /**
     * \brief 初始化
     *
     * \param seconds_of_day  从本地00:00起到触发时间的秒数
     *                        如：08:30 = 8 * 3600 + 30 * 60 = 30600
     *
     * \param week_mask       星期掩码，指定一个星期中哪几天可以执行定时任务，固定长度7个字符。
     *                        星期日开始。对应要执行的标记字符'1'，其它表示不执行。
     *                        比如：某任务需要在周一到周五执行，星期掩码则为："0111110"
     */
    bool initialize(int seconds_of_day, const std::string &week_mask);

  protected:
    virtual int calculateWaitSeconds(uint32_t curr_local_ts) override;

  private:
    int seconds_of_day_ = 0;
    uint8_t week_mask_ = 0;
};

}
}

#endif //TBOX_ALARM_WEEKLY_ALARM_H_20221019
