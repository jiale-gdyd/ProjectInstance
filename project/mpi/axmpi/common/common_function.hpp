#ifndef MPI_AXMPI_COMMON_FUNCTION_HPP
#define MPI_AXMPI_COMMON_FUNCTION_HPP

#include <utils/export.h>

#include "common_vin.hpp"
#include "common_camera.hpp"
#include "common_system.hpp"

API_BEGIN_NAMESPACE(media)

enum {
    SYS_CASE_NONE                  = -1,
    SYS_CASE_SINGLE_OS04A10        = 0,
    SYS_CASE_SINGLE_IMX334         = 1,
    SYS_CASE_SINGLE_GC4653         = 2,
    SYS_CASE_DUAL_OS04A10          = 3,
    SYS_CASE_SINGLE_OS08A20        = 4,
    SYS_CASE_SINGLE_OS04A10_ONLINE = 5,
    SYS_CASE_SINGLE_DVP            = 6,
    SYS_CASE_SINGLE_BT601          = 7,
    SYS_CASE_SINGLE_BT656          = 8,
    SYS_CASE_SINGLE_BT1120         = 9,
    SYS_CASE_MIPI_YUV              = 10,
    SYS_CASE_BUTT
};

int axcam_setup(axcam_t cams[MAX_CAMERAS], int sysCase, int hadMode, int *snsType, axsys_args_t *pArgs, int frameRate);

extern axsys_pool_cfg_t gtSysCommPoolSingleOs04a10Sdr[5];
extern axsys_pool_cfg_t gtSysCommPoolSingleOs04a10OnlineSdr[5];
extern axsys_pool_cfg_t gtSysCommPoolSingleOs04a10Hdr[5];
extern axsys_pool_cfg_t gtSysCommPoolSingleOs04a10OnlineHdr[5];

extern axsys_pool_cfg_t gtSysCommPoolSingleImx334Sdr[5];
extern axsys_pool_cfg_t gtSysCommPoolSingleImx334Hdr[5];

extern axsys_pool_cfg_t gtSysCommPoolSingleGc4653[5];
extern axsys_pool_cfg_t gtSysCommPoolDoubleOs04a10[5];
extern axsys_pool_cfg_t gtSysCommPoolSingleOs08a20Sdr[5];
extern axsys_pool_cfg_t gtSysCommPoolSingleOs08a20Hdr[5];

extern axsys_pool_cfg_t gtSysCommPoolSingleDVP[3];

extern axsys_pool_cfg_t gtSysCommPoolBT601[5];
extern axsys_pool_cfg_t gtSysCommPoolBT656[5];
extern axsys_pool_cfg_t gtSysCommPoolBT1120[5];
extern axsys_pool_cfg_t gtSysCommPoolMIPI_YUV[1];

API_END_NAMESPACE(media)

#endif
