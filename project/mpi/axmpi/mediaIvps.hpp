#ifndef AXERA_MEDIAIVPS_HPP
#define AXERA_MEDIAIVPS_HPP

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <thread>
#include <functional>
#include <ax_ivps_api.h>
#include <utils/export.h>

#include "axapi.h"

API_BEGIN_NAMESPACE(media)

typedef AX_IVPS_RGN_DISP_GROUP_S axivps_rgn_disp_grp_t;

class API_HIDDEN MediaIvps {
public:
    MediaIvps();
    ~MediaIvps();

public:
    int updateRegion(int handler, const axivps_rgn_disp_grp_t *pDispInfo);
};

API_END_NAMESPACE(media)

#endif