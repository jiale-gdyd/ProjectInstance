#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_SAFE_OPEN_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_SAFE_OPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

#include <sys/stat.h>
#include <fcntl.h>

#include "../acl_vstream.h"
#include "../acl_vstring.h"

extern ACL_VSTREAM *acl_safe_open(const char *path, int flags, int mode, struct stat * st, uid_t user, gid_t group, ACL_VSTRING *why);

#endif

#ifdef  __cplusplus
}
#endif

#endif
