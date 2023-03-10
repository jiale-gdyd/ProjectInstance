#include <rtsp/internal/util.h>
#include <rtsp/internal/NetCommon.h>
#include <rtsp/internal/RTSPCommonEnv.h>

#include "../../private.h"

namespace rtsp {
char *strDup(char const*str)
{
    if (str == NULL) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = new char[len];

    if (copy != NULL) {
        memcpy(copy, str, len);
    }

    return copy;
}

char *strDupSize(char const *str)
{
    if (str == NULL) {
        return NULL;
    }

    size_t len = strlen(str) + 1;
    char *copy = new char[len];

    return copy;
}

int CheckUdpPort(unsigned short port)
{
    int newSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocket < 0) {
        rtsp_error("unable to create datagram socket");
        return -1;
    }

    struct sockaddr_in c_addr;
    memset(&c_addr, 0, sizeof(c_addr));
    c_addr.sin_addr.s_addr = INADDR_ANY;
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(port);

    if (bind(newSocket, (struct sockaddr *)&c_addr, sizeof(c_addr)) != 0) {
        rtsp_error("bind() error (port number:[%d)", port);
        closeSocket(newSocket);

        return -1;
    }

    closeSocket(newSocket);
    return 0;
}
}
