#ifndef TBOX_TERMINAL_SERVICE_TELNETD_H
#define TBOX_TERMINAL_SERVICE_TELNETD_H

#include "../../event/loop.h"

namespace tbox {
namespace terminal {

class TerminalInteract;

class Telnetd {
  public:
    Telnetd(event::Loop *wp_loop, TerminalInteract *wp_terminal);
    ~Telnetd();

  public:
    bool initialize(const std::string &bind_addr);
    bool start();
    void stop();
    void cleanup();

  private:
    class Impl;
    Impl *impl_ = nullptr;
};

}
}

#endif //TBOX_TERMINAL_TELNETD_H_20220127
