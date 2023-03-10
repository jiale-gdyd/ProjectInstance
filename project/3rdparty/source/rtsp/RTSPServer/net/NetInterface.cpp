#include "Socket.h"
#include "NetInterface.h"

namespace xop {
std::string NetInterface::GetLocalIPAddress()
{
    SOCKET sockfd = 0;
    char buf[512] = {0};
    struct ifreq *ifreq;
    struct ifconf ifconf;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        close(sockfd);
        return "0.0.0.0";
    }

    ifconf.ifc_len = 512;
    ifconf.ifc_buf = buf;
    if (ioctl(sockfd, SIOCGIFCONF, &ifconf) < 0) {
        close(sockfd);
        return "0.0.0.0";
    }

    close(sockfd);

    ifreq = (struct ifreq *)ifconf.ifc_buf;
    for (int i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; i--) {
        if (ifreq->ifr_flags == AF_INET) {
            if (strcmp(ifreq->ifr_name, "lo") != 0) {
                return inet_ntoa(((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr);
            }

            ifreq++;
        }
    }

    return "0.0.0.0";
}
}
