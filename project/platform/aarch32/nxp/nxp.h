#ifndef PLATFORM_AARCH32_NXP_H
#define PLATFORM_AARCH32_NXP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 函数名称: nxp_aarch32_platform_remove
 * 功能描述: NXP平台应用注销
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int nxp_aarch32_platform_remove(void);

/**
 * 函数名称: nxp_aarch32_platform_probe
 * 功能描述: NXP平台应用注册
 * 输入参数: argc --> 输入命令行参数个数
 *          argv --> 输入命令行参数列表
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int nxp_aarch32_platform_probe(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
