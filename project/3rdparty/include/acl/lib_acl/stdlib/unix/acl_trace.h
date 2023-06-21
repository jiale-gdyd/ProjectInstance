#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_TRACE_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 将当前函数的堆栈输出至指定文件中
 * @param filepath {const char*} 目标文件名
 */
void acl_trace_save(const char *filepath);

/**
 * 将当前函数的堆栈输出至日志中
 */
void acl_trace_info(void);

#ifdef __cplusplus
}
#endif

#endif
