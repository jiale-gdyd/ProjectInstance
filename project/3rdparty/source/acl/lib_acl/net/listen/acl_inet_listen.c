#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef ACL_UNIX
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "acl/lib_acl/stdlib/acl_sys_patch.h"
#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_mymalloc.h"
#include "acl/lib_acl/stdlib/acl_iostuff.h"
#include "acl/lib_acl/net/acl_host_port.h"
#include "acl/lib_acl/net/acl_sane_inet.h"
#include "acl/lib_acl/net/acl_sane_socket.h"
#include "acl/lib_acl/net/acl_listen.h"

#endif

/* acl_inet_listen - create TCP listener */
ACL_SOCKET acl_inet_listen(const char *addr, int backlog, unsigned flag)
{
    ACL_SOCKET sock = acl_sane_bind(addr, flag, SOCK_STREAM, NULL);

    if (sock == ACL_SOCKET_INVALID) {
        acl_msg_error("%s(%d), %s: bind %s error %s", __FILE__,
            __LINE__, __FUNCTION__, addr, acl_last_serror());
        return ACL_SOCKET_INVALID;
    }

#if defined(TCP_FASTOPEN)
    if (flag & ACL_INET_FLAG_FASTOPEN) {
        int on = 1;
        int ret = setsockopt(sock, IPPROTO_TCP, TCP_FASTOPEN,
            (const void *) &on, sizeof(on));
        if (ret < 0) {
            acl_msg_warn("%s(%d): setsocket(TCP_FASTOPEN): %s",
                __FUNCTION__, __LINE__, acl_last_serror());
        }
    }
#endif

    acl_non_blocking(sock, flag & ACL_INET_FLAG_NBLOCK ?
        ACL_NON_BLOCKING : ACL_BLOCKING);

    if (listen(sock, backlog) < 0) {
        acl_socket_close(sock);
        acl_msg_error("%s(%d), %s: listen %s error %s", __FILE__,
            __LINE__, __FUNCTION__, addr, acl_last_serror());
        return ACL_SOCKET_INVALID;
    }

    acl_msg_info("%s: listen %s ok", __FUNCTION__, addr);
    return sock;
}

ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd)
{
    return acl_inet_accept_ex(listen_fd, NULL, 0);
}

ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf, size_t size)
{
    ACL_SOCKADDR sa;
    socklen_t len = sizeof(sa);
    ACL_SOCKET fd;

    memset(&sa, 0, sizeof(sa));

    /* when client_addr not null and protocol is AF_INET, acl_sane_accept
    * will set nodelay on the accepted socket, 2008.9.4, zsx
    */
    fd = acl_sane_accept(listen_fd, (struct sockaddr*) &sa, &len);
    if (fd == ACL_SOCKET_INVALID)
        return fd;

    if (ipbuf != NULL && size > 0 && !acl_inet_ntop(&sa.sa, ipbuf, size))
        ipbuf[0] = 0;

    return fd;
}
