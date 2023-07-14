#ifndef TBOX_TERMINAL_INTERACT_H
#define TBOX_TERMINAL_INTERACT_H

#include "types.h"

namespace tbox {
namespace terminal {

class Connection;

class TerminalInteract {
  public:
    virtual SessionToken newSession(Connection *wp_conn) = 0;
    virtual bool deleteSession(const SessionToken &st) = 0;

    enum Option : uint32_t {
        kEnableEcho = 0x01,
        kQuietMode  = 0x02, //! 安静模式，不主动打印提示信息
    };
    virtual uint32_t getOptions(const SessionToken &st) const = 0;
    virtual void setOptions(const SessionToken &st, uint32_t options) = 0;

    virtual bool onBegin(const SessionToken &st) = 0;
    virtual bool onExit(const SessionToken &st) = 0;
    virtual bool onRecvString(const SessionToken &st, const std::string &str) = 0;
    virtual bool onRecvWindowSize(const SessionToken &st, uint16_t w, uint16_t h) = 0;

  protected:
    virtual ~TerminalInteract() { }
};

}
}

#endif //TBOX_TERMINAL_INTERACT_H_20220214
