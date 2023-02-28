#ifndef PLATFORM_AARCH32_ROCKCHIP_PRIVATE_HPP
#define PLATFORM_AARCH32_ROCKCHIP_PRIVATE_HPP

#include <aarch32/rockchip/mpi/mediaBase.hpp>

API_BEGIN_NAMESPACE(media)

class RV1126 : public MediaInterface {
public:
    RV1126();
    ~RV1126();

    int init();
    void deinit();
};

API_END_NAMESPACE(media)

#endif
