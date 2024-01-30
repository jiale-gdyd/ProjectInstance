#include <xlib/xlib/config.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xutilsprivate.h>

const XLibPrivateVTable *xlib__private__(void)
{
    static const XLibPrivateVTable table = {
        x_wakeup_new,
        x_wakeup_free,
        x_wakeup_get_pollfd,
        x_wakeup_signal,
        x_wakeup_acknowledge,

        x_get_worker_context,

        x_check_setuid,
        x_main_context_new_with_next_id,

        x_dir_open_with_errno,
        x_dir_new_from_dirp,

        xlib_init,

        x_find_program_for_path,

        x_uri_get_default_scheme_port,

        x_set_prgname_once,
    };

    return &table;
}
