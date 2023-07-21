#ifndef TBOX_ALARM_WORKDAY_ALARM_H
#define TBOX_ALARM_WORKDAY_ALARM_H

#include "alarm.h"
#include "workday_calendar.h"

namespace tbox {
namespace alarm {

/**
 * \brief 工作日、节假日定时器
 */
class WorkdayAlarm : public Alarm
{
  public:
    using Alarm::Alarm;

    /**
     * \brief 初始化
     *
     * \param seconds_of_day  从本地00:00起到触发时间的秒数
     *                        如：08:30 = 8 * 3600 + 30 * 60 = 30600
     * \param wp_calendar     工作日日历对象
     * \param workday         true:工作日执行，false:在节假日执行
     *
     * \return  true    成功
     *          false   失败
     *
     * \warnning  必须要确保 wp_calendar 指向的日历对象的生命期比本 WorkdayAlarm 长
     *            否则会出异常
     */
    bool initialize(int seconds_of_day, WorkdayCalendar *wp_calendar, bool workday);

  protected:
    virtual int calculateWaitSeconds(uint32_t curr_local_ts) override;
    virtual bool onEnable() override;
    virtual bool onDisable() override;

  private:
    int seconds_of_day_ = 0;
    WorkdayCalendar *wp_calendar_ = nullptr;
    bool workday_ = true;
};

}
}

#endif //TBOX_ALARM_WORKDAY_ALARM_H_20221024
