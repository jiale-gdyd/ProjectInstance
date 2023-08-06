#ifndef TBOX_BASE_BACKTRACE_H
#define TBOX_BASE_BACKTRACE_H

#include <string>
#include "log.h"

namespace tbox {

/// 导出调用栈
std::string DumpBacktrace(const unsigned int max_frames = 64);

}

#define LogBacktrace(level) LogPrintf((level), "call stack:\n%s", tbox::DumpBacktrace().c_str())

#endif // TBOX_BACKTRACE_H_20220708
