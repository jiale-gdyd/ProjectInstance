#ifndef TBOX_TELNETD_TERMINAL_H
#define TBOX_TELNETD_TERMINAL_H

#include "terminal_interact.h"
#include "terminal_nodes.h"

namespace tbox {
namespace terminal {

class Terminal : public TerminalInteract,
                 public TerminalNodes {
  public:
    Terminal();
    virtual ~Terminal() override;

  public:
    virtual SessionToken newSession(Connection *wp_conn) override;
    virtual bool deleteSession(const SessionToken &st) override;

    virtual uint32_t getOptions(const SessionToken &st) const override;
    virtual void setOptions(const SessionToken &st, uint32_t options) override;

    virtual bool onBegin(const SessionToken &st) override;
    virtual bool onRecvString(const SessionToken &st, const std::string &str) override;
    virtual bool onRecvWindowSize(const SessionToken &st, uint16_t w, uint16_t h) override;
    virtual bool onExit(const SessionToken &st) override;

  public:
    virtual NodeToken createFuncNode(const Func &func, const std::string &help) override;
    virtual NodeToken createDirNode(const std::string &help) override;
    virtual NodeToken rootNode() const override;
    virtual NodeToken findNode(const std::string &path) const override;
    virtual bool mountNode(const NodeToken &parent, const NodeToken &child, const std::string &name) override;

  private:
    class Impl;
    Impl *impl_ = nullptr;
};

}
}

#endif //TBOX_TELNETD_TERMINAL_H_20220128
