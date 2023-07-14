/**
 * 定义宏 TBOX_ASSERT(), 令其它错误信息打印到Log日志上
 */
#ifndef TBOX_BASE_ASSERT_H
#define TBOX_BASE_ASSERT_H

#include <cstdlib>
#include "log.h"

//! WARN: Don't use this in log related module

#ifdef  NDEBUG //! 在非调试模式下，什么都不用做
# define TBOX_ASSERT(expr) void(0)
#else //! 在调试模式下，要在日志中打印错误并退出
# define TBOX_ASSERT(expr) \
  if (!(expr)) { \
    LogFatal("TBOX_ASSERT(%s)", #expr); \
    std::abort(); \
  }
#endif

#endif //TBOX_BASE_ASSERT_H_20221026
