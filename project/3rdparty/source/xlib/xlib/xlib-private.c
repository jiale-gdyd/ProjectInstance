#include <xlib/xlib/config.h>
#include <xlib/xlib/xlib-init.h>
#include <xlib/xlib/xlib-private.h>

XLibPrivateVTable *xlib__private__(void)
{
    static XLibPrivateVTable table = {
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
    };

    return &table;
}
