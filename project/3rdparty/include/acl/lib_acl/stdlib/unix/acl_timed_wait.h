#ifndef ACL_LIBACL_STDLIB_UNIX_ACL_TIMED_WAIT_H
#define ACL_LIBACL_STDLIB_UNIX_ACL_TIMED_WAIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

#include <sys/types.h>
#include <unistd.h>

extern int acl_timed_waitpid(pid_t, ACL_WAIT_STATUS_T *, int, int);

#endif

#ifdef __cplusplus
}
#endif

#endif
