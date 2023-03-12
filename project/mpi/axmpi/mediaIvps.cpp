#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaIvps.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

namespace axpi {
MediaIvps::MediaIvps()
{

}

MediaIvps::~MediaIvps()
{

}

int MediaIvps::updateRegion(int handler, const axivps_rgn_disp_grp_t *pDispInfo)
{
    int ret = AX_IVPS_RGN_Update((IVPS_RGN_HANDLE)handler, (const AX_IVPS_RGN_DISP_GROUP_S *)pDispInfo);
    if (ret != 0) {
        return -1;
    }

    return 0;
}
}
