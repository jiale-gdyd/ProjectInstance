#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_OPEN_LOCK_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_OPEN_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

#include <fcntl.h>

#include "../acl_vstream.h"
#include "../acl_vstring.h"

extern ACL_VSTREAM *acl_open_lock(const char *, int, int, ACL_VSTRING *);

#endif

#ifdef __cplusplus
}
#endif

#endif
