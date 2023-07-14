#ifndef TBOX_MAIN_LOG_H
#define TBOX_MAIN_LOG_H

#include "../base/json_fwd.h"

#include "../log/async_stdout_sink.h"
#include "../log/async_syslog_sink.h"
#include "../log/async_file_sink.h"

#include "../terminal/terminal_nodes.h"

#include "context.h"

namespace tbox {
namespace main {

class Log {
  public:
    void fillDefaultConfig(Json &cfg) const;
    bool initialize(const char *proc_name, Context &ctx, const Json &cfg);
    void cleanup();

  protected:
    void initSink(const Json &js, log::Sink &ch);
    void initShell(terminal::TerminalNodes &term);
    void initShellForSink(log::Sink &log_ch, terminal::TerminalNodes &term, terminal::NodeToken dir_node);
    void initShellForAsyncFileSink(terminal::TerminalNodes &term, terminal::NodeToken dir_node);

  private:
    log::AsyncStdoutSink async_stdout_sink_;
    log::AsyncSyslogSink async_syslog_sink_;
    log::AsyncFileSink   async_file_sink_;
};

}
}

#endif //TBOX_MAIN_LOG_H_20220414
