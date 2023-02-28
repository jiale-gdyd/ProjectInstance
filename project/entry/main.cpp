#include <linux/kconfig.h>
#include "process_init.h"

/**
 * 函数名称: main
 * 功能描述: 应用进程启动连接主入口
 * 输入参数: argc --> 命令行参数个数
 *          argv --> 命令行参数列表
 * 输出参数: 无
 * 返回说明: 返回0或1
 */
int main(int argc, char *argv[])
{
    int ret = 0;
    app_version_header();

#if defined(CONFIG_CHILD_PROCESS)
    int status;
    pid_t w, pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        ret = app_start_run_child(argc, argv);
    }
#endif

#if defined(CONFIG_PARENT_PROCESS)
    ret = app_start_run_parent(argc, argv);
#endif

#if defined(CONFIG_CHILD_PROCESS)
    do {
        w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("exited, status:[%d]\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal:[%d]\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal:[%d]\n", WSTOPSIG(status));
        } else if (WIFCONTINUED(status)) {
            printf("continued\n");
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
#endif

    return ret;
}
