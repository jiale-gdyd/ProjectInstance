#include <string.h>
#include <ax_isp_api.h>
#include <ax_buffer_tool.h>
#include <ax_global_type.h>

#include "../axmpi.h"
#include "common_system.hpp"
#include "common_camera.hpp"
#include "../../utilities/log.hpp"

namespace axpi {
static uint32_t axisp_add_to_plan(axisp_pool_floorplant_t *pPoolFloorPlan, uint32_t nCfgCnt, AX_POOL_CONFIG_T *pPoolConfig)
{
    uint32_t i, done = 0;
    AX_POOL_CONFIG_T *pPC;

    for (i = 0; i < nCfgCnt; i++) {
        pPC = &pPoolFloorPlan->CommPool[i];
        if (pPC->BlkSize == pPoolConfig->BlkSize) {
            pPC->BlkCnt += pPoolConfig->BlkCnt;
            done = 1;
        }
    }

    if (!done) {
        pPoolFloorPlan->CommPool[i] = *pPoolConfig;
        nCfgCnt += 1;
    }

    return nCfgCnt;
}

static int axisp_calc_pool(axsys_pool_cfg_t *pPoolCfg, uint32_t nCommPoolCnt, axisp_pool_floorplant_t *pPoolFloorPlan)
{
    int i, nCfgCnt = 0;
    AX_POOL_CONFIG_T tPoolConfig;

    tPoolConfig.MetaSize = 4 * 1024;
    tPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((char *)tPoolConfig.PartitionName, "anonymous");

    for (i = 0; i < nCommPoolCnt; i++) {
        tPoolConfig.BlkSize = AX_VIN_GetImgBufferSize(pPoolCfg->height, pPoolCfg->widthStride, (AX_IMG_FORMAT_E)pPoolCfg->format, AX_TRUE);
        tPoolConfig.BlkCnt = pPoolCfg->blockCount;
        nCfgCnt = axisp_add_to_plan(pPoolFloorPlan, nCfgCnt, &tPoolConfig);
        pPoolCfg += 1;
    }

    return 0;
}

int axsys_init(axsys_args_t *pArgs)
{
    int ret = 0;
    axisp_pool_floorplant_t tPoolFloorPlan = {0};

    ret = AX_SYS_Init();
    if (ret != 0) {
        axmpi_error("AX_SYS_Init failed, return:[%d]", ret);
        return -1;
    }

    AX_POOL_Exit();

    ret = axisp_calc_pool(pArgs->poolCfg, pArgs->poolCfgCount, &tPoolFloorPlan);
    if (ret != 0) {
        axmpi_error("axisp_calc_pool failed, return:[%d]", ret);
        return -2;
    }

    ret = AX_POOL_SetConfig(&tPoolFloorPlan);
    if (ret != 0) {
        axmpi_error("AX_POOL_SetConfig failed, return:[%d]", ret);
        return -3;
    }

    ret = AX_POOL_Init();
    if (ret != 0) {
        axmpi_error("AX_POOL_Init failed, return:[%d]", ret);
        return -4;
    }

    return 0;
}

int axsys_deinit()
{
    AX_POOL_Exit();
    AX_SYS_Deinit();
    return 0;
}
}
