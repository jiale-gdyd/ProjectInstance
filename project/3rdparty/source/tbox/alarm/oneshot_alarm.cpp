#include <tbox/alarm/oneshot_alarm.h>

#include <sys/time.h>

#include <tbox/base/log.h>
#include <tbox/event/loop.h>

namespace tbox {
namespace alarm {

namespace {
constexpr auto kSecondsOfDay = 60 * 60 * 24;
}

bool OneshotAlarm::initialize(int seconds_of_day) {
  if (state_ == State::kRunning) {
    LogWarn("alarm is running state, disable first");
    return false;
  }

  if (seconds_of_day < 0 || seconds_of_day >= kSecondsOfDay) {
    LogWarn("seconds_of_day:%d, out of range: [0,%d)", seconds_of_day, kSecondsOfDay);
    return false;
  }

  seconds_of_day_ = seconds_of_day;
  state_ = State::kInited;
  return true;
}

int OneshotAlarm::calculateWaitSeconds(uint32_t curr_local_ts) {
  int curr_seconds = curr_local_ts % kSecondsOfDay;

#if 1
  LogTrace("curr_seconds:%d", curr_seconds);
#endif

  int wait_seconds = seconds_of_day_ - curr_seconds;
  if (wait_seconds <= 0)
    wait_seconds += kSecondsOfDay;
  return wait_seconds;
}

void OneshotAlarm::onTimeExpired() {
  state_ = State::kInited;

  ++cb_level_;
  if (cb_)
    cb_();
  --cb_level_;
}

}
}
