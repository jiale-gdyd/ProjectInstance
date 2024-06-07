#ifndef __XLIB_PRIVATE_H__
#define __XLIB_PRIVATE_H__

#include "../xlib.h"
#include "xwakeup.h"
#include "xdatasetprivate.h"

#define X_SIGNEDNESS_OF(T)                  (((T) -1) <= 0)
#define X_CONTAINER_OF(ptr, type, field)    ((type *)X_STRUCT_MEMBER_P(ptr, -X_STRUCT_OFFSET(type, field)))

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

    xboolean (*x_set_prgname_once)(const xchar *prgname);

    xpointer (*x_datalist_id_update_atomic)(XData **datalist, XQuark key_id, XDataListUpdateAtomicFunc callback, xpointer user_data);
} XLibPrivateVTable;

XLIB_AVAILABLE_IN_ALL
const XLibPrivateVTable *xlib__private__(void);

xboolean x_uint_equal(xconstpointer v1, xconstpointer v2);
xuint x_uint_hash(xconstpointer v);

#if defined(__GNUC__)
#define X_THREAD_LOCAL      __thread
#else
#undef X_THREAD_LOCAL
#endif

#define _x_datalist_id_update_atomic(datalist, key_id, callback, user_data)     \
    (XLIB_PRIVATE_CALL(x_datalist_id_update_atomic)((datalist), (key_id), (callback), (user_data)))

#endif
