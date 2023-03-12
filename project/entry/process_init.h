#ifndef PROCESS_INIT_H
#define PROCESS_INIT_H

#include <stdio.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 函数名称: app_start_run_child
 * 功能描述: 应用程序开始运行(处理各监听事件以及事项)
 * 输入参数: argc --> 命令行参数个数
 *          argv --> 命令行参数列表
 * 输出参数: 无
 * 返回说明: 返回1或0
 */
int app_start_run_child(int argc, char *argv[]);

/**
 * 函数名称: app_start_run_parent
 * 功能描述: 应用程序开始运行(处理各监听事件以及事项)
 * 输入参数: argc --> 命令行参数个数
 *          argv --> 命令行参数列表
 * 输出参数: 无
 * 返回说明: 返回1或0
 */
int app_start_run_parent(int argc, char *argv[]);

/**
 * 函数名称: app_version_header
 * 功能描述: 应用程序版本头
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回<0
 */
int app_version_header(void);

/**
 * 函数名称: catch_signal_name
 * 功能描述: 根据捕获的信号值获取对应的解释
 * 输入参数: signo --> 捕获的信号值
 * 输出参数: 无
 * 返回说明: 返回捕获的信号值对应解释
 */
inline const char *catch_signal_name(int signo)
{
    switch (signo) {
        case SIGHUP:    return "[SIGHUP] Hangup";
        case SIGINT:    return "[SIGINT] Interactive attention signal";
        case SIGQUIT:   return "[SIGQUIT] Quit";
        case SIGILL:    return "[SIGILL] Illegal instruction";
        case SIGTRAP:   return "[SIGTRAP] Trace/breakpoint trap";
        case SIGABRT:   return "[SIGABRT] Abnormal termination";
        case SIGFPE:    return "[SIGFPE] Erroneous arithmetic operation";
        case SIGKILL:   return "[SIGKILL] Killed";
        case SIGBUS:    return "[SIGBUS] Bus error";
        case SIGSEGV:   return "[SIGSEGV] Invalid access to storage";
        case SIGSYS:    return "[SIGSYS] Bad system call";
        case SIGPIPE:   return "[SIGPIPE] Broken pipe";
        case SIGALRM:   return "[SIGALRM] Alarm clock";
        case SIGTERM:   return "[SIGTERM] Termination request";
        case SIGURG:    return "[SIGURG] Urgent data is available at a socket";
        case SIGSTOP:   return "[SIGSTOP] Stop, unblockable";
        case SIGTSTP:   return "[SIGTSTP] Keyboard stop";
        case SIGCONT:   return "[SIGCONT] Continue";
        case SIGCHLD:   return "[SIGCHLD] Child terminated or stopped";
        case SIGTTIN:   return "[SIGTTIN] Background read from control terminal";
        case SIGTTOU:   return "[SIGTTOU] Background write to control terminal";
        case SIGPOLL:   return "[SIGPOLL] Pollable event occurred (System V)";
        case SIGXCPU:   return "[SIGXCPU] CPU time limit exceeded";
        case SIGXFSZ:   return "[SIGXFSZ] File size limit exceeded";
        case SIGVTALRM: return "[SIGVTALRM] Virtual timer expired";
        case SIGPROF:   return "[SIGPROF] Profiling timer expired";
        case SIGWINCH:  return "[SIGWINCH] Window size change (4.3 BSD, Sun)";
        case SIGUSR1:   return "[SIGUSR1] User-defined signal 1";
        case SIGUSR2:   return "[SIGUSR2] User-defined signal 2";
    }
}

#ifdef __cplusplus
}
#endif

#endif
