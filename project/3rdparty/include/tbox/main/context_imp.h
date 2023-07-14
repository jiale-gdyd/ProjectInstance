#ifndef TBOX_MAIN_CONTEXT_IMP_H
#define TBOX_MAIN_CONTEXT_IMP_H

#include "context.h"

#include "../base/json_fwd.h"
#include "../terminal/terminal.h"
#include "../terminal/service/telnetd.h"
#include "../terminal/service/tcp_rpc.h"

namespace tbox {
namespace main {

//! 进程上下文
class ContextImp : public Context {
  public:
    ContextImp();
    ~ContextImp();

    void fillDefaultConfig(Json &cfg) const;

    bool initialize(const Json &cfg);
    bool start();
    void stop();
    void cleanup();

  public:
    virtual event::Loop* loop() const override { return sp_loop_; }
    virtual eventx::ThreadPool* thread_pool() const override { return sp_thread_pool_; }
    virtual eventx::TimerPool* timer_pool() const override { return sp_timer_pool_; }
    virtual eventx::Async* async() const override { return sp_async_; }
    virtual terminal::TerminalNodes* terminal() const override { return sp_terminal_; }

    virtual std::chrono::milliseconds running_time() const override;
    virtual std::chrono::system_clock::time_point start_time_point() const override;

  protected:
    bool initLoop(const Json &js);
    bool initThreadPool(const Json &js);
    bool initTelnetd(const Json &js);
    bool initRpc(const Json &js);
    void initShell();

  private:
    event::Loop *sp_loop_ = nullptr;
    eventx::ThreadPool *sp_thread_pool_ = nullptr;
    eventx::TimerPool  *sp_timer_pool_ = nullptr;
    eventx::Async      *sp_async_ = nullptr;

    terminal::Terminal *sp_terminal_ = nullptr;
    terminal::Telnetd  *sp_telnetd_ = nullptr;
    terminal::TcpRpc   *sp_tcp_rpc_ = nullptr;
    bool telnetd_init_ok = false;
    bool tcp_rpc_init_ok = false;

    std::chrono::steady_clock::time_point start_time_point_;
};

}
}

#endif //TBOX_MAIN_CONTEXT_IMP_H_20220116
