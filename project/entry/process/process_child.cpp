#include <process_init.h>

/**
 * 函数名称: app_module_init
 * 功能描述: 应用模块初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回<0
 */
static int app_module_init(int argc, char *argv[])
{
    int ret = 0;

    /* your application init code here */

    if (ret != 0) {
        printf("\033[1;31mapp_module_init failed, return:[%d]\033[0m\n", ret);
    }

    return 0;
}

/**
 * 函数名称: app_module_exit
 * 功能描述: 应用模块资源释放
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，失败返回<0
 */
static int app_module_exit(void)
{
    /* your application exit code here */

    return 0;
}

/**
 * 函数名称: app_start_run_child
 * 功能描述: 应用程序开始运行(处理各监听事件以及事项)
 * 输入参数: argc --> 命令行参数个数
 *          argv --> 命令行参数列表
 * 输出参数: 无
 * 返回说明: 返回1或0
 */
int app_start_run_child(int argc, char *argv[])
{
    app_module_init(argc, argv);
    return app_module_exit();
}
