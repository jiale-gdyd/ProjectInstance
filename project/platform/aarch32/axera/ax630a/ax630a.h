#ifndef PLATFORM_AARCH32_AXERA_AX630A_H
#define PLATFORM_AARCH32_AXERA_AX630A_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 函数名称: axera_ax630a_app_remove
 * 功能描述: AXERA平台应用注销
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int axera_ax630a_app_remove(void);

/**
 * 函数名称: axera_ax630a_app_probe
 * 功能描述: AXERA平台应用注册
 * 输入参数: argc --> 输入命令行参数个数
 *          argv --> 输入命令行参数列表
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int axera_ax630a_app_probe(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
