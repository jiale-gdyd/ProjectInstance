#ifndef XOP_SOCKET_H
#define XOP_SOCKET_H

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/route.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/select.h>

#include <cstdint>
#include <cstring>

#define SOCKET              int
#define INVALID_SOCKET      (-1)
#define SOCKET_ERROR        (-1)

#endif
