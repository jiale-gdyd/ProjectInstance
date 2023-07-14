#ifndef TBOX_NETWORK_SOCKADDR_H
#define TBOX_NETWORK_SOCKADDR_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <string>

#include "../base/assert.h"
#include "ip_address.h"

namespace tbox {
namespace network {

class DomainSockPath;

class SockAddr {
  public:
    SockAddr();

    SockAddr(const IPAddress &ip, uint16_t port);
    SockAddr(const DomainSockPath &path);

    SockAddr(const struct sockaddr &addr, socklen_t len);
    SockAddr(const struct sockaddr_in &addr_in);

    SockAddr(const SockAddr &other);
    SockAddr& operator = (const SockAddr &other);

    static SockAddr FromString(const std::string &add_str);

    enum Type {
        kNone,
        kIPv4,
        kLocal, //! Unix Local socket
    };
    Type type() const;
    std::string toString() const;

    bool get(IPAddress &ip, uint16_t &port) const;

    template <typename T>
    socklen_t toSockAddr(T &addr) const {
        TBOX_ASSERT(len_ <= sizeof(T));
        ::memcpy(&addr, &addr_, len_);
        return len_;
    }

    bool operator == (const SockAddr &rhs) const;
    bool operator != (const SockAddr &rhs) const { return !(*this == rhs); }

  private:
    struct sockaddr_storage addr_;
    socklen_t len_ = 0; //!< 表示地址有效长度
};

class DomainSockPath {
  public:
    explicit DomainSockPath(const std::string &path) : path_(path) { }
    const std::string& get() const { return path_; }

  private:
    std::string path_;
};

}
}

#endif //TBOX_NETWORK_SOCKADDR_H_20171105
