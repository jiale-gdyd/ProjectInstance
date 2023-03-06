#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

EMSDemoImpl::EMSDemoImpl() : mThreadFin(false)
{
    mPipelines.clear();
}

EMSDemoImpl::~EMSDemoImpl()
{
    getApi()->stop();
    mThreadFin = true;
    mediaDeinit();
}

int EMSDemoImpl::init()
{
    media::axsys_init_params_t sysParams;
#if 0
    sysParams.enableCamera = true;
    sysParams.sysCameraCase = media::SYS_CASE_SINGLE_GC4653;
    sysParams.sysCameraHdrMode = media::SNS_MODE_NONE;
    sysParams.sysCameraSnsType = media::GALAXYCORE_GC4653;
    sysParams.cameraIvpsFrameRate = 25;
#else
    sysParams.enableCamera = false;
    media::axsys_pool_cfg_t poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 10},
    };
    sysParams.sysCommonArgs.poolCfgCount = 1;
    sysParams.sysCommonArgs.poolCfg = poolcfg;
#endif

    if (getApi()->init(&sysParams) != 0) {
        return -1;
    }

    /* NPU初始化 */

    if (mediaInit()) {
        return -2;
    }

    getApi()->run();
    return 0;
}

API_END_NAMESPACE(EMS)
