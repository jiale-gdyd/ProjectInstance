#ifndef __HOOK_HEAD_H__
#define __HOOK_HEAD_H__

#include "acl/lib_fiber/c/fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef socket_t (WINAPI *socket_fn)(int, int, int);
typedef int (WINAPI *close_fn)(socket_t);
typedef int (WINAPI *listen_fn)(socket_t, int);
typedef socket_t (WINAPI *accept_fn)(socket_t, struct sockaddr *, socklen_t *);
typedef int (WINAPI *connect_fn)(socket_t, const struct sockaddr *, socklen_t);

typedef int (WINAPI *getaddrinfo_fn)(const char *node, const char *service,
    const struct addrinfo* hints, struct addrinfo **res);
typedef void (WINAPI *freeaddrinfo_fn)(struct addrinfo *res);
typedef struct hostent *(WINAPI *gethostbyname_fn)(const char *);

#if defined(_WIN32) || defined(_WIN64)

typedef int (WINAPI *recv_fn)(socket_t, char *, int, int);
typedef int (WINAPI *recvfrom_fn)(socket_t, char *, int, int,
    struct sockaddr *, socklen_t *);
typedef int (WINAPI *send_fn)(socket_t, const char *, int, int);
typedef int (WINAPI *sendto_fn)(socket_t, const char *, int, int,
    const struct sockaddr *, socklen_t);
typedef int (WINAPI *poll_fn)(struct pollfd *, nfds_t, int);
typedef int (WINAPI *select_fn)(int, fd_set *, fd_set *,
    fd_set *, const struct timeval *);

