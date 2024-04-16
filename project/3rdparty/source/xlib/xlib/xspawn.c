#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xspawn.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xshell.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xstring.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xlib-unix.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xspawn-private.h>
#include <xlib/xlib/xtrace-private.h>

#define INHERITS_OR_NULL_STDIN              (X_SPAWN_STDIN_FROM_DEV_NULL | X_SPAWN_CHILD_INHERITS_STDIN)
#define INHERITS_OR_NULL_STDOUT             (X_SPAWN_STDOUT_TO_DEV_NULL | X_SPAWN_CHILD_INHERITS_STDOUT)
#define INHERITS_OR_NULL_STDERR             (X_SPAWN_STDERR_TO_DEV_NULL | X_SPAWN_CHILD_INHERITS_STDERR)

#define IS_STD_FILENO(_fd)                  ((_fd >= STDIN_FILENO) && (_fd <= STDERR_FILENO))
#define IS_VALID_FILENO(_fd)                (_fd >= 0)

#ifdef HAVE_POSIX_SPAWN
#ifdef __GLIBC__

#if __GLIBC_PREREQ(2, 24)
#define POSIX_SPAWN_AVAILABLE
#endif
#else
#define POSIX_SPAWN_AVAILABLE
#endif
#endif

extern char **environ;

#ifndef O_CLOEXEC
#define O_CLOEXEC                           0
#else
#define HAVE_O_CLOEXEC                      1
#endif

static xint x_execute(const xchar *file, xchar **argv, xchar **argv_buffer, xsize argv_buffer_len, xchar **envp, const xchar *search_path, xchar *search_path_buffer, xsize search_path_buffer_len);
static xboolean fork_exec(xboolean intermediate_child, const xchar *working_directory, const xchar *const *argv, const xchar *const *envp, xboolean close_descriptors, xboolean search_path, xboolean search_path_from_envp, xboolean stdout_to_null, xboolean stderr_to_null, xboolean child_inherits_stdin, xboolean file_and_argv_zero, xboolean cloexec_pipes, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint *stdin_pipe_out, xint *stdout_pipe_out, xint *stderr_pipe_out, xint stdin_fd, xint stdout_fd, xint stderr_fd, const xint *source_fds, const xint *target_fds, xsize n_fds, XError **error);

X_DEFINE_QUARK(x-exec-error-quark, x_spawn_error)
X_DEFINE_QUARK(x-spawn-exit-error-quark, x_spawn_exit_error)

xboolean x_spawn_async(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, XError **error)
{
    return x_spawn_async_with_pipes(working_directory, argv, envp, flags, child_setup, user_data, child_pid, NULL, NULL, NULL, error);
}

#undef READ_OK

typedef enum {
    READ_FAILED = 0,
    READ_OK,
    READ_EOF
} ReadResult;

static ReadResult read_data(XString *str, xint fd, XError **error)
{
    xssize bytes;
    xchar buf[4096];

again:
    bytes = read(fd, buf, 4096);
    if (bytes == 0) {
        return READ_EOF;
    } else if (bytes > 0) {
        x_string_append_len(str, buf, bytes);
        return READ_OK;
    } else if (errno == EINTR) {
        goto again;
    } else {
        int errsv = errno;
        x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_READ, _("Failed to read data from child process (%s)"), x_strerror(errsv));

        return READ_FAILED;
    }
}

xboolean x_spawn_sync(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, xchar **standard_output, xchar **standard_error, xint *wait_status, XError **error)
{
    XPid pid;
    xint ret;
    xint status;
    xboolean failed;
    xint outpipe = -1;
    xint errpipe = -1;
    XString *outstr = NULL;
    XString *errstr = NULL;

    x_return_val_if_fail(argv != NULL, FALSE);
    x_return_val_if_fail(argv[0] != NULL, FALSE);
    x_return_val_if_fail(!(flags & X_SPAWN_DO_NOT_REAP_CHILD), FALSE);
    x_return_val_if_fail(standard_output == NULL || !(flags & X_SPAWN_STDOUT_TO_DEV_NULL), FALSE);
    x_return_val_if_fail (standard_error == NULL || !(flags & X_SPAWN_STDERR_TO_DEV_NULL), FALSE);

    if (standard_output) {
        *standard_output = NULL;
    }

    if (standard_error) {
        *standard_error = NULL;
    }

    if (!fork_exec(FALSE,
        working_directory,
        (const xchar *const *)argv,
        (const xchar *const *)envp,
        !(flags & X_SPAWN_LEAVE_DESCRIPTORS_OPEN),
        (flags & X_SPAWN_SEARCH_PATH) != 0,
        (flags & X_SPAWN_SEARCH_PATH_FROM_ENVP) != 0,
        (flags & X_SPAWN_STDOUT_TO_DEV_NULL) != 0,
        (flags & X_SPAWN_STDERR_TO_DEV_NULL) != 0,
        (flags & X_SPAWN_CHILD_INHERITS_STDIN) != 0,
        (flags & X_SPAWN_FILE_AND_ARGV_ZERO) != 0,
        (flags & X_SPAWN_CLOEXEC_PIPES) != 0,
        child_setup,
        user_data,
        &pid,
        NULL,
        standard_output ? &outpipe : NULL,
        standard_error ? &errpipe : NULL,
        -1, -1, -1,
        NULL, NULL, 0,
        error))
    {
        return FALSE;
    }

    failed = FALSE;
    if (outpipe >= 0) {
        outstr = x_string_new(NULL);
    }

    if (errpipe >= 0) {
        errstr = x_string_new(NULL);
    }

    while (!failed && (outpipe >= 0 || errpipe >= 0)) {
        XPollFD fds[] = {
            { outpipe, X_IO_IN | X_IO_HUP | X_IO_ERR, 0 },
            { errpipe, X_IO_IN | X_IO_HUP | X_IO_ERR, 0 },
        };

        ret = x_poll(fds, X_N_ELEMENTS(fds), -1);
        if (ret < 0) {
            int errsv = errno;

            if (errno == EINTR) {
                continue;
            }

            failed = TRUE;
            x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_READ, _("Unexpected error in reading data from a child process (%s)"), x_strerror(errsv));
            break;
        }

        if (outpipe >= 0 && fds[0].revents != 0) {
            switch (read_data(outstr, outpipe, error)) {
                case READ_FAILED:
                    failed = TRUE;
                    break;

                case READ_EOF:
                    x_clear_fd(&outpipe, NULL);
                    break;

                default:
                    break;
            }

            if (failed) {
                break;
            }
        }

        if (errpipe >= 0 && fds[1].revents != 0) {
            switch (read_data(errstr, errpipe, error)) {
                case READ_FAILED:
                    failed = TRUE;
                    break;

                case READ_EOF:
                    x_clear_fd(&errpipe, NULL);
                    break;

                default:
                    break;
            }

            if (failed) {
                break;
            }
        }
    }

    x_clear_fd(&outpipe, NULL);
    x_clear_fd(&errpipe, NULL);

