#ifndef ACL_LIB_ACL_STDLIB_SYS_SNPRINTF_H
#define ACL_LIB_ACL_STDLIB_SYS_SNPRINTF_H

#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int acl_secure_snprintf(char *buf, size_t size, const char *fmt, ...);
int acl_secure_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif

#endif
