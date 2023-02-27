#include <linux/kconfig.h>
#include "process_init.h"

#if defined(CONFIG_UNITTEST)
#include <unittest/unittest.h>
#endif

#if defined(CONFIG_LIBDRM_TOOLS)
#include <libdrm/libdrm.h>
#endif

#if defined(CONFIG_GUI)
#include "gui_demo/gui_demo.h"
#endif

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

#if defined(CONFIG_GUI)
    gui_demo_main(argc, argv);
    ret = gui_demo_exit();
#elif defined(CONFIG_UNITTEST)
    unittest_init(argc, argv);
    ret = unittest_exit();
#elif defined(CONFIG_LIBDRM_TOOLS)
#if defined(CONFIG_LIBDRM_MODETEST)
    ret = libdrm_modetest_main(argc, argv);
#elif defined(CONFIG_LIBDRM_MODEPRINT)
    ret = libdrm_modeprint_main(argc, argv);
#endif
#else
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
#endif

    return ret;
}