again:
    ret = waitpid(pid, &status, 0);
    if (ret < 0) {
        if (errno == EINTR) {
            goto again;
        } else if (errno == ECHILD) {
            if (wait_status) {
                x_warning("In call to x_spawn_sync(), wait status of a child process was requested but ECHILD was received by waitpid(). See the documentation of x_child_watch_source_new() for possible causes.");
            } else {

            }
        } else {
            if (!failed) {
                int errsv = errno;
                failed = TRUE;

                x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_READ, _("Unexpected error in waitpid() (%s)"), x_strerror(errsv));
            }
        }
    }

    if (failed) {
        if (outstr) {
            x_string_free(outstr, TRUE);
        }

        if (errstr) {
            x_string_free(errstr, TRUE);
        }

        return FALSE;
    } else {
        if (wait_status) {
            *wait_status = status;
        }

        if (standard_output) {
            *standard_output = x_string_free(outstr, FALSE);
        }

        if (standard_error) {
            *standard_error = x_string_free(errstr, FALSE);
        }

        return TRUE;
    }
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

    if ((flags & INHERITS_OR_NULL_STDIN) != 0) {
        stdin_pipe_out = NULL;
    }

    if ((flags & INHERITS_OR_NULL_STDOUT) != 0) {
        stdout_pipe_out = NULL;
    }

    if ((flags & INHERITS_OR_NULL_STDERR) != 0) {
        stderr_pipe_out = NULL;
    }

    return fork_exec(!(flags & X_SPAWN_DO_NOT_REAP_CHILD),
        working_directory,
        (const xchar *const *) argv,
        (const xchar *const *) envp,
        !(flags & X_SPAWN_LEAVE_DESCRIPTORS_OPEN),
        (flags & X_SPAWN_SEARCH_PATH) != 0,
        (flags & X_SPAWN_SEARCH_PATH_FROM_ENVP) != 0,
        (flags & X_SPAWN_STDOUT_TO_DEV_NULL) != 0,
        (flags & X_SPAWN_STDERR_TO_DEV_NULL) != 0,
        (flags & X_SPAWN_CHILD_INHERITS_STDIN) != 0,
        (flags & X_SPAWN_FILE_AND_ARGV_ZERO) != 0,
        (flags & X_SPAWN_CLOEXEC_PIPES) != 0,
        child_setup,
        user_data,
        child_pid_out,
        stdin_pipe_out,
        stdout_pipe_out,
        stderr_pipe_out,
        stdin_fd,
        stdout_fd,
        stderr_fd,
        source_fds,
        target_fds,
        n_fds,
        error);
}

xboolean x_spawn_async_with_fds(const xchar *working_directory, xchar **argv, xchar **envp, XSpawnFlags flags, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint stdin_fd, xint stdout_fd, xint stderr_fd, XError **error)
{
    x_return_val_if_fail(argv != NULL, FALSE);
    x_return_val_if_fail(argv[0] != NULL, FALSE);
    x_return_val_if_fail(stdout_fd < 0 || !(flags & X_SPAWN_STDOUT_TO_DEV_NULL), FALSE);
    x_return_val_if_fail(stderr_fd < 0 || !(flags & X_SPAWN_STDERR_TO_DEV_NULL), FALSE);
    x_return_val_if_fail(stdin_fd < 0 || !(flags & X_SPAWN_CHILD_INHERITS_STDIN), FALSE);

    return fork_exec(!(flags & X_SPAWN_DO_NOT_REAP_CHILD),
        working_directory,
        (const xchar *const *)argv,
        (const xchar *const *)envp,
        !(flags & X_SPAWN_LEAVE_DESCRIPTORS_OPEN),
        (flags & X_SPAWN_SEARCH_PATH) != 0,
        (flags & X_SPAWN_SEARCH_PATH_FROM_ENVP) != 0,
        (flags & X_SPAWN_STDOUT_TO_DEV_NULL) != 0,
        (flags & X_SPAWN_STDERR_TO_DEV_NULL) != 0,
        (flags & X_SPAWN_CHILD_INHERITS_STDIN) != 0,
        (flags & X_SPAWN_FILE_AND_ARGV_ZERO) != 0,
        (flags & X_SPAWN_CLOEXEC_PIPES) != 0,
        child_setup,
        user_data,
        child_pid,
        NULL, NULL, NULL,
        stdin_fd,
        stdout_fd,
        stderr_fd,
        NULL, NULL, 0,
        error);
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
    xboolean ret = FALSE;

    if (WIFEXITED (wait_status)) {
        if (WEXITSTATUS(wait_status) != 0) {
            x_set_error(error, X_SPAWN_EXIT_ERROR, WEXITSTATUS(wait_status), _("Child process exited with code %ld"), (long)WEXITSTATUS(wait_status));
            goto out;
        }
    } else if (WIFSIGNALED(wait_status)) {
        x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Child process killed by signal %ld"), (long)WTERMSIG(wait_status));
        goto out;
    } else if (WIFSTOPPED(wait_status)) {
        x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Child process stopped by signal %ld"), (long)WSTOPSIG(wait_status));
        goto out;
    } else {
        x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Child process exited abnormally"));
        goto out;
    }

    ret = TRUE;
out:
    return ret;
}

