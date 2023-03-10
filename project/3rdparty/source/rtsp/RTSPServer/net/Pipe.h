#ifndef XOP_PIPE_H
#define XOP_PIPE_H

#include "TcpSocket.h"

namespace xop {
class Pipe {
public:
    Pipe();
    virtual ~Pipe();

    void Close();
    bool Create();

    int Read(void *buf, int len);
    int Write(void *buf, int len);

    SOCKET Read() const {
        return pipe_fd_[0];
    }

    SOCKET Write() const {
        return pipe_fd_[1];
    }

private:
    SOCKET pipe_fd_[2];
};
}

#endif
