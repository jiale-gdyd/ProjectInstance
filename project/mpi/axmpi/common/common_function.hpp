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

API_END_NAMESPACE(media)

#endif
