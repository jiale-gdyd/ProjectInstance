#include <unistd.h>
#include <string.h>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

namespace axpi {
int g_majorStreamWidth = 1920;
int g_majorStreamHeight = 1080;

MediaApi::MediaApi()
{
    mIspLoopOut = false;
    memset(mCamers, 0x00, sizeof(mCamers));
}

MediaApi::~MediaApi()
{
    mIspLoopOut = true;
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

        memset(&(mSysInitParam.sysCommonArgs), 0, sizeof(mSysInitParam.sysCommonArgs));
        g_majorStreamWidth = mCamers[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
        g_majorStreamHeight = mCamers[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;

        printf("major stream width:[%d], height:[%d]\n", g_majorStreamWidth, g_majorStreamHeight);
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

int MediaApi::camInit()
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
        }
    }

    return 0;
}

int MediaApi::run()
{
    if (mSysInitParam.enableCamera) {
        for (int i = 0; i < mSysInitParam.sysCommonArgs.camCount; ++i) {

            if (mCamers[i].bOpen) {
                mIspThreadId[i] = std::thread(std::bind(&MediaApi::ispRunThread, this, i));
            }
        }
    }

    mSemaphore.wait();

    if (mSysInitParam.enableCamera) {
        for (int i = 0; i < mSysInitParam.sysCommonArgs.camCount; ++i) {
            if (!mCamers[i].bOpen) {
                continue;
            }

            if (mIspThreadId[i].joinable()) {
                mIspThreadId[i].join();
            }

            axcam_close(&mCamers[i]);
        }
    }

    return 0;
}

void MediaApi::stop()
{
    mIspLoopOut = true;
    mSemaphore.post(2);
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

axpi::MediaApi *MediaBase::mMediaApi = nullptr;

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

axpi::MediaApi *MediaBase::getApi()
{
    if (mMediaApi == nullptr) {
        mMediaApi = new axpi::MediaApi();
    }

    return mMediaApi;
}
}
