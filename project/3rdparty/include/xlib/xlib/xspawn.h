#ifndef __X_SPAWN_H__
#define __X_SPAWN_H__

#include "xerror.h"
#include "xtypes.h"

X_BEGIN_DECLS

#define X_SPAWN_ERROR               x_spawn_error_quark()
#define X_SPAWN_EXIT_ERROR          x_spawn_exit_error_quark()

typedef enum {
    X_SPAWN_ERROR_FORK,
    X_SPAWN_ERROR_READ,
    X_SPAWN_ERROR_CHDIR,
    X_SPAWN_ERROR_ACCES,
    X_SPAWN_ERROR_PERM,
    X_SPAWN_ERROR_TOO_BIG,
    X_SPAWN_ERROR_2BIG XLIB_DEPRECATED_ENUMERATOR_IN_2_32_FOR(X_SPAWN_ERROR_TOO_BIG) = X_SPAWN_ERROR_TOO_BIG,
    X_SPAWN_ERROR_NOEXEC,
    X_SPAWN_ERROR_NAMETOOLONG,
    X_SPAWN_ERROR_NOENT,
    X_SPAWN_ERROR_NOMEM,
    X_SPAWN_ERROR_NOTDIR,
    X_SPAWN_ERROR_LOOP,
    X_SPAWN_ERROR_TXTBUSY,
    X_SPAWN_ERROR_IO,
    X_SPAWN_ERROR_NFILE,
    X_SPAWN_ERROR_MFILE,
    X_SPAWN_ERROR_INVAL,
    X_SPAWN_ERROR_ISDIR,
    X_SPAWN_ERROR_LIBBAD,
    X_SPAWN_ERROR_FAILED
} XSpawnError;

typedef void (*XSpawnChildSetupFunc)(xpointer data);

typedef enum {
    X_SPAWN_DEFAULT                = 0,
    X_SPAWN_LEAVE_DESCRIPTORS_OPEN = 1 << 0,
    X_SPAWN_DO_NOT_REAP_CHILD      = 1 << 1,
    X_SPAWN_SEARCH_PATH            = 1 << 2,
    X_SPAWN_STDOUT_TO_DEV_NULL     = 1 << 3,
    X_SPAWN_STDERR_TO_DEV_NULL     = 1 << 4,
    X_SPAWN_CHILD_INHERITS_STDIN   = 1 << 5,
    X_SPAWN_FILE_AND_ARGV_ZERO     = 1 << 6,
    X_SPAWN_SEARCH_PATH_FROM_ENVP  = 1 << 7,
    X_SPAWN_CLOEXEC_PIPES          = 1 << 8,
    X_SPAWN_CHILD_INHERITS_STDOUT  = 1 << 9,
    X_SPAWN_CHILD_INHERITS_STDERR  = 1 << 10,
    X_SPAWN_STDIN_FROM_DEV_NULL    = 1 << 11
} XSpawnFlags;

XLIB_AVAILABLE_IN_ALL
XQuark x_spawn_error_quark(void);

XLIB_AVAILABLE_IN_ALL
XQuark x_spawn_exit_error_quark(void);

XLIB_AVAILABLE_IN_ALL
xboolean x_spawn_async(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_spawn_async_with_pipes(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint *standard_input, xint *standard_output, xint *standard_error, XError **error);

XLIB_AVAILABLE_IN_2_68
xboolean x_spawn_async_with_pipes_and_fds(const xchar *working_directory, const xchar *const *argv, const xchar *const *envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, xint stdin_fd, xint stdout_fd, xint stderr_fd, const xint *source_fds, const xint *target_fds, xsize n_fds, XPid *child_pid_out, xint *stdin_pipe_out, xint *stdout_pipe_out, xint *stderr_pipe_out, XError **error);

XLIB_AVAILABLE_IN_2_58
xboolean x_spawn_async_with_fds(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint stdin_fd, xint stdout_fd, xint stderr_fd, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_spawn_sync(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, xchar **standard_output, xchar **standard_error, xint *wait_status, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_spawn_command_line_sync(const xchar *command_line, xchar **standard_output, xchar **standard_error, xint *wait_status, XError **error);

XLIB_AVAILABLE_IN_ALL
xboolean x_spawn_command_line_async(const xchar *command_line, XError **error);

XLIB_AVAILABLE_IN_2_70
xboolean x_spawn_check_wait_status(xint wait_status, XError **error);

XLIB_DEPRECATED_IN_2_70_FOR(x_spawn_check_wait_status)
xboolean x_spawn_check_exit_status(xint wait_status, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_spawn_close_pid(XPid pid);

X_END_DECLS

#endif
