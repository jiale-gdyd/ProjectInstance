#include <unistd.h>
#include <string.h>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaApi::MediaApi()
{
    mIspLoopOut = false;
    memset(mCamers, 0, sizeof(mCamers));
    memset(&mCommonAgrs, 0, sizeof(mCommonAgrs));
}

MediaApi::~MediaApi()
{
    mIspLoopOut = true;
    for (int i = 0; i < mCommonAgrs.camCount; ++i) {
        if (!mCamers[i].bOpen) {
            continue;
        }

        axcam_close(&mCamers[i]);
        if (mIspThreadId[i].joinable()) {
            mIspThreadId[i].join();
        }
    }

    axcam_deinit();
    axsys_deinit();
}

int MediaApi::init(int sysCase, int hdrMode, int snsType, int frameRate)
{
    int ret = axcam_setup(mCamers, sysCase, hdrMode, &snsType, &mCommonAgrs, frameRate);
    if (ret != 0) {
        return -1;
    }

    ret = axsys_init(&mCommonAgrs);
    if (ret != 0) {
        return -2;
    }

    ret = axcam_init();
    if (ret != 0) {
        axsys_deinit();
        return -3;
    }

    for (int i = 0; i < mCommonAgrs.camCount; ++i) {
        ret = axcam_open(&mCamers[i]);
        if (ret != 0) {
            axcam_deinit();
            axsys_deinit();
            return -4;
        }

        mCamers[i].bOpen = true;
        mIspThreadId[i] = std::thread(std::bind(&MediaApi::ispRunThread, this, i));
    }

    return 0;
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
