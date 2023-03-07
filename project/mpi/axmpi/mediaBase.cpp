#include <unistd.h>
#include <string.h>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaApi::MediaApi()
{
    mIspLoopOut = false;
    memset(mCamers, 0x00, sizeof(mCamers));
}

MediaApi::~MediaApi()
{
    mIspLoopOut = true;
    for (int i = 0; i < mSysInitParam.sysCommonArgs.camCount; ++i) {
        if (!mCamers[i].bOpen) {
            continue;
        }

        axcam_close(&mCamers[i]);
        if (mIspThreadId[i].joinable()) {
            mIspThreadId[i].join();
        }
    }

    if (mSysInitParam.enableCamera) {
        axcam_deinit();
    }

    axsys_deinit();
}

int MediaApi::init(axsys_init_params_t *param)
{
    if (!param) {
        return -1;
    }

    int ret = -1;
    memcpy(&mSysInitParam, param, sizeof(axsys_init_params_t));

    if (mSysInitParam.enableCamera) {
        ret = axcam_setup(mCamers, mSysInitParam.cameraInfo.sysCameraCase,
            mSysInitParam.cameraInfo.sysCameraHdrMode, (int *)&mSysInitParam.cameraInfo.sysCameraSnsType,
            &mSysInitParam.sysCommonArgs, mSysInitParam.cameraInfo.cameraIvpsFrameRate);
        if (ret != 0) {
            return -2;
        }
    }

    ret = axsys_init(&(mSysInitParam.sysCommonArgs));
    if (ret != 0) {
        return -3;
    }

    if (mSysInitParam.enableNPU) {
        AX_NPU_SDK_EX_ATTR_T sNpuAttr;
        sNpuAttr.eHardMode = (AX_NPU_SDK_EX_HARD_MODE_T)mSysInitParam.npuInitInfo.npuHdrMode;
        ret = AX_NPU_SDK_EX_Init_with_attr(&sNpuAttr);
        if (ret != 0) {
            axcam_deinit();
            axsys_deinit();
            return -4;
        }
    }

    return 0;
}

int MediaApi::run()
{
    if (mSysInitParam.enableCamera) {
        int ret = axcam_init();
        if (ret != 0) {
            axsys_deinit();
            return -1;
        }

        for (int i = 0; i < mSysInitParam.sysCommonArgs.camCount; ++i) {
            ret = axcam_open(&mCamers[i]);
            if (ret != 0) {
                axcam_deinit();
                axsys_deinit();
                return -2;
            }

            mCamers[i].bOpen = true;
            mIspThreadId[i] = std::thread(std::bind(&MediaApi::ispRunThread, this, i));
        }
    }

    while (!mIspLoopOut) {
        sleep(3);
    }

    return 0;
}

void MediaApi::stop()
{
    mIspLoopOut = true;
}

void MediaApi::ispRunThread(int camId)
{
    while (!mIspLoopOut) {
        if (!mCamers[camId].bOpen) {
            usleep(40000);
            continue;
        }

        AX_ISP_Run(mCamers[camId].nPipeId);
    }
}

media::MediaApi *MediaBase::mMediaApi = nullptr;

MediaBase::MediaBase()
{

}

MediaBase::~MediaBase()
{
    if (mMediaApi) {
        delete mMediaApi;
        mMediaApi = nullptr;
    }
}

media::MediaApi *MediaBase::getApi()
{
    if (mMediaApi == nullptr) {
        mMediaApi = new media::MediaApi();
    }

    return mMediaApi;
}

API_END_NAMESPACE(media)
