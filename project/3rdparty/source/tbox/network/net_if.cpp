#include <tbox/network/net_if.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <errno.h>
#include <cstring>
#include <sstream>

#include <tbox/base/log.h>

namespace tbox {
namespace network {
namespace {

bool LoadFrom(NetIF &net_if, const struct ifaddrs *ifa)
{
    if (ifa->ifa_addr->sa_family != AF_INET) {
        LogWarn("unsupport IPv6");
        return false;
    }

    net_if.name  = ifa->ifa_name;
    net_if.flags = ifa->ifa_flags;
    struct sockaddr_in *addr_v4 = (struct sockaddr_in*)ifa->ifa_addr;
    net_if.ip = addr_v4->sin_addr.s_addr;
    struct sockaddr_in *mask_v4 = (struct sockaddr_in*)ifa->ifa_netmask;
    net_if.mask = mask_v4->sin_addr.s_addr;
    return true;
}

}

bool GetNetIF(std::vector<NetIF> &net_if_vec)
{
    struct ifaddrs *if_list = nullptr;
    if (getifaddrs(&if_list) == 0) {
        for (auto ifa = if_list; ifa != nullptr; ifa = ifa->ifa_next) {
            NetIF net_if;
            if (LoadFrom(net_if, ifa))
                net_if_vec.push_back(std::move(net_if));
        }
        freeifaddrs(if_list);
        return true;

    } else {
        LogWarn("fail: errno:%d, %s", errno, strerror(errno));
        return false;
    }
}

bool GetNetIF(const std::string &name, std::vector<NetIF> &net_if_vec)
{
    struct ifaddrs *if_list = nullptr;
    if (getifaddrs(&if_list) == 0) {
        for (auto ifa = if_list; ifa != nullptr; ifa = ifa->ifa_next) {
            if (name != ifa->ifa_name)
                continue;

            NetIF net_if;
            if (LoadFrom(net_if, ifa))
                net_if_vec.push_back(std::move(net_if));
        }
        freeifaddrs(if_list);
        return true;

    } else {
        LogWarn("fail: errno:%d, %s", errno, strerror(errno));
        return false;
    }
}

}
}
