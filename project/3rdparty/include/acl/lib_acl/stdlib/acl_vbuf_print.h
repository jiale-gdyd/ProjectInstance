#ifndef ACL_LIBACL_STDLIB_ACL_VBUF_PRINT_H
#define ACL_LIBACL_STDLIB_ACL_VBUF_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdarg.h>
#include "acl_vbuf.h"

ACL_API ACL_VBUF *acl_vbuf_print(ACL_VBUF *, const char *, va_list);

#ifdef __cplusplus
}
#endif

#endif
