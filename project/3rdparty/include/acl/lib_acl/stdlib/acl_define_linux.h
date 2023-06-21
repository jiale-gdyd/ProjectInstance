#ifndef ACL_LIBACL_STDLIB_ACL_DEFINE_LINUX_H
#define ACL_LIBACL_STDLIB_ACL_DEFINE_LINUX_H

#ifndef ACL_LINUX
#define ACL_LINUX
#endif

#ifndef ACL_UNIX
#define ACL_UNIX
#endif

#include <stddef.h>

#ifndef _GNU_SOURCE
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS                   64
#endif
#endif

#define ACL_USE_PATHS_H
#define ACL_HAS_FLOCK_LOCK

#define ACL_HAS_FCNTL_LOCK
#define ACL_INTERNAL_LOCK                   ACL_FLOCK_STYLE_FLOCK
#define ACL_ROOT_PATH                       "/bin:/usr/bin:/sbin:/usr/sbin"
#define ACL_PATH_MAILDIR                    "/var/mail"
#define ACL_PATH_BSHELL                     "/bin/sh"
#define ACL_PATH_DEFPATH                    "/usr/bin"
#define ACL_PATH_STDPATH                    "/usr/bin:/usr/sbin"

#if defined(ACL_HAVE_NO_STAT64)
#define acl_stat                            stat
#define acl_fstat                           fstat
#else
#define acl_stat                            stat64
#define acl_fstat                           fstat64
#endif

#ifndef ACL_WAIT_STATUS_T
typedef int ACL_WAIT_STATUS_T;
#define ACL_NORMAL_EXIT_STATUS(status)      !(status)
#endif

#define ACL_FIONREAD_IN_TERMIOS_H
#define ACL_HAVE_NO_RWLOCK

#endif
