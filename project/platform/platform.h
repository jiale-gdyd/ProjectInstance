#ifndef PLATFORM_PLATFORM_H
#define PLATFORM_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 函数名称: platform_remove
 * 功能描述: 平台应用注销
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int platform_remove(void);

/**
 * 函数名称: platform_probe
 * 功能描述: 平台应用注册
 * 输入参数: argc --> 输入命令行参数个数
 *          argv --> 输入命令行参数列表
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int platform_probe(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
