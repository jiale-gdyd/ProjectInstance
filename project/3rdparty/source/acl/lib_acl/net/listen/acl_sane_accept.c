/* System library. */
#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"
#ifdef HP_UX
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED  1
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef ACL_UNIX
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/net/acl_tcp_ctl.h"
#include "acl/lib_acl/net/acl_sane_inet.h"
#include "acl/lib_acl/net/acl_sane_socket.h"
#include "acl/lib_acl/net/acl_listen.h"

#endif

static acl_accept_fn __sys_accept = accept;

void acl_set_accept(acl_accept_fn fn)
{
    __sys_accept = fn;
}

/* acl_sane_accept - sanitize accept() error returns */

ACL_SOCKET acl_sane_accept(ACL_SOCKET sock, struct sockaddr * sa, socklen_t *len)
{
    static int accept_ok_errors[] = {
        ACL_EAGAIN,
        ACL_ECONNREFUSED,
        ACL_ECONNRESET,
        ACL_EHOSTDOWN,
        ACL_EHOSTUNREACH,
        ACL_EINTR,
        ACL_ENETDOWN,
        ACL_ENETUNREACH,
        ACL_ENOTCONN,
        ACL_EWOULDBLOCK,
        ACL_ENOBUFS,        /* HPUX11 */
        ACL_ECONNABORTED,
        0,
    };
    ACL_SOCKET fd;

    /*
    * XXX Solaris 2.4 accept() returns EPIPE when a UNIX-domain client
    * has disconnected in the mean time. From then on, UNIX-domain
    * sockets are hosed beyond recovery. There is no point treating
    * this as a beneficial error result because the program would go
    * into a tight loop.
    * XXX LINUX < 2.1 accept() wakes up before the three-way handshake is
    * complete, so it can fail with ECONNRESET and other "false alarm"
    * indications.
    * 
    * XXX FreeBSD 4.2-STABLE accept() returns ECONNABORTED when a
    * UNIX-domain client has disconnected in the mean time. The data
#endif
    * that was sent with connect() write() close() is lost, even though
    * the write() and close() reported successful completion.
    * This was fixed shortly before FreeBSD 4.3.
    * 
    * XXX HP-UX 11 returns ENOBUFS when the client has disconnected in
    * the mean time.
    */

#if defined(_WIN32) || defined(_WIN64)
# ifdef USE_WSASOCK
    fd = __sys_accept(sock, (struct sockaddr *) sa, (socklen_t *) len, 0, 0);
# else
    fd = __sys_accept(sock, (struct sockaddr *) sa, (socklen_t *) len);
    //fd = WSAAccept(sock, (struct sockaddr *) sa, (socklen_t *) len, 0, 0);
# endif
#else
    fd = __sys_accept(sock, (struct sockaddr *) sa, (socklen_t *) len);
#endif

    if (fd == ACL_SOCKET_INVALID) {
        int  count = 0, err, error = acl_last_error();
        for (; (err = accept_ok_errors[count]) != 0; count++) {
            if (error == err) {
                acl_set_error(ACL_EAGAIN);
                break;
            }
        }
    }

    /*
    * XXX Solaris select() produces false read events, so that read()
    * blocks forever on a blocking socket, and fails with EAGAIN on
    * a non-blocking socket. Turning on keepalives will fix a blocking
    * socket provided that the kernel's keepalive timer expires before
    * the Postfix watchdog timer.
    */
#ifdef AF_INET6
    else if (sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6))
#else
    else if (sa && sa->sa_family == AF_INET)
#endif
    {
        int on = 1;

        /* default set client to nodelay --- add by zsx, 2008.9.4 */
        acl_tcp_nodelay(fd, on);

#if defined(BROKEN_READ_SELECT_ON_TCP_SOCKET) && defined(SO_KEEPALIVE)
        (void) setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
            (char *) &on, sizeof(on));
#endif
    }

    return fd;
}

ACL_SOCKET acl_accept(ACL_SOCKET sock, char *buf, size_t size, int* sock_type)
{
    ACL_SOCKADDR addr;
    socklen_t len = sizeof(addr);
    struct sockaddr *sa = (struct sockaddr*) &addr;
    ACL_SOCKET fd;

    memset(&addr, 0, sizeof(addr));

    fd = acl_sane_accept(sock, sa, &len);
    if (fd == ACL_SOCKET_INVALID)
        return fd;

    if (sock_type != NULL)
        *sock_type = sa->sa_family;

    if (buf == NULL || size == 0)
        return fd;

    buf[0] = 0;

#ifdef	ACL_UNIX
    if (sa->sa_family == AF_UNIX) {
        if (acl_getsockname(fd, buf, size) < 0) {
            buf[0] = 0;
            acl_msg_error("%s(%d): getsockname error=%s",
                __FUNCTION__, __LINE__, acl_last_serror());
        }
        return fd;
    }
#endif
    /* There're some bug in accept on Windows sock lib, the sa->samily is set 0,
    * which will cause the acl_inet_ntop() failed, so we just try use another
    * way by calling acl_getpeername() to get the peer addr of remote socket.
    * ---zsx, 2021.8.23
    */
    if (acl_inet_ntop(sa, buf, size) == 0) {
        if (acl_getpeername(fd, buf, size) == -1) {
            acl_msg_error("%s(%d): getpeername error=%s",
                __FUNCTION__, __LINE__, acl_last_serror());
        }
    }
    return fd;
}
