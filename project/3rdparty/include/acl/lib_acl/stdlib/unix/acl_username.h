#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_USERNAME_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_USERNAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

const char *acl_username(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