typedef int (WSAAPI *WSARecv_fn)(socket_t, LPWSABUF, DWORD, LPDWORD, LPDWORD,
    LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef socket_t (WSAAPI *WSAAccept_fn)(SOCKET, struct sockaddr FAR *,
    LPINT, LPCONDITIONPROC, DWORD_PTR);

#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(MINGW)

typedef int (*fcntl_fn)(int, int, ...);
typedef int (*setsockopt_fn)(socket_t, int, int, const void *, socklen_t);
typedef unsigned (*sleep_fn)(unsigned int seconds);
typedef ssize_t  (*read_fn)(socket_t, void *, size_t);
typedef ssize_t  (*readv_fn)(socket_t, const struct iovec *, int);
typedef ssize_t  (*recv_fn)(socket_t, void *, size_t, int);
typedef ssize_t  (*recvfrom_fn)(socket_t, void *, size_t, int,
    struct sockaddr *, socklen_t *);
typedef ssize_t  (*recvmsg_fn)(socket_t, struct msghdr *, int);
typedef ssize_t  (*write_fn)(socket_t, const void *, size_t);
typedef ssize_t  (*writev_fn)(socket_t, const struct iovec *, int);
typedef ssize_t  (*send_fn)(socket_t, const void *, size_t, int);
typedef ssize_t  (*sendto_fn)(socket_t, const void *, size_t, int,
    const struct sockaddr *, socklen_t);
typedef ssize_t  (*sendmsg_fn)(socket_t, const struct msghdr *, int);

# ifdef HAS_MMSG
typedef int (*recvmmsg_fn)(int, struct mmsghdr *, unsigned int,
    int, const struct timespec *);
typedef int (*sendmmsg_fn)(int, struct mmsghdr *, unsigned int, int);
# endif

# ifdef  __USE_LARGEFILE64
typedef ssize_t  (*sendfile64_fn)(socket_t, int, off64_t*, size_t);
# endif

typedef int (*poll_fn)(struct pollfd *, nfds_t, int);
typedef int (*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);

# ifdef	HAS_EPOLL
typedef int (*epoll_create_fn)(int);
typedef int (*epoll_wait_fn)(int, struct epoll_event *,int, int);
typedef int (*epoll_ctl_fn)(int, int, int, struct epoll_event *);
# endif

# ifdef HAS_IO_URING
typedef int (*openat_fn)(int, const char *, int, mode_t);
typedef int (*unlink_fn)(const char *);

#  ifdef HAS_STATX
typedef int (*statx_fn)(int dirfd, const char *, int, unsigned int, struct statx *);
#  endif

#  ifdef HAS_RENAMEAT2
typedef int (*renameat2_fn)(int, const char *, int, const char *, unsigned);
#  endif

typedef int (*mkdirat_fn)(int, const char *, mode_t);
typedef ssize_t (*splice_fn)(int, loff_t *, int, loff_t *, size_t, unsigned);
# endif

# ifndef __APPLE__
typedef int (*gethostbyname_r_fn)(const char *, struct hostent *, char *,
    size_t, struct hostent **, int *);
# endif

typedef size_t (*pread_fn)(int, void *, size_t, off_t);
typedef ssize_t (*pwrite_fn)(int, const void *, size_t, off_t);

#endif  // __linux__, __APPLE__, __FreeBSD__, MINGW

// in hook.c
FIBER_API void WINAPI set_socket_fn(socket_fn *fn);
FIBER_API void WINAPI set_close_fn(close_fn *fn);
FIBER_API void WINAPI set_listen_fn(listen_fn *fn);
FIBER_API void WINAPI set_accept_fn(accept_fn *fn);
FIBER_API void WINAPI set_connect_fn(connect_fn *fn);
FIBER_API void WINAPI set_recv_fn(recv_fn *fn);
FIBER_API void WINAPI set_recvfrom_fn(recvfrom_fn *fn);
FIBER_API void WINAPI set_send_fn(send_fn *fn);
FIBER_API void WINAPI set_sendto_fn(sendto_fn *fn);
FIBER_API void WINAPI set_poll_fn(poll_fn *fn);
FIBER_API void WINAPI set_select_fn(select_fn *fn);
FIBER_API void WINAPI set_getaddrinfo_fn(getaddrinfo_fn *fn);
FIBER_API void WINAPI set_freeaddrinfo_fn(freeaddrinfo_fn *fn);
FIBER_API void WINAPI set_gethostbyname_fn(gethostbyname_fn *fn);

#if defined(_WIN32) || defined(_WIN64)
FIBER_API void WINAPI set_WSARecv_fn(WSARecv_fn *fn);
FIBER_API void WINAPI set_WSAAccept_fn(WSAAccept_fn *fn);
#endif

extern socket_fn            *sys_socket;
extern close_fn             *sys_close;
extern listen_fn            *sys_listen;
extern accept_fn            *sys_accept;
extern connect_fn           *sys_connect;

extern recv_fn              *sys_recv;

extern recvfrom_fn          *sys_recvfrom;

extern send_fn              *sys_send;
extern sendto_fn            *sys_sendto;
extern poll_fn              *sys_poll;
extern select_fn            *sys_select;

extern getaddrinfo_fn       *sys_getaddrinfo;
extern freeaddrinfo_fn      *sys_freeaddrinfo;
extern gethostbyname_fn     *sys_gethostbyname;

#if defined(_WIN32) || defined(_WIN64)

extern WSARecv_fn           *sys_WSARecv;
extern WSAAccept_fn         *sys_WSAAccept;

#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)  || defined(MINGW) // SYS_UNIX

extern fcntl_fn             *sys_fcntl;
extern setsockopt_fn        *sys_setsockopt;
extern sleep_fn             *sys_sleep;

extern read_fn              *sys_read;
extern readv_fn             *sys_readv;
extern recvmsg_fn           *sys_recvmsg;

extern write_fn             *sys_write;
extern writev_fn            *sys_writev;
extern sendmsg_fn           *sys_sendmsg;

# ifdef HAS_MMSG
extern recvmmsg_fn          *sys_recvmmsg;
extern sendmmsg_fn          *sys_sendmmsg;
# endif

# ifdef __USE_LARGEFILE64
extern sendfile64_fn        *sys_sendfile64;
# endif

# ifdef	HAS_EPOLL
extern epoll_create_fn      *sys_epoll_create;
extern epoll_wait_fn        *sys_epoll_wait;
extern epoll_ctl_fn         *sys_epoll_ctl;
# endif

# ifdef HAS_IO_URING
extern openat_fn            *sys_openat;
extern unlink_fn            *sys_unlink;
# ifdef HAS_STATX
extern statx_fn             *sys_statx;
# endif
# ifdef HAS_RENAMEAT2
extern renameat2_fn         *sys_renameat2;
# endif
extern mkdirat_fn           *sys_mkdirat;
extern splice_fn            *sys_splice;
# endif // HAS_IO_URING

# ifndef __APPLE__
extern gethostbyname_r_fn   *sys_gethostbyname_r;
# endif

extern pread_fn             *sys_pread;
extern pwrite_fn            *sys_pwrite;

#endif // SYS_UNIX

// in hook.c
void hook_once(void);

#if defined(__linux__)
// in epoll.c
int epoll_try_register(int epfd);
int epoll_close(int epfd);
#endif

#ifdef __cplusplus
}
#endif

#endif
