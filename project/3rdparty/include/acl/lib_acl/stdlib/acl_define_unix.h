#ifndef ACL_LIBACL_STDLIB_ACL_DEFINE_UNIX_H
#define ACL_LIBACL_STDLIB_ACL_DEFINE_UNIX_H

#include "acl_define_linux.h"

#ifdef ACL_UNIX

#ifndef PATH_SEP_C
#define PATH_SEP_C                      '/'
#endif

#ifndef PATH_SEP_S
#define PATH_SEP_S                      "/"
#endif

#define ACL_HAS_PTHREAD

#endif

#ifdef ACL_UNIX

#include <assert.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/select.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include <netdb.h>
#undef _GNU_SOURCE
#endif

#ifndef acl_assert
#define acl_assert                      assert
#endif

#define ACL_API

#define ACL_ETIMEDOUT                   ETIMEDOUT

#define ACL_ETIME                       ETIME
#define ACL_ENOMEM                      ENOMEM
#define ACL_EINVAL                      EINVAL

#define ACL_ECONNREFUSED                ECONNREFUSED
#define ACL_ECONNRESET                  ECONNRESET
#define ACL_EHOSTDOWN                   EHOSTDOWN
#define ACL_EHOSTUNREACH                EHOSTUNREACH
#define ACL_EINTR                       EINTR
#define ACL_EAGAIN                      EAGAIN
#define ACL_ENETDOWN                    ENETDOWN
#define ACL_ENETUNREACH                 ENETUNREACH
#define ACL_ENOTCONN                    ENOTCONN
#define ACL_EISCONN                     EISCONN
#define ACL_EWOULDBLOCK                 EWOULDBLOCK
#define ACL_ENOBUFS                     ENOBUFS
#define ACL_ECONNABORTED                ECONNABORTED
#define ACL_EINPROGRESS                 EINPROGRESS
#define ACL_EMFILE                      EMFILE

#define ACL_SOCKET                      int
#define ACL_FILEFD                      int
#define ACL_SOCKET_INVALID              (int)-1
#define ACL_FILE_HANDLE                 int
#define ACL_FILE_INVALID                (int)-1
#define ACL_DLL_HANDLE                  void*
#define ACL_DLL_FARPROC                 void*

#define acl_int64                       long long int
#define acl_uint64                      unsigned long long int
#define ACL_FMT_I64D                    "%lld"
#define ACL_FMT_I64U                    "%llu"

#define ACL_PATH_BSHELL                 "/bin/sh"

#endif

#endif
