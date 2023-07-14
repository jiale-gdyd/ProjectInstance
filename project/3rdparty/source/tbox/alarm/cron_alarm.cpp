#include <tbox/alarm/cron_alarm.h>

#include <cstring>

#include <tbox/base/log.h>
#include <tbox/base/assert.h>
#include <tbox/event/loop.h>

#include <tbox/alarm/ccronexpr.h>

namespace tbox {
namespace alarm {

CronAlarm::CronAlarm(event::Loop *loop) :
  Alarm(loop),
  sp_cron_expr_(malloc(sizeof(cron_expr)))
{
  TBOX_ASSERT(sp_cron_expr_ != nullptr);
  memset(sp_cron_expr_, 0, sizeof(cron_expr));
}

CronAlarm::~CronAlarm()
{
  free(sp_cron_expr_);
}

bool CronAlarm::initialize(const std::string &cron_expr_str)
{
  if (state_ == State::kRunning) {
    LogWarn("alarm is running state, disable first");
    return false;
  }

  const char *error_str = nullptr;
  memset(sp_cron_expr_, 0, sizeof(cron_expr));

  // check validity of cron str
  cron_parse_expr(cron_expr_str.c_str(), static_cast<cron_expr *>(sp_cron_expr_), &error_str);
  if (error_str != nullptr) { // Invalid expression.
    LogWarn("cron_expr error: %s", error_str);
    return false;
  }

  state_ = State::kInited;
  return true;
}

int CronAlarm::calculateWaitSeconds(uint32_t curr_local_ts) {
  auto next_local_ts = cron_next(static_cast<cron_expr *>(sp_cron_expr_), curr_local_ts);
  return next_local_ts - curr_local_ts;
}

}
}
