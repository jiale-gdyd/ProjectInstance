#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>

#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xutils.h>
#include <xlib/xlib/xunicode.h>
#include <xlib/xlib/xbacktrace.h>
#include <xlib/xlib/xlib-unixprivate.h>

static void stack_trace(const char *const *args);

#ifdef USE_LLDB
#define DEBUGGER            "lldb"
#else
#define DEBUGGER            "gdb"
#endif

#define BUFSIZE             1024

XLIB_AVAILABLE_IN_ALL volatile xboolean xlib_on_error_halt;
volatile xboolean xlib_on_error_halt = TRUE;

void x_on_error_query (const xchar *prg_name)
{
    xchar buf[16];
    static const xchar *const query1 = "[E]xit, [H]alt";
    static const xchar *const query2 = ", show [S]tack trace";
    static const xchar *const query3 = " or [P]roceed";

    if (!prg_name) {
        prg_name = x_get_prgname ();
    }

retry:
    if (prg_name) {
        fprintf(stdout, "%s (pid:%u): %s%s%s: ", prg_name, (xuint)getpid(), query1, query2, query3);
    } else {
        fprintf(stdout, "(process:%u): %s%s: ", (xuint)getpid(), query1, query3);
    }
    fflush(stdout);

    if (isatty(0) && isatty(1)) {
        if (fgets(buf, 8, stdin) == NULL) {
            _exit(0);
        }
    } else {
        strcpy(buf, "E\n");
    }

    if ((buf[0] == 'E' || buf[0] == 'e') && buf[1] == '\n') {
        _exit(0);
    } else if ((buf[0] == 'P' || buf[0] == 'p') && buf[1] == '\n') {
        return;
    } else if (prg_name && (buf[0] == 'S' || buf[0] == 's') && buf[1] == '\n') {
        x_on_error_stack_trace(prg_name);
        goto retry;
    } else if ((buf[0] == 'H' || buf[0] == 'h') && buf[1] == '\n') {
        while (xlib_on_error_halt);

        xlib_on_error_halt = TRUE;
        return;
    } else {
        goto retry;
    }
}

void x_on_error_stack_trace(const xchar *prg_name)
{
    pid_t pid;
    int status;
    xchar buf[16];
    const xchar *args[5] = { DEBUGGER, NULL, NULL, NULL, NULL };

    if (!prg_name) {
        return;
    }

    sprintf(buf, "%u", (xuint)getpid());

#ifdef USE_LLDB
    args[1] = prg_name;
    args[2] = "-p";
    args[3] = buf;
#else
    args[1] = prg_name;
    args[2] = buf;
#endif

    pid = fork ();
    if (pid == 0) {
        stack_trace(args);
        _exit(0);
    } else if (pid == (pid_t)-1) {
        perror("unable to fork " DEBUGGER);
        return;
    }

    while (1) {
        pid_t retval = waitpid(pid, &status, 0);
        if (WIFEXITED(retval) || WIFSIGNALED(retval)) {
            break;
        }
    }
}

static xboolean stack_trace_done = FALSE;

static void stack_trace_sigchld(int signum)
{
    stack_trace_done = TRUE;
}

static inline const char *get_strerror(char *buffer, xsize n)
{
#if defined(STRERROR_R_CHAR_P)
    return (const char *)strerror_r(errno, buffer, n);
#elif defined(HAVE_STRERROR_R)
    int ret = strerror_r(errno, buffer, n);
    if (ret == 0 || ret == EINVAL) {
        return buffer;
    }

    return NULL;
#else
    const char *error_str = strerror(errno);
    if (!error_str) {
        return NULL;
    }

    strncpy(buffer, error_str, n);
    return buffer;
#endif
}

static xssize checked_write(int fd, xconstpointer buf, xsize n)
{
    xssize written = write(fd, buf, n);

    if (written == -1) {
        char msg[BUFSIZE] = {0};
        char error_str[BUFSIZE / 2] = {0};

        get_strerror(error_str, sizeof(error_str) - 1);
        snprintf(msg, sizeof(msg) - 1, "Unable to write to fd %d: %s", fd, error_str);
        perror(msg);
        _exit(0);
    }

    return written;
}

