#pragma once

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <thread>
#include <functional>
#include <ax_ivps_api.h>

namespace axpi {
typedef AX_IVPS_RGN_DISP_GROUP_S axivps_rgn_disp_grp_t;

class MediaIvps {
public:
    MediaIvps();
    ~MediaIvps();

public:
    int updateRegion(int handler, const axivps_rgn_disp_grp_t *pDispInfo);
};
}
