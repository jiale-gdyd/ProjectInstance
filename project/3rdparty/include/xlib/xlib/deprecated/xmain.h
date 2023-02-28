#ifndef __X_DEPRECATED_MAIN_H__
#define __X_DEPRECATED_MAIN_H__

#include "../xmain.h"

X_BEGIN_DECLS

#define x_main_new(is_running)      x_main_loop_new(NULL, is_running) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_loop_new)
#define x_main_run(loop)            x_main_loop_run(loop) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_loop_run)
#define x_main_quit(loop)           x_main_loop_quit(loop) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_loop_quit)
#define x_main_destroy(loop)        x_main_loop_unref(loop) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_loop_unref)
#define x_main_is_running(loop)     x_main_loop_is_running(loop) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_loop_is_running)
#define x_main_iteration(may_block) x_main_context_iteration(NULL, may_block) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_context_iteration)
#define x_main_pending()            x_main_context_pending(NULL) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_context_pending)
#define x_main_set_poll_func(func)  x_main_context_set_poll_func(NULL, func) XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_main_context_set_poll_func)

X_END_DECLS

#endif
