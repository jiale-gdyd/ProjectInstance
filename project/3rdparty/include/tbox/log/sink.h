#ifndef TBOX_LOG_SINK_H
#define TBOX_LOG_SINK_H

#include <map>
#include <string>
#include <mutex>
#include "../base/log.h"
#include "../base/log_imp.h"

#define TIMESTAMP_STRING_SIZE   15

namespace tbox {
namespace log {

//! 日志打印通道类
class Sink {
  public:
    virtual ~Sink();

    void setLevel(int level);
    void setLevel(const std::string &module, int level);
    void unsetLevel(const std::string &module);

    void enableColor(bool enable);

    bool enable();
    void disable();

  protected:
    virtual void onEnable() { }
    virtual void onDisable() { }

    virtual void onLogFrontEnd(const LogContent *content) = 0;

    void handleLog(const LogContent *content);

    static void HandleLog(const LogContent *content, void *ptr);
    bool filter(int level, const std::string &module);

  protected:
    bool enable_color_ = false;

  private:
    std::mutex lock_;

    uint32_t output_id_ = 0;
    std::map<std::string, int> modules_level_;
    int default_level_ = LOG_LEVEL_INFO;
};

}
}

#endif //TBOX_LOG_SINK_H_20220406