xboolean x_spawn_check_exit_status(xint wait_status, XError **error)
{
    return x_spawn_check_wait_status(wait_status, error);
}

static xssize write_all(xint fd, xconstpointer vbuf, xsize to_write)
{
    xchar *buf = (xchar *)vbuf;

    while (to_write > 0) {
        xssize count = write(fd, buf, to_write);
        if (count < 0) {
            if (errno != EINTR) {
                return FALSE;
            }
        } else {
            to_write -= count;
            buf += count;
        }
    }

    return TRUE;
}

X_NORETURN static void write_err_and_exit(xint fd, xint msg)
{
    xint en = errno;

    write_all(fd, &msg, sizeof(msg));
    write_all(fd, &en, sizeof(en));

    _exit(1);
}

static void set_cloexec(int fd)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static void unset_cloexec(int fd)
{
    int flags;
    int result;

    flags = fcntl(fd, F_GETFD, 0);
    if (flags != -1) {
        int errsv;
        flags &= (~FD_CLOEXEC);

        do {
            result = fcntl(fd, F_SETFD, flags);
            errsv = errno;
        } while (result == -1 && errsv == EINTR);
    }
}

static int dupfd_cloexec(int old_fd, int new_fd_min)
{
  int fd, errsv;

#ifdef F_DUPFD_CLOEXEC
    do {
        fd = fcntl(old_fd, F_DUPFD_CLOEXEC, new_fd_min);
        errsv = errno;
    } while (fd == -1 && errsv == EINTR);
#else
    int result, flags;
    do {
        fd = fcntl(old_fd, F_DUPFD, new_fd_min);
        errsv = errno;
    } while (fd == -1 && errsv == EINTR);

    flags = fcntl(fd, F_GETFD, 0);
    if (flags != -1) {
        flags |= FD_CLOEXEC;
        do {
            result = fcntl (fd, F_SETFD, flags);
            errsv = errno;
        } while (result == -1 && errsv == EINTR);
    }
#endif

    return fd;
}

static xint safe_dup2(xint fd1, xint fd2)
{
    xint ret;

    do {
        ret = dup2(fd1, fd2);
    } while (ret < 0 && (errno == EINTR || errno == EBUSY));

    return ret;
}

static xboolean relocate_fd_out_of_standard_range(xint *fd)
{
    xint ret = -1;
    const int min_fileno = STDERR_FILENO + 1;

    do {
        ret = fcntl(*fd, F_DUPFD, min_fileno);
    } while ((ret < 0) && (errno == EINTR));

    if (ret >= min_fileno) {
        *fd = ret;
        return TRUE;
    }

    return FALSE;
}

static xint safe_open(const char *path, xint mode)
{
    xint ret;

    do {
        ret = open(path, mode);
    } while (ret < 0 && errno == EINTR);

    return ret;
}

enum {
    CHILD_CHDIR_FAILED,
    CHILD_EXEC_FAILED,
    CHILD_OPEN_FAILED,
    CHILD_DUPFD_FAILED,
    CHILD_FORK_FAILED,
    CHILD_CLOSE_FAILED,
};

