#ifndef TBOX_HTTP_CLIENT_CLIENT_H
#define TBOX_HTTP_CLIENT_CLIENT_H

#include "../../event/loop.h"
#include "../../network/sockaddr.h"

#include "../request.h"
#include "../respond.h"

namespace tbox {
namespace http {
namespace client {

class Client {
  public:
    explicit Client(event::Loop *wp_loop);
    virtual ~Client();

  public:
    //! 初始化，设置目标服务器
    bool initialize(const network::SockAddr &server_addr);

    //! 收到回复时的回调
    using RespondCallback = std::function<void(const Respond &res)>;

    /**
     * \brief   发送请求
     * \param   req     请求数据
     * \param   cb      回复的回调
     */
    void request(const Request &req, const RespondCallback &cb);

    //! 清理，与initialize()是逆操作
    void cleanup();

  private:
    class Impl;
    Impl *impl_;
};

}
}
}

#endif //TBOX_HTTP_CLIENT_H_20220504
