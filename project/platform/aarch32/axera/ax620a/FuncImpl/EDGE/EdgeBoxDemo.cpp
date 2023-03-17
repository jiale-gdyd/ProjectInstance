#include "EdgeBoxDemo.hpp"

namespace edge {
EdgeBoxDemo::EdgeBoxDemo() : mInitFin(false), mAlgoInit(false), mQuitThread(false), 
    mAlgoWidth(960), mAlgoHeight(540), mHandler(nullptr), mCamFPS(25), mIvpsFPS(60)
{

}

EdgeBoxDemo::~EdgeBoxDemo()
{
    mInitFin = false;

    rtspClientRelease();
    pipelineRelease();
    axpi::axpi_exit(&mHandler);

    getApi()->stop();
}

int EdgeBoxDemo::init()
{
    axpi::axsys_init_params_t params;
    axpi::axsys_pool_cfg_t poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 10},
    };

    params.enableCamera = false;
    params.sysCommonArgs.poolCfgCount = 1;
    params.sysCommonArgs.poolCfg = poolcfg;
    params.enableNPU = true;
    params.npuInitInfo.npuHdrMode = AX_NPU_VIRTUAL_1_1;

    if (getApi()->init(&params) != 0) {
        return -1;
    }

    if (axpi::axpi_init("/root/yolov5s.json", &mHandler) != 0) {
        return -2;
    } else {
        mAlgoInit = true;
        axpi::axpi_get_ivps_width_height(mHandler, "/root/yolov5s.json", mAlgoWidth, mAlgoHeight);
    }

    mRtspURL.push_back("rtsp://192.168.1.7");
    mRtspURL.push_back("rtsp://192.168.1.8");
    if (mRtspURL.size() > MAX_RTSPURLS) {
        mRtspURL.resize(MAX_RTSPURLS);
    }

    mPipelines.resize(mRtspURL.size());

    if (pipelineCreate()) {
        return -3;
    }

    if (rtspClientCreate()) {
        return -4;
    }

    mInitFin = true;
    return getApi()->run();
}
}
