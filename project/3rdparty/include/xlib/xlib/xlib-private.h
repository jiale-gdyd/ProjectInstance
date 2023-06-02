#ifndef __XLIB_PRIVATE_H__
#define __XLIB_PRIVATE_H__

#include "../xlib.h"
#include "xwakeup.h"

static inline void x_ignore_leak(xconstpointer p)
{

}

static inline void x_ignore_strv_leak(XStrv strv)
{

}

static inline void x_begin_ignore_leaks(void)
{

}

static inline void x_end_ignore_leaks(void)
{

}

xboolean x_check_setuid(void);
XMainContext *x_get_worker_context(void);
XMainContext *x_main_context_new_with_next_id(xuint next_id);

XDir *x_dir_new_from_dirp(xpointer dirp);
XDir *x_dir_open_with_errno(const xchar *path, xuint flags);

char *x_find_program_for_path(const char *program, const char *path, const char *working_dir);

int x_uri_get_default_scheme_port(const char *scheme);

#define XLIB_PRIVATE_CALL(symbol)       (xlib__private__()->symbol)
#define XLIB_DEFAULT_LOCALE             ""

typedef struct {
    XWakeup *(*x_wakeup_new)(void);
    void (*x_wakeup_free)(XWakeup *wakeup);
    void (*x_wakeup_get_pollfd)(XWakeup *wakeup, XPollFD *poll_fd);
    void (*x_wakeup_signal)(XWakeup *wakeup);
    void (*x_wakeup_acknowledge)(XWakeup *wakeup);

    XMainContext *(*x_get_worker_context)(void);

    xboolean (*x_check_setuid)(void);
    XMainContext *(*x_main_context_new_with_next_id)(xuint next_id);

    XDir *(*x_dir_open_with_errno)(const xchar *path, xuint flags);
    XDir *(*x_dir_new_from_dirp)(xpointer dirp);

    void (*xlib_init)(void);

    char *(*x_find_program_for_path)(const char *program, const char *path, const char *working_dir);

    int (*x_uri_get_default_scheme_port)(const char *scheme);
} XLibPrivateVTable;

XLIB_AVAILABLE_IN_ALL
XLibPrivateVTable *xlib__private__(void);

#endif
