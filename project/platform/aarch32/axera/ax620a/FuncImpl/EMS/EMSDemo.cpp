#include "EMSDemo.hpp"

namespace EMS {
EMSDemo::EMSDemo() : mInitFin(false), mAlgoInit(false), mQuitThread(false), mStartID(0x90015),
    mAlgoWidth(960), mAlgoHeight(540), mHandler(nullptr), mCamFPS(25), mIvpsFPS(60),
    mVoRotateAngle(90), mScreenWidth(480), mScreenHeight(854)
{

}

EMSDemo::~EMSDemo()
{
    mInitFin = false;

    pipelineRelease();
    axpi::axpi_exit(&mHandler);

    getApi()->stop();
}

int EMSDemo::init()
{
    axpi::axsys_init_params_t params;
    params.enableCamera = true;
    params.cameraInfo.sysCameraCase = axpi::SYS_CASE_SINGLE_GC4653;
    params.cameraInfo.cameraIvpsFrameRate = mCamFPS;
    params.cameraInfo.sysCameraHdrMode = AX_SNS_LINEAR_MODE;
    params.cameraInfo.sysCameraSnsType = axpi::GALAXYCORE_GC4653;
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

    if (getApi()->camInit()) {
        return -3;
    }

    if (pipelineCreate()) {
        return -4;
    }

    mInitFin = true;
    return getApi()->run();
}
}