static void do_exec(xint child_err_report_fd, xint stdin_fd, xint stdout_fd, xint stderr_fd, xint *source_fds, const xint *target_fds, xsize n_fds, const xchar *working_directory, const xchar *const *argv, xchar **argv_buffer, xsize argv_buffer_len, const xchar *const *envp, xboolean close_descriptors, const xchar *search_path, xchar *search_path_buffer, xsize search_path_buffer_len, xboolean stdout_to_null, xboolean stderr_to_null, xboolean child_inherits_stdin, xboolean file_and_argv_zero, XSpawnChildSetupFunc child_setup, xpointer user_data)
{
    xsize i;
    xint max_target_fd = 0;

    if (working_directory && chdir(working_directory) < 0) {
        write_err_and_exit(child_err_report_fd, CHILD_CHDIR_FAILED);
    }

    if (IS_STD_FILENO(stdin_fd) && (stdin_fd != STDIN_FILENO)) {
        int old_fd = stdin_fd;

        if (!relocate_fd_out_of_standard_range(&stdin_fd)) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        if (stdout_fd == old_fd) {
            stdout_fd = stdin_fd;
        }

        if (stderr_fd == old_fd) {
            stderr_fd = stdin_fd;
        }
    }

    if (IS_VALID_FILENO(stdin_fd) && (stdin_fd != STDIN_FILENO)) {
        if (safe_dup2(stdin_fd, 0) < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        set_cloexec(stdin_fd);
    } else if (!child_inherits_stdin) {
        xint read_null = safe_open("/dev/null", O_RDONLY);
        if (read_null < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_OPEN_FAILED);
        }

        if (safe_dup2(read_null, 0) < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        x_clear_fd(&read_null, NULL);
    }

    if (IS_STD_FILENO(stdout_fd) && (stdout_fd != STDOUT_FILENO)) {
        int old_fd = stdout_fd;

        if (!relocate_fd_out_of_standard_range(&stdout_fd)) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        if (stderr_fd == old_fd) {
            stderr_fd = stdout_fd;
        }
    }

    if (IS_VALID_FILENO(stdout_fd) && (stdout_fd != STDOUT_FILENO)) {
        if (safe_dup2(stdout_fd, 1) < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        set_cloexec(stdout_fd);
    } else if (stdout_to_null) {
        xint write_null = safe_open("/dev/null", O_WRONLY);
        if (write_null < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_OPEN_FAILED);
        }

        if (safe_dup2(write_null, 1) < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        x_clear_fd(&write_null, NULL);
    }

    if (IS_STD_FILENO(stderr_fd) && (stderr_fd != STDERR_FILENO)) {
        if (!relocate_fd_out_of_standard_range(&stderr_fd)) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }
    }

    if (IS_VALID_FILENO(stderr_fd) && (stderr_fd != STDERR_FILENO)) {
        if (safe_dup2(stderr_fd, 2) < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        set_cloexec(stderr_fd);
    } else if (stderr_to_null) {
        xint write_null = safe_open("/dev/null", O_WRONLY);
        if (write_null < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_OPEN_FAILED);
        }

        if (safe_dup2(write_null, 2) < 0) {
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        x_clear_fd(&write_null, NULL);
    }

    if (close_descriptors) {
        if (child_setup == NULL && n_fds == 0) {
            if (safe_dup2(child_err_report_fd, 3) < 0) {
                write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
            }

            set_cloexec(3);
            if (x_closefrom(4) < 0) {
                write_err_and_exit(child_err_report_fd, CHILD_CLOSE_FAILED);
            }

            child_err_report_fd = 3;
        } else {
            if (x_fdwalk_set_cloexec(3) < 0) {
                write_err_and_exit(child_err_report_fd, CHILD_CLOSE_FAILED);
            }
        }
    } else {
        set_cloexec(child_err_report_fd);
    }

    if (n_fds > 0) {
        for (i = 0; i < n_fds; i++) {
            max_target_fd = MAX(max_target_fd, target_fds[i]);
        }

        if (max_target_fd == X_MAXINT) {
            errno = EINVAL;
            write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
        }

        for (i = 0; i < n_fds; i++) {
            if (source_fds[i] != target_fds[i]) {
                source_fds[i] = dupfd_cloexec(source_fds[i], max_target_fd + 1);
                if (source_fds[i] < 0) {
                    write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
                }
            }
        }

        for (i = 0; i < n_fds; i++) {
            if (source_fds[i] == target_fds[i]) {
                unset_cloexec(source_fds[i]);
            } else {
                if (target_fds[i] == child_err_report_fd) {
                    child_err_report_fd = dupfd_cloexec(child_err_report_fd, max_target_fd + 1);
                    if (child_err_report_fd < 0) {
                        write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
                    }
                }

                if (safe_dup2(source_fds[i], target_fds[i]) < 0) {
                    write_err_and_exit(child_err_report_fd, CHILD_DUPFD_FAILED);
                }

                x_clear_fd(&source_fds[i], NULL);
            }
        }
    }

    if (child_setup) {
        (*child_setup)(user_data);
    }

    x_execute(argv[0], (xchar **)(file_and_argv_zero ? argv + 1 : argv), argv_buffer, argv_buffer_len, (xchar **)envp, search_path, search_path_buffer, search_path_buffer_len);

    write_err_and_exit(child_err_report_fd, CHILD_EXEC_FAILED);
}

static xboolean read_ints(int fd, xint *buf, xint n_ints_in_buf, xint *n_ints_read, XError **error)
{
    xsize bytes = 0;

    while (TRUE) {
        xssize chunk;

        if (bytes >= sizeof(xint) * 2) {
            break;
        }

again:
        chunk = read(fd, ((xchar *)buf) + bytes, sizeof(xint) * n_ints_in_buf - bytes);
        if (chunk < 0 && errno == EINTR) {
            goto again;
        }

        if (chunk < 0) {
            int errsv = errno;
            x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Failed to read from child pipe (%s)"), x_strerror(errsv));

            return FALSE;
        } else if (chunk == 0) {
            break;
        }else {
            bytes += chunk;
        }
    }

    *n_ints_read = (xint)(bytes / sizeof(xint));
    return TRUE;
}

#ifdef POSIX_SPAWN_AVAILABLE
static xboolean do_posix_spawn(const xchar *const *argv, const xchar *const *envp, xboolean search_path, xboolean stdout_to_null, xboolean stderr_to_null, xboolean child_inherits_stdin, xboolean file_and_argv_zero, XPid *child_pid, xint *child_close_fds, xint stdin_fd, xint stdout_fd, xint stderr_fd, const xint *source_fds, const xint *target_fds, xsize n_fds)
{
    int r;
    xsize i;
    pid_t pid;
    XSList *elem;
    sigset_t mask;
    xint max_target_fd = 0;
    posix_spawnattr_t attr;
    xint parent_close_fds[3];
    XSList *child_close = NULL;
    const xchar *const *argv_pass;
    xint *duped_source_fds = NULL;
    xsize num_parent_close_fds = 0;
    posix_spawn_file_actions_t file_actions;

    x_assert(argv != NULL && argv[0] != NULL);

    if (*argv[0] == '\0') {
        return ENOENT;
    }

    r = posix_spawnattr_init(&attr);
    if (r != 0) {
        return r;
    }

    if (child_close_fds) {
        int i = -1;
        while (child_close_fds[++i] != -1) {
            child_close = x_slist_prepend(child_close, XINT_TO_POINTER(child_close_fds[i]));
        }
    }

    r = posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF);
    if (r != 0) {
        goto out_free_spawnattr;
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);

    r = posix_spawnattr_setsigdefault(&attr, &mask);
    if (r != 0) {
        goto out_free_spawnattr;
    }

    r = posix_spawn_file_actions_init(&file_actions);
    if (r != 0) {
        goto out_free_spawnattr;
    }

    if (stdin_fd >= 0) {
        r = posix_spawn_file_actions_adddup2(&file_actions, stdin_fd, 0);
        if (r != 0) {
            goto out_close_fds;
        }

        if (!x_slist_find(child_close, XINT_TO_POINTER(stdin_fd))) {
            child_close = x_slist_prepend(child_close, XINT_TO_POINTER(stdin_fd));
        }
    } else if (!child_inherits_stdin) {
        xint read_null = safe_open("/dev/null", O_RDONLY | O_CLOEXEC);
        x_assert(read_null != -1);
        parent_close_fds[num_parent_close_fds++] = read_null;

#ifndef HAVE_O_CLOEXEC
        fcntl(read_null, F_SETFD, FD_CLOEXEC);
#endif
        r = posix_spawn_file_actions_adddup2(&file_actions, read_null, 0);
        if (r != 0) {
            goto out_close_fds;
        }
    }

    if (stdout_fd >= 0) {
        r = posix_spawn_file_actions_adddup2(&file_actions, stdout_fd, 1);
        if (r != 0) {
            goto out_close_fds;
        }

        if (!x_slist_find(child_close, XINT_TO_POINTER(stdout_fd))) {
            child_close = x_slist_prepend(child_close, XINT_TO_POINTER(stdout_fd));
        }
    } else if (stdout_to_null) {
        xint write_null = safe_open("/dev/null", O_WRONLY | O_CLOEXEC);
        x_assert(write_null != -1);
        parent_close_fds[num_parent_close_fds++] = write_null;

#ifndef HAVE_O_CLOEXEC
        fcntl (write_null, F_SETFD, FD_CLOEXEC);
#endif
        r = posix_spawn_file_actions_adddup2(&file_actions, write_null, 1);
        if (r != 0) {
            goto out_close_fds;
        }
    }

    if (stderr_fd >= 0) {
        r = posix_spawn_file_actions_adddup2(&file_actions, stderr_fd, 2);
        if (r != 0) {
            goto out_close_fds;
        }

        if (!x_slist_find(child_close, XINT_TO_POINTER(stderr_fd))) {
            child_close = x_slist_prepend(child_close, XINT_TO_POINTER(stderr_fd));
        }
    } else if (stderr_to_null) {
        xint write_null = safe_open("/dev/null", O_WRONLY | O_CLOEXEC);
        x_assert(write_null != -1);
        parent_close_fds[num_parent_close_fds++] = write_null;

#ifndef HAVE_O_CLOEXEC
        fcntl(write_null, F_SETFD, FD_CLOEXEC);
#endif
        r = posix_spawn_file_actions_adddup2(&file_actions, write_null, 2);
        if (r != 0) {
            goto out_close_fds;
        }
    }

    for (i = 0; i < n_fds; i++) {
        max_target_fd = MAX(max_target_fd, target_fds[i]);
    }

    if (max_target_fd == X_MAXINT) {
        goto out_close_fds;
    }

    duped_source_fds = x_new(xint, n_fds);
    for (i = 0; i < n_fds; i++) {
        duped_source_fds[i] = -1;
    }

    for (i = 0; i < n_fds; i++) {
        duped_source_fds[i] = dupfd_cloexec(source_fds[i], max_target_fd + 1);
        if (duped_source_fds[i] < 0) {
            goto out_close_fds;
        }
    }

    for (i = 0; i < n_fds; i++) {
        r = posix_spawn_file_actions_adddup2(&file_actions, duped_source_fds[i], target_fds[i]);
        if (r != 0) {
            goto out_close_fds;
        }
    }

    for (elem = child_close; elem != NULL; elem = elem->next) {
        r = posix_spawn_file_actions_addclose(&file_actions, XPOINTER_TO_INT(elem->data));
        if (r != 0) {
            goto out_close_fds;
        }
    }

    argv_pass = file_and_argv_zero ? argv + 1 : argv;
    if (envp == NULL) {
        envp = (const xchar *const *)environ;
    }

    if (!search_path || strchr(argv[0], '/') != NULL) {
        r = posix_spawn(&pid, argv[0], &file_actions, &attr, (char *const *)argv_pass, (char *const *)envp);
    } else {
        r = posix_spawnp(&pid, argv[0], &file_actions, &attr, (char *const *)argv_pass, (char *const *)envp);
    }

    if (r == 0 && child_pid != NULL) {
        *child_pid = pid;
    }

out_close_fds:
    for (i = 0; i < num_parent_close_fds; i++) {
        x_clear_fd(&parent_close_fds[i], NULL);
    }

    if (duped_source_fds != NULL) {
        for (i = 0; i < n_fds; i++) {
            x_clear_fd(&duped_source_fds[i], NULL);
        }

        x_free(duped_source_fds);
    }

    posix_spawn_file_actions_destroy(&file_actions);

out_free_spawnattr:
    posix_spawnattr_destroy(&attr);
    x_slist_free(child_close);

    return r;
}
#endif

static xboolean source_fds_collide_with_pipe(const XUnixPipe *pipefd, const int *source_fds, xsize n_fds, XError **error)
{
    return (_x_spawn_invalid_source_fd(pipefd->fds[X_UNIX_PIPE_END_READ], source_fds, n_fds, error) || _x_spawn_invalid_source_fd(pipefd->fds[X_UNIX_PIPE_END_WRITE], source_fds, n_fds, error));
}

static xboolean fork_exec(xboolean intermediate_child, const xchar *working_directory, const xchar *const *argv, const xchar *const *envp, xboolean close_descriptors, xboolean search_path, xboolean search_path_from_envp, xboolean stdout_to_null, xboolean stderr_to_null, xboolean child_inherits_stdin, xboolean file_and_argv_zero, xboolean cloexec_pipes, XSpawnChildSetupFunc child_setup, xpointer user_data, XPid *child_pid, xint *stdin_pipe_out, xint *stdout_pipe_out, xint *stderr_pipe_out, xint stdin_fd, xint stdout_fd, xint stderr_fd, const xint *source_fds, const xint *target_fds, xsize n_fds, XError **error)
{
    xint status;
    XPid pid = -1;
    xsize argv_buffer_len = 0;
    xint n_child_close_fds = 0;
    xchar **argv_buffer = NULL;
    xint *source_fds_copy = NULL;
    const xchar *chosen_search_path;
    xchar **argv_buffer_heap = NULL;
    XUnixPipe stdin_pipe = X_UNIX_PIPE_INIT;
    XUnixPipe stdout_pipe = X_UNIX_PIPE_INIT;
    XUnixPipe stderr_pipe = X_UNIX_PIPE_INIT;
    xchar *search_path_buffer = NULL;
    xsize search_path_buffer_len = 0;
    xchar *search_path_buffer_heap = NULL;
    XUnixPipe child_err_report_pipe = X_UNIX_PIPE_INIT;
    XUnixPipe child_pid_report_pipe = X_UNIX_PIPE_INIT;
    xint child_close_fds[4] = { -1, -1, -1, -1 };
    xuint pipe_flags = cloexec_pipes ? O_CLOEXEC : 0;

    x_assert(argv != NULL && argv[0] != NULL);
    x_assert(stdin_pipe_out == NULL || stdin_fd < 0);
    x_assert(stdout_pipe_out == NULL || stdout_fd < 0);
    x_assert(stderr_pipe_out == NULL || stderr_fd < 0);

    if (stdin_pipe_out != NULL) {
        if (!x_unix_pipe_open(&stdin_pipe, pipe_flags, error)) {
            goto cleanup_and_fail;
        }

        if (source_fds_collide_with_pipe(&stdin_pipe, source_fds, n_fds, error)) {
            goto cleanup_and_fail;
        }

        child_close_fds[n_child_close_fds++] = x_unix_pipe_get(&stdin_pipe, X_UNIX_PIPE_END_WRITE);
        stdin_fd = x_unix_pipe_get(&stdin_pipe, X_UNIX_PIPE_END_READ);
    }

    if (stdout_pipe_out != NULL) {
        if (!x_unix_pipe_open(&stdout_pipe, pipe_flags, error)) {
            goto cleanup_and_fail;
        }

        if (source_fds_collide_with_pipe(&stdout_pipe, source_fds, n_fds, error)) {
            goto cleanup_and_fail;
        }

        child_close_fds[n_child_close_fds++] = x_unix_pipe_get(&stdout_pipe, X_UNIX_PIPE_END_READ);
        stdout_fd = x_unix_pipe_get(&stdout_pipe, X_UNIX_PIPE_END_WRITE);
    }

    if (stderr_pipe_out != NULL) {
        if (!x_unix_pipe_open(&stderr_pipe, pipe_flags, error)) {
            goto cleanup_and_fail;
        }

        if (source_fds_collide_with_pipe(&stderr_pipe, source_fds, n_fds, error)) {
            goto cleanup_and_fail;
        }

        child_close_fds[n_child_close_fds++] = x_unix_pipe_get(&stderr_pipe, X_UNIX_PIPE_END_READ);
        stderr_fd = x_unix_pipe_get(&stderr_pipe, X_UNIX_PIPE_END_WRITE);
    }

    child_close_fds[n_child_close_fds++] = -1;

#ifdef POSIX_SPAWN_AVAILABLE
    if (!intermediate_child && working_directory == NULL && !close_descriptors && !search_path_from_envp && child_setup == NULL) {
        x_trace_mark(X_TRACE_CURRENT_TIME, 0, "XLib", "posix_spawn", "%s", argv[0]);

        status = do_posix_spawn(argv, envp, search_path, stdout_to_null, stderr_to_null, child_inherits_stdin, file_and_argv_zero, child_pid, child_close_fds, stdin_fd, stdout_fd, stderr_fd, source_fds, target_fds, n_fds);
        if (status == 0) {
            goto success;
        }

        if (status != ENOEXEC) {
            x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Failed to spawn child process “%s” (%s)"), argv[0], x_strerror(status));
            goto cleanup_and_fail;
        }

        x_debug("posix_spawn failed (ENOEXEC), fall back to regular gspawn");
    } else {
        x_trace_mark(X_TRACE_CURRENT_TIME, 0, "XLib", "fork", "posix_spawn avoided %s%s%s%s%s", !intermediate_child ? "" : "(automatic reaping requested) ", working_directory == NULL ? "" : "(workdir specified) ", !close_descriptors ? "" : "(fd close requested) ", !search_path_from_envp ? "" : "(using envp for search path) ", child_setup == NULL ? "" : "(child_setup specified) ");
    }
#endif

    chosen_search_path = NULL;
    if (search_path_from_envp) {
        chosen_search_path = x_environ_getenv((xchar **)envp, "PATH");
    }

    if (search_path && chosen_search_path == NULL) {
        chosen_search_path = x_getenv("PATH");
    }

    if ((search_path || search_path_from_envp) && chosen_search_path == NULL) {
        chosen_search_path = "/bin:/usr/bin:.";
    }

    if (search_path || search_path_from_envp) {
        x_assert(chosen_search_path != NULL);
    } else {
        x_assert(chosen_search_path == NULL);
    }

    if (chosen_search_path != NULL) {
        search_path_buffer_len = strlen(chosen_search_path) + strlen(argv[0]) + 2;
        if (search_path_buffer_len < 4000) {
            search_path_buffer = (xchar *)x_alloca(search_path_buffer_len);
        } else{
            search_path_buffer_heap = (xchar *)x_malloc(search_path_buffer_len);
            search_path_buffer = search_path_buffer_heap;
        }
    }

    if (search_path || search_path_from_envp) {
        x_assert(search_path_buffer != NULL);
    } else {
        x_assert(search_path_buffer == NULL);
    }

    argv_buffer_len = x_strv_length((xchar **) argv) + 2;
    if (argv_buffer_len < 4000 / sizeof (xchar *)) {
        argv_buffer = x_newa(xchar *, argv_buffer_len);
    } else {
        argv_buffer_heap = x_new(xchar *, argv_buffer_len);
        argv_buffer = argv_buffer_heap;
    }

    source_fds_copy = x_new(int, n_fds);
    if (n_fds > 0) {
        memcpy(source_fds_copy, source_fds, sizeof(*source_fds) * n_fds);
    }

    if (!x_unix_pipe_open(&child_err_report_pipe, pipe_flags, error)) {
        goto cleanup_and_fail;
    }

    if (source_fds_collide_with_pipe(&child_err_report_pipe, source_fds, n_fds, error)) {
        goto cleanup_and_fail;
    }

    if (intermediate_child) {
        if (!x_unix_pipe_open(&child_pid_report_pipe, pipe_flags, error)) {
            goto cleanup_and_fail;
        }

        if (source_fds_collide_with_pipe(&child_pid_report_pipe, source_fds, n_fds, error)) {
            goto cleanup_and_fail;
        }
    }

    pid = fork();
    if (pid < 0) {
        int errsv = errno;
        x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FORK, _("Failed to fork (%s)"), x_strerror(errsv));

        goto cleanup_and_fail;
    } else if (pid == 0) {
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);

        x_unix_pipe_close(&child_err_report_pipe, X_UNIX_PIPE_END_READ, NULL);
        x_unix_pipe_close(&child_pid_report_pipe, X_UNIX_PIPE_END_READ, NULL);

        if (child_close_fds[0] != -1) {
            int i = -1;
            while (child_close_fds[++i] != -1) {
                x_clear_fd(&child_close_fds[i], NULL);
            }
        }

        if (intermediate_child) {
            XPid grandchild_pid;
            grandchild_pid = fork();

            if (grandchild_pid < 0) {
                write_all(x_unix_pipe_get(&child_pid_report_pipe, X_UNIX_PIPE_END_WRITE), &grandchild_pid, sizeof(grandchild_pid));
                write_err_and_exit(x_unix_pipe_get(&child_err_report_pipe, X_UNIX_PIPE_END_WRITE), CHILD_FORK_FAILED);  
            } else if (grandchild_pid == 0) {
                x_unix_pipe_close(&child_pid_report_pipe, X_UNIX_PIPE_END_WRITE, NULL);
                do_exec(x_unix_pipe_get(&child_err_report_pipe, X_UNIX_PIPE_END_WRITE), stdin_fd, stdout_fd, stderr_fd, source_fds_copy, target_fds, n_fds, working_directory, argv, argv_buffer, argv_buffer_len, envp, close_descriptors, chosen_search_path, search_path_buffer, search_path_buffer_len, stdout_to_null, stderr_to_null, child_inherits_stdin, file_and_argv_zero, child_setup, user_data);
            } else {
                write_all(x_unix_pipe_get(&child_pid_report_pipe, X_UNIX_PIPE_END_WRITE), &grandchild_pid, sizeof(grandchild_pid));
                x_unix_pipe_close(&child_pid_report_pipe, X_UNIX_PIPE_END_WRITE, NULL);

                _exit(0);
            }
        } else {
            do_exec(x_unix_pipe_get(&child_err_report_pipe, X_UNIX_PIPE_END_WRITE), stdin_fd, stdout_fd, stderr_fd, source_fds_copy, target_fds, n_fds, working_directory, argv, argv_buffer, argv_buffer_len, envp, close_descriptors, chosen_search_path, search_path_buffer, search_path_buffer_len, stdout_to_null, stderr_to_null, child_inherits_stdin, file_and_argv_zero, child_setup, user_data);
        }
    } else {
        xint buf[2];
        xint n_ints = 0;

        x_unix_pipe_close(&child_err_report_pipe, X_UNIX_PIPE_END_WRITE, NULL);
        x_unix_pipe_close(&child_pid_report_pipe, X_UNIX_PIPE_END_WRITE, NULL);

        if (intermediate_child) {
            wait_again:
            if (waitpid(pid, &status, 0) < 0) {
                if (errno == EINTR) {
                    goto wait_again;
                } else if (errno == ECHILD) {
                    ;
                } else {
                    x_warning("waitpid() should not fail in 'fork_exec'");
                }
            }
        }

        if (!read_ints(x_unix_pipe_get(&child_err_report_pipe, X_UNIX_PIPE_END_READ), buf, 2, &n_ints, error)) {
            goto cleanup_and_fail;
        }

        if (n_ints >= 2) {
            switch (buf[0]) {
                case CHILD_CHDIR_FAILED:
                    x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_CHDIR, _("Failed to change to directory “%s” (%s)"), working_directory, x_strerror(buf[1]));
                    break;

                case CHILD_EXEC_FAILED:
                    x_set_error(error, X_SPAWN_ERROR, _x_spawn_exec_err_to_x_error(buf[1]), _("Failed to execute child process “%s” (%s)"), argv[0], x_strerror(buf[1]));
                    break;

                case CHILD_OPEN_FAILED:
                    x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Failed to open file to remap file descriptor (%s)"), x_strerror(buf[1]));
                    break;

                case CHILD_DUPFD_FAILED:
                    x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Failed to duplicate file descriptor for child process (%s)"), x_strerror(buf[1]));
                    break;

                case CHILD_FORK_FAILED:
                    x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FORK, _("Failed to fork child process (%s)"), x_strerror(buf[1]));
                    break;

                case CHILD_CLOSE_FAILED:
                    x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Failed to close file descriptor for child process (%s)"), x_strerror(buf[1]));
                    break;

                default:
                    x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Unknown error executing child process “%s”"), argv[0]);
                    break;
            }

            goto cleanup_and_fail;
        }

        if (intermediate_child) {
            n_ints = 0;

            if (!read_ints(x_unix_pipe_get(&child_pid_report_pipe, X_UNIX_PIPE_END_READ), buf, 1, &n_ints, error)) {
                goto cleanup_and_fail;
            }

            if (n_ints < 1) {
                int errsv = errno;
                x_set_error(error, X_SPAWN_ERROR, X_SPAWN_ERROR_FAILED, _("Failed to read enough data from child pid pipe (%s)"), x_strerror(errsv));
                goto cleanup_and_fail;
            } else {
                pid = buf[0];
            }
        }

        x_unix_pipe_close(&child_err_report_pipe, X_UNIX_PIPE_END_READ, NULL);
        x_unix_pipe_close(&child_pid_report_pipe, X_UNIX_PIPE_END_READ, NULL);

        x_free(search_path_buffer_heap);
        x_free(argv_buffer_heap);
        x_free(source_fds_copy);

        if (child_pid) {
            *child_pid = pid;
        }

        goto success;
    }

