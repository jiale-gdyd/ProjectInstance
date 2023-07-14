#ifndef TBOX_ALARM_CRON_ALARM_H
#define TBOX_ALARM_CRON_ALARM_H

#include <string>

#include "alarm.h"

namespace tbox {
namespace alarm {

/*
 * @brief The linux cron alarm.
 *
 * CronAlarm allow the user to make plans by linux cron expression.
 *  Linux-cron tab expression
  *    *    *    *    *    *
  -    -    -    -    -    -
  |    |    |    |    |    |
  |    |    |    |    |    +----- day of week (0 - 7) (Sunday=0 or 7) OR sun,mon,tue,wed,thu,fri,sat
  |    |    |    |    +---------- month (1 - 12) OR jan,feb,mar,apr ...
  |    |    |    +--------------- day of month (1 - 31)
  |    |    +-------------------- hour (0 - 23)
  |    +------------------------- minute (0 - 59)
  +------------------------------ second (0 - 59)

 *
 * code example:
 *
 * Loop *loop = Loop::New();
 *
 * CronAlarm tmr(loop);
 * tmr.initialize("18 28 14 * * *"); // every day at 14:28:18
 * tmr.setCallback([] { std::cout << "timeout" << std::endl; });
 * tmr.enable();
 *
 * loop->runLoop();
 */
class CronAlarm : public Alarm
{
  public:
    explicit CronAlarm(event::Loop *wp_loop);
    virtual ~CronAlarm();

    bool initialize(const std::string &cron_expr_str);

  protected:
    virtual int calculateWaitSeconds(uint32_t curr_local_ts) override;

  private:
    void *sp_cron_expr_;
};

}
}

#endif //TBOX_ALARM_CRON_ALARM_H
