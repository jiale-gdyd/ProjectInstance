#include <xlib/xlib/config.h>
#include <xlib/xlib/xspawn.h>
#include <xlib/xlib/xshell.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xspawn-private.h>

#define INHERITS_OR_NULL_STDIN              (X_SPAWN_STDIN_FROM_DEV_NULL | X_SPAWN_CHILD_INHERITS_STDIN)
#define INHERITS_OR_NULL_STDOUT             (X_SPAWN_STDOUT_TO_DEV_NULL | X_SPAWN_CHILD_INHERITS_STDOUT)
#define INHERITS_OR_NULL_STDERR             (X_SPAWN_STDERR_TO_DEV_NULL | X_SPAWN_CHILD_INHERITS_STDERR)

xboolean x_spawn_async(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, XError **error)
{
    return x_spawn_async_with_pipes(working_directory, argv, envp, flags, child_setup, user_data, child_pid, NULL, NULL, NULL, error);
}

xboolean x_spawn_sync(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, xchar **standard_output, xchar **standard_error, xint *wait_status, XError **error)
{
    x_return_val_if_fail(argv != NULL, FALSE);
    x_return_val_if_fail(argv[0] != NULL, FALSE);
    x_return_val_if_fail(!(flags & X_SPAWN_DO_NOT_REAP_CHILD), FALSE);
    x_return_val_if_fail(standard_output == NULL || !(flags & X_SPAWN_STDOUT_TO_DEV_NULL), FALSE);
    x_return_val_if_fail(standard_error == NULL || !(flags & X_SPAWN_STDERR_TO_DEV_NULL), FALSE);

    return x_spawn_sync_impl(working_directory, argv, envp, flags, child_setup, user_data, standard_output, standard_error, wait_status, error);
}

xboolean x_spawn_async_with_pipes(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint *standard_input, xint *standard_output, xint *standard_error, XError **error)
{
    return x_spawn_async_with_pipes_and_fds(working_directory, (const xchar *const *)argv, (const xchar *const *)envp, flags, child_setup, user_data, -1, -1, -1, NULL, NULL, 0, child_pid, standard_input, standard_output, standard_error, error);
}

xboolean x_spawn_async_with_pipes_and_fds(const xchar *working_directory, const xchar *const *argv, const xchar *const *envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, xint stdin_fd, xint stdout_fd, xint stderr_fd, const xint *source_fds, const xint *target_fds, xsize n_fds, XPid *child_pid_out, xint *stdin_pipe_out, xint *stdout_pipe_out, xint *stderr_pipe_out, XError **error)
{
    x_return_val_if_fail(argv != NULL, FALSE);
    x_return_val_if_fail(argv[0] != NULL, FALSE);

    x_return_val_if_fail((flags & INHERITS_OR_NULL_STDIN) != INHERITS_OR_NULL_STDIN, FALSE);
    x_return_val_if_fail((flags & INHERITS_OR_NULL_STDOUT) != INHERITS_OR_NULL_STDOUT, FALSE);
    x_return_val_if_fail((flags & INHERITS_OR_NULL_STDERR) != INHERITS_OR_NULL_STDERR, FALSE);

    x_return_val_if_fail(stdin_pipe_out == NULL || stdin_fd < 0, FALSE);
    x_return_val_if_fail(stdout_pipe_out == NULL || stdout_fd < 0, FALSE);
    x_return_val_if_fail(stderr_pipe_out == NULL || stderr_fd < 0, FALSE);

    return x_spawn_async_with_pipes_and_fds_impl(working_directory, argv, envp, flags, child_setup, user_data, stdin_fd, stdout_fd, stderr_fd, source_fds, target_fds, n_fds, child_pid_out, stdin_pipe_out, stdout_pipe_out, stderr_pipe_out, error);
}

xboolean x_spawn_async_with_fds(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint stdin_fd, xint stdout_fd, xint stderr_fd, XError **error)
{
    x_return_val_if_fail(stdout_fd < 0 || !(flags & X_SPAWN_STDOUT_TO_DEV_NULL), FALSE);
    x_return_val_if_fail(stderr_fd < 0 || !(flags & X_SPAWN_STDERR_TO_DEV_NULL), FALSE);
    x_return_val_if_fail(stdin_fd < 0 || !(flags & X_SPAWN_CHILD_INHERITS_STDIN), FALSE);

    return x_spawn_async_with_pipes_and_fds(working_directory, (const xchar *const *)argv, (const xchar *const *)envp, flags, child_setup, user_data, stdin_fd, stdout_fd, stderr_fd, NULL, NULL, 0, child_pid, NULL, NULL, NULL, error);
}

xboolean x_spawn_command_line_sync(const xchar *command_line, xchar **standard_output, xchar **standard_error, xint *wait_status, XError **error)
{
    xboolean retval;
    xchar **argv = NULL;

    x_return_val_if_fail(command_line != NULL, FALSE);

    if (!x_shell_parse_argv(command_line, NULL, &argv, error)) {
        return FALSE;
    }

    retval = x_spawn_sync(NULL, argv, NULL, X_SPAWN_SEARCH_PATH, NULL, NULL, standard_output, standard_error, wait_status, error);
    x_strfreev(argv);

    return retval;
}

xboolean x_spawn_command_line_async(const xchar *command_line, XError **error)
{
    xboolean retval;
    xchar **argv = NULL;

    x_return_val_if_fail(command_line != NULL, FALSE);

    if (!x_shell_parse_argv(command_line, NULL, &argv, error)) {
        return FALSE;
    }

    retval = x_spawn_async(NULL, argv, NULL, X_SPAWN_SEARCH_PATH, NULL, NULL, NULL, error);
    x_strfreev(argv);

    return retval;
}

xboolean x_spawn_check_wait_status(xint wait_status, XError **error)
{
    return x_spawn_check_wait_status_impl(wait_status, error);
}

xboolean x_spawn_check_exit_status(xint wait_status, XError **error)
{
    return x_spawn_check_wait_status(wait_status, error);
}

void x_spawn_close_pid(XPid pid)
{
    x_spawn_close_pid_impl(pid);
}