success:
    x_unix_pipe_close(&stdin_pipe, X_UNIX_PIPE_END_READ, NULL);
    x_unix_pipe_close(&stdout_pipe, X_UNIX_PIPE_END_WRITE, NULL);
    x_unix_pipe_close(&stderr_pipe, X_UNIX_PIPE_END_WRITE, NULL);

    if (stdin_pipe_out != NULL) {
        *stdin_pipe_out = x_unix_pipe_steal(&stdin_pipe, X_UNIX_PIPE_END_WRITE);
    }

    if (stdout_pipe_out != NULL) {
        *stdout_pipe_out = x_unix_pipe_steal(&stdout_pipe, X_UNIX_PIPE_END_READ);
    }

    if (stderr_pipe_out != NULL) {
        *stderr_pipe_out = x_unix_pipe_steal(&stderr_pipe, X_UNIX_PIPE_END_READ);
    }

    return TRUE;

cleanup_and_fail:
    if (pid > 0) {
wait_failed:
        if (waitpid(pid, NULL, 0) < 0) {
            if (errno == EINTR) {
                goto wait_failed;
            } else if (errno == ECHILD) {
                ;
            } else {
                x_warning("waitpid() should not fail in 'fork_exec'");
            }
        }
    }

    x_unix_pipe_clear(&stdin_pipe);
    x_unix_pipe_clear(&stdout_pipe);
    x_unix_pipe_clear(&stderr_pipe);
    x_unix_pipe_clear(&child_err_report_pipe);
    x_unix_pipe_clear(&child_pid_report_pipe);

    x_clear_pointer(&search_path_buffer_heap, x_free);
    x_clear_pointer(&argv_buffer_heap, x_free);
    x_clear_pointer(&source_fds_copy, x_free);

    return FALSE;
}

