#ifndef TBOX_EVENT_SIGNAL_EVENT_IMPL_H
#define TBOX_EVENT_SIGNAL_EVENT_IMPL_H

#include "signal_event.h"

namespace tbox {
namespace event {

class CommonLoop;

class SignalSubscribuer {
  public:
    virtual void onSignal(int signo) = 0;

  protected:
    virtual ~SignalSubscribuer() { }
};

class SignalEventImpl : public SignalEvent,
                        public SignalSubscribuer {
  public:
    explicit SignalEventImpl(CommonLoop *wp_loop, const std::string &what);
    virtual ~SignalEventImpl() override;

  public:
    virtual bool initialize(int signum, Mode mode) override;
    virtual bool initialize(const std::set<int> &sigset, Mode mode) override;
    virtual bool initialize(const std::initializer_list<int> &sigset, Mode mode) override;

    virtual void setCallback(CallbackFunc &&cb) override { cb_ = std::move(cb); }

    virtual bool isEnabled() const override { return is_enabled_; }
    virtual bool enable() override;
    virtual bool disable() override;

    virtual Loop* getLoop() const override;

  public:
    virtual void onSignal(int signo) override;

  private:
    CommonLoop *wp_loop_;
    CallbackFunc cb_;

    bool is_inited_ = false;
    bool is_enabled_ = false;

    std::set<int> sigset_;
    Mode mode_ = Mode::kPersist;

    int cb_level_ = 0;
};

}
}

#endif //TBOX_EVENT_SIGNAL_EVENT_IMPL_H_20220301