static int checked_dup(int fd)
{
    int new_fd = dup(fd);

    if (new_fd == -1) {
        char msg[BUFSIZE] = {0};
        char error_str[BUFSIZE / 2] = {0};

        get_strerror(error_str, sizeof(error_str) - 1);
        snprintf(msg, sizeof(msg) - 1, "Unable to duplicate fd %d: %s", fd, error_str);
        perror(msg);
        _exit(0);
    }

    return new_fd;
}

static void stack_trace(const char *const *args)
{
    char c;
    pid_t pid;
    fd_set fdset;
    int in_fd[2];
    int out_fd[2];
    fd_set readset;
    struct timeval tv;
    int sel, idx, state;
#ifdef USE_LLDB
    int line_idx;
#endif
    char buffer[BUFSIZE];

    stack_trace_done = FALSE;
    signal(SIGCHLD, stack_trace_sigchld);

    if (!x_unix_open_pipe_internal(in_fd, TRUE) || !x_unix_open_pipe_internal(out_fd, TRUE)) {
        perror("unable to open pipe");
        _exit(0);
    }

    pid = fork();
    if (pid == 0) {
        int old_err = dup(2);
        if (old_err != -1) {
            int getfd = fcntl(old_err, F_GETFD);
            if (getfd != -1) {
                (void)fcntl(old_err, F_SETFD, getfd | FD_CLOEXEC);
            }
        }

        close(0);
        checked_dup(in_fd[0]);
        close(1);
        checked_dup(out_fd[1]);
        close(2);
        checked_dup(out_fd[1]);

        execvp(args[0], (char **)args);

        if (old_err != -1) {
            close(2);
            (void)!dup(old_err);
        }

        perror("exec " DEBUGGER " failed");
        _exit(0);
    } else if (pid == (pid_t)-1) {
        perror("unable to fork");
        _exit(0);
    }

    FD_ZERO(&fdset);
    FD_SET(out_fd[0], &fdset);

#ifdef USE_LLDB
    checked_write(in_fd[1], "bt\n", 3);
    checked_write(in_fd[1], "p x = 0\n", 8);
    checked_write(in_fd[1], "process detach\n", 15);
    checked_write(in_fd[1], "quit\n", 5);
#else
    checked_write(in_fd[1], "set width unlimited\n", 20);
    checked_write(in_fd[1], "backtrace\n", 10);
    checked_write(in_fd[1], "p x = 0\n", 8);
    checked_write(in_fd[1], "quit\n", 5);
#endif

    idx = 0;
#ifdef USE_LLDB
    line_idx = 0;
#endif
    state = 0;

    while (1) {
        readset = fdset;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        sel = select(FD_SETSIZE, &readset, NULL, NULL, &tv);
        if (sel == -1) {
            break;
        }

        if ((sel > 0) && (FD_ISSET(out_fd[0], &readset))) {
            if (read(out_fd[0], &c, 1)) {
#ifdef USE_LLDB
                line_idx += 1;
#endif

                switch (state) {
                    case 0:
#ifdef USE_LLDB
                        if (c == '*' || (c == ' ' && line_idx == 1))
#else
                        if (c == '#')
#endif
                        {
                            state = 1;
                            idx = 0;
                            buffer[idx++] = c;
                        }
                        break;

                    case 1:
                        if (idx < BUFSIZE) {
                            buffer[idx++] = c;
                        }

                        if ((c == '\n') || (c == '\r')) {
                            buffer[idx] = 0;
                            fprintf(stdout, "%s", buffer);
                            state = 0;
                            idx = 0;
#ifdef USE_LLDB
                            line_idx = 0;
#endif
                        }
                        break;

                    default:
                        break;
                }
            }
        } else if (stack_trace_done) {
            break;
        }
    }

    close(in_fd[0]);
    close(in_fd[1]);
    close(out_fd[0]);
    close(out_fd[1]);
    _exit(0);
}
