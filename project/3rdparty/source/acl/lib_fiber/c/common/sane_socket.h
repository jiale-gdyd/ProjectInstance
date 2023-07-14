#ifndef SANE_SOCKET_INCLUDE_H
#define SANE_SOCKET_INCLUDE_H

#include "acl/lib_fiber/c/libfiber.h"

int is_listen_socket(socket_t fd);

int getsockfamily(socket_t fd);

// return: -1, SOCK_STREAM, SOCK_DGRAM
int getsocktype(socket_t fd);

// set the TCP socket SO_LINGER option
void tcp_so_linger(socket_t fd, int onoff, int timeout);

#endif
