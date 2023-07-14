#ifndef ACL_LIB_FIBER_FIBER_DEFINE_H
#define ACL_LIB_FIBER_FIBER_DEFINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef intptr_t acl_handle_t;

# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif

# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/select.h>
# include <poll.h>
# include <unistd.h>
# include <netdb.h>

# define INVALID_SOCKET	-1
typedef int socket_t;

# define FIBER_ETIMEDOUT        ETIMEDOUT
# define FIBER_ETIME            ETIMEDOUT
//# define FIBER_ETIME          ETIME
# define FIBER_ENOMEM           ENOMEM
# define FIBER_EINVAL           EINVAL
# define FIBER_ECONNREFUSED     ECONNREFUSED
# define FIBER_ECONNRESET       ECONNRESET
# define FIBER_EHOSTDOWN        EHOSTDOWN
# define FIBER_EHOSTUNREACH     EHOSTUNREACH
# define FIBER_EINTR            EINTR
# define FIBER_EAGAIN           EAGAIN
# define FIBER_ENETDOWN         ENETDOWN
# define FIBER_ENETUNREACH      ENETUNREACH
# define FIBER_ENOTCONN         ENOTCONN
# define FIBER_EISCONN          EISCONN
# define FIBER_EWOULDBLOCK      EWOULDBLOCK
# define FIBER_ENOBUFS          ENOBUFS
# define FIBER_ECONNABORTED     ECONNABORTED
# define FIBER_EINPROGRESS      EINPROGRESS

# include <sys/syscall.h>
# if defined(SYS_recvmmsg) && defined(SYS_sendmmsg) && !defined(ANDROID)
#  define HAS_MMSG
# endif

#ifdef FIBER_LIB
# ifndef FIBER_API
#  define FIBER_API
# endif
#elif defined(FIBER_DLL) // || defined(_WINDLL)
# if defined(FIBER_EXPORTS) || defined(fiber_EXPORTS)
#  ifndef FIBER_API
#   define FIBER_API __declspec(dllexport)
#  endif
# elif !defined(FIBER_API)
#  define FIBER_API __declspec(dllimport)
# endif
#elif !defined(FIBER_API)
# define FIBER_API
#endif

/**
 * The fiber struct type definition
 */
typedef struct ACL_FIBER ACL_FIBER;
typedef unsigned int acl_fiber_t;

#ifdef __cplusplus
}
#endif

#endif
