#ifndef TBOX_HTTP_SERVER_SERVER_H
#define TBOX_HTTP_SERVER_SERVER_H

#include "../../event/loop.h"
#include "../../network/sockaddr.h"

#include "../common.h"
#include "../request.h"
#include "../respond.h"
#include "context.h"

#include "types.h"

namespace tbox {
namespace http {
namespace server {

class Middleware;

class Server {
    friend Context;

  public:
    explicit Server(event::Loop *wp_loop);
    virtual ~Server();

  public:
    bool initialize(const network::SockAddr &bind_addr, int listen_backlog);
    bool start();
    void stop();
    void cleanup();

    enum class State { kNone, kInited, kRunning };
    State state() const;
    void setContextLogEnable(bool enable);

  public:
    void use(const RequestCallback &cb);
    void use(Middleware *wp_middleware);

  private:
    class Impl;
    Impl *impl_;
};

}
}
}

#endif //TBOX_HTTP_SERVER_H_20220501
