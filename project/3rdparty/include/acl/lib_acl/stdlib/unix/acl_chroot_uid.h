#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_CHROOT_UID_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_CHROOT_UID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

extern int acl_chroot_uid(const char *, const char *);

#endif

#ifdef  __cplusplus
}
#endif

#endif
