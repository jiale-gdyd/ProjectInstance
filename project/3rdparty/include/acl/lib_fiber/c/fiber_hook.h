#ifndef ACL_LIB_FIBER_FIBER_HOOK_H
#define ACL_LIB_FIBER_FIBER_HOOK_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

FIBER_API socket_t acl_fiber_socket(int domain, int type, int protocol);
FIBER_API int acl_fiber_listen(socket_t, int backlog);
FIBER_API int acl_fiber_close(socket_t fd);
FIBER_API socket_t acl_fiber_accept(socket_t , struct sockaddr *, socklen_t *);
FIBER_API int acl_fiber_connect(socket_t , const struct sockaddr *, socklen_t );
FIBER_API ssize_t acl_fiber_read(socket_t, void* buf, size_t count);
FIBER_API ssize_t acl_fiber_readv(socket_t, const struct iovec* iov, int iovcnt);
FIBER_API ssize_t acl_fiber_recvmsg(socket_t, struct msghdr* msg, int flags);

# ifdef HAS_MMSG
FIBER_API int acl_fiber_recvmmsg(int sockfd, struct mmsghdr *msgvec,
    unsigned int vlen, int flags, const struct timespec *timeout);
FIBER_API int acl_fiber_sendmmsg(int sockfd, struct mmsghdr *msgvec,
    unsigned int vlen, int flags);
# endif

FIBER_API ssize_t acl_fiber_write(socket_t, const void* buf, size_t count);
FIBER_API ssize_t acl_fiber_writev(socket_t, const struct iovec* iov, int iovcnt);
FIBER_API ssize_t acl_fiber_sendmsg(socket_t, const struct msghdr* msg, int flags);

FIBER_API ssize_t acl_fiber_recv(socket_t, void* buf, size_t len, int flags);
FIBER_API ssize_t acl_fiber_recvfrom(socket_t, void* buf, size_t len, int flags,
    struct sockaddr* src_addr, socklen_t* addrlen);

FIBER_API ssize_t acl_fiber_send(socket_t, const void* buf, size_t len, int flags);
FIBER_API ssize_t acl_fiber_sendto(socket_t, const void* buf, size_t len, int flags,
    const struct sockaddr* dest_addr, socklen_t addrlen);

FIBER_API int acl_fiber_select(int nfds, fd_set *readfds, fd_set *writefds,
    fd_set *exceptfds, struct timeval *timeout);
FIBER_API int acl_fiber_poll(struct pollfd *fds, nfds_t nfds, int timeout);

FIBER_API struct hostent *acl_fiber_gethostbyname(const char *name);
FIBER_API int acl_fiber_gethostbyname_r(const char *name, struct hostent *ent,
    char *buf, size_t buflen, struct hostent **result, int *h_errnop);
FIBER_API int acl_fiber_getaddrinfo(const char *node, const char *service,
    const struct addrinfo* hints, struct addrinfo **res);
FIBER_API void acl_fiber_freeaddrinfo(struct addrinfo *res);

FIBER_API void acl_fiber_set_sysio(socket_t fd);

#ifdef __cplusplus
}
#endif

#endif