static xboolean script_execute(const xchar *file, xchar **argv, xchar **argv_buffer, xsize argv_buffer_len, xchar **envp)
{
    xsize argc = 0;

    while (argv[argc]) {
        ++argc;
    }

    if (argc + 2 > argv_buffer_len) {
        return FALSE;
    }

    argv_buffer[0] = (char *)"/bin/sh";
    argv_buffer[1] = (char *)file;

    while (argc > 0) {
        argv_buffer[argc + 1] = argv[argc];
        --argc;
    }

    if (envp) {
        execve(argv_buffer[0], argv_buffer, envp);
    } else {
        execv(argv_buffer[0], argv_buffer);
    }

    return TRUE;
}

static xchar *my_strchrnul(const xchar *str, xchar c)
{
    xchar *p = (xchar *)str;
    while (*p && (*p != c)) {
        ++p;
    }

    return p;
}

static xint x_execute(const xchar *file, xchar **argv, xchar **argv_buffer, xsize argv_buffer_len, xchar **envp, const xchar *search_path, xchar *search_path_buffer, xsize search_path_buffer_len)
{
    if (file == NULL || *file == '\0') {
        errno = ENOENT;
        return -1;
    }

    if (search_path == NULL || strchr (file, '/') != NULL) {
        if (envp) {
            execve(file, argv, envp);
        } else {
            execv(file, argv);
        }
        
        if (errno == ENOEXEC && !script_execute(file, argv, argv_buffer, argv_buffer_len, envp)) {
            errno = ENOMEM;
            return -1;
        }
    } else {
        xsize len;
        xchar *name;
        xsize pathlen;
        const xchar *path, *p;
        xboolean got_eacces = 0;

        path = search_path;
        len = strlen(file) + 1;
        pathlen = strlen(path);
        name = search_path_buffer;

        if (search_path_buffer_len < pathlen + len + 1) {
            errno = ENOMEM;
            return -1;
        }

        memcpy(name + pathlen + 1, file, len);
        name = name + pathlen;
        *name = '/';

        p = path;
        do {
            char *startp;

            path = p;
            p = my_strchrnul(path, ':');

            if (p == path) {
                startp = name + 1;
            } else {
                startp = (char *)memcpy(name - (p - path), path, p - path);
            }

            if (envp) {
                execve(startp, argv, envp);
            } else {
                execv(startp, argv);
            }
            
            if (errno == ENOEXEC && !script_execute(startp, argv, argv_buffer, argv_buffer_len, envp)) {
                errno = ENOMEM;
                return -1;
            }

            switch (errno) {
                case EACCES:
                    got_eacces = TRUE;

                X_GNUC_FALLTHROUGH;
                case ENOENT:
#ifdef ESTALE
                case ESTALE:
#endif
#ifdef ENOTDIR
                case ENOTDIR:
#endif
                    break;

                case ENODEV:
                case ETIMEDOUT:
                    break;

                default:
                    return -1;
            }
        } while (*p++ != '\0');

        if (got_eacces) {
            errno = EACCES;
        }
    }

    return -1;
}

void x_spawn_close_pid(XPid pid)
{

}
