#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/net/acl_tcp_ctl.h"
#include "acl/lib_acl/net/acl_connect.h"

#endif

static acl_connect_fn __sys_connect = connect;

void acl_set_connect(acl_connect_fn fn)
{
    __sys_connect = fn;
}

/* acl_sane_connect - sanitize connect() results */

int acl_sane_connect(ACL_SOCKET sock, const struct sockaddr *sa, socklen_t len)
{
    int   on;

    /*
    * XXX Solaris select() produces false read events, so that read()
    * blocks forever on a blocking socket, and fails with EAGAIN on
    * a non-blocking socket. Turning on keepalives will fix a blocking
    * socket provided that the kernel's keepalive timer expires before
    * the Postfix watchdog timer.
    */

#ifdef AF_INET6
    if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6) {
#else
    if (sa->sa_family == AF_INET) {
#endif
        /* default set to nodelay --- zsx, 2008.9.4*/
        acl_tcp_nodelay(sock, 1);
#if defined(BROKEN_READ_SELECT_ON_TCP_SOCKET) && defined(SO_KEEPALIVE)
        on = 1;
        (void) setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
                (char *) &on, sizeof(on));
#endif
    }

    on = 1;

#ifdef SO_REUSEADDR
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
        (char *) &on, sizeof(on)) < 0) {

        acl_msg_error("acl_sane_connect: setsockopt error(%s)",
            acl_last_serror());
    }
#endif
    return __sys_connect(sock, sa, len);
}
