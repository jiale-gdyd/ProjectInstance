#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_MYCHOWN_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_MYCHOWN_H

#ifdef _cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

int acl_mychown(const char *path, const char *s_owner, const char *s_group);
int acl_myfchown(const int fd, const char *s_owner, const char *s_group);

#endif

#ifdef _cplusplus
}
#endif

#endif
