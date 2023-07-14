#ifndef TBOX_NETWORK_DNS_REQUEST_H
#define TBOX_NETWORK_DNS_REQUEST_H

#include <map>
#include "../event/loop.h"
#include "../eventx/timeout_monitor.hpp"

#include "udp_socket.h"
#include "domain_name.h"

namespace tbox {
namespace network {

/// DNS请求
///
/// 用于发送DNS请求，支持并行发送多个DNS清求
class DnsRequest {
  public:
    struct A {
        uint32_t ttl;
        IPAddress ip;
    };

    struct CNAME {
        uint32_t ttl;
        DomainName cname;
    };

    //! 查询结果
    struct Result {
        enum class Status {
            kSuccess,           //!< 成功
            /// 以下是错误状态
            kDomainError,       //!< 域名错误
            kAllDnsFail,        //!< 所有服务器都失败
            kTimeout,           //!< 所有的DNS回复超时
            kFail,              //!< 其它错误
        };

        Status status = Status::kSuccess;   //!< 结果状态
        std::vector<A>      a_vec;      //!< A记录列表
        std::vector<CNAME>  cname_vec;  //!< CNAME记录列表
    };

    using IPAddressVec  = std::vector<IPAddress>;

    /// 请求结束回调
    using Callback = std::function<void(const Result &result)>;

    using ReqId = uint16_t;

  public:
    explicit DnsRequest(event::Loop *wp_loop);
    explicit DnsRequest(event::Loop *wp_loop, const IPAddressVec &dns_ip_vec);
    virtual ~DnsRequest();

  public:
    /// 设置DNS IP地址
    void setDnsIPAddresses(const IPAddressVec &dns_ip_vec);

    /// 向DNS服务器发送查询请求
    ReqId request(const DomainName &domain, const Callback &cb);

    /// 取消当前的查询请求
    bool cancel(ReqId req_id);

    /// 检查当前是否处于查询中
    bool isRunning(ReqId req_id) const;

  protected:
    struct Request {
        Callback cb;
        size_t response_count = 0;
    };

    void init();
    void onUdpRecv(const void *data_ptr, size_t data_size, const SockAddr &from);
    void onRequestTimeout(ReqId req_id);

    void addRequest(ReqId req_id, const Callback &cb);
    Request* findRequest(ReqId req_id);
    bool deleteRequest(ReqId req_id);

  private:
    UdpSocket udp_;
    eventx::TimeoutMonitor<ReqId> timeout_monitor_;

    IPAddressVec dns_ip_vec_;
    ReqId req_id_alloc_ = 0;

    std::map<ReqId, Request> requests_;
};

}
}

#endif //TBOX_NETWORK_DNS_REQUEST_H_20230207
