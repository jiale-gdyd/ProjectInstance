#ifndef XOP_TCP_SOCKET_H
#define XOP_TCP_SOCKET_H

#include <string>
#include <cstdint>

#include "Socket.h"

namespace xop {
class TcpSocket {
public:
    TcpSocket(SOCKET sockfd = -1);
    virtual ~TcpSocket();

    void Close();
    SOCKET Create();

    bool Bind(std::string ip, uint16_t port);
    bool Listen(int backlog);

    SOCKET Accept();
    bool Connect(std::string ip, uint16_t port, int timeout = 0);

    void ShutdownWrite();
    SOCKET GetSocket() const {
        return sockfd_;
    }

private:
    SOCKET sockfd_ = -1;
};
}

#endif
