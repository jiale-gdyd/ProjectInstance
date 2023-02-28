#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::mediaInit()
{
    int ret = -1;
    bool bEnableVenc = false;
    bool bEnableVdecNotVI = false;
    size_t srcWidth = mOriginWidth, srcHeight = mOriginHeight;

    if (mUseVdecNotVi && (mVdecParameter.size() > 0)) {
        ret = getMedia()->getVdec().createVdecChn(mVdecParameter[0].vdecChn, mVdecParameter[0].codecFile, mVdecParameter[0].codecType,
            mVdecParameter[0].bDecLoop, mVdecParameter[0].oneFramSize, mVdecParameter[0].intervalMs, mVdecParameter[0].hardwareAlloc, mVdecParameter[0].allocFlag);
        if (ret) {
            bEnableVdecNotVI = false;
            printf("create vdec chn:[%d] failed\n", mVdecParameter[0].vdecChn);
        } else {
            bEnableVdecNotVI = true;
        }
    }

    if (!bEnableVdecNotVI) {
        mUseVdecNotVi = false;
        ret = getMedia()->getVi().createViChn(mVideoFirstInChn, 6, 3, mVideoInputDevNode.c_str(), mVideoInputWidth, mVideoInputHeight, mInputImagePixType);
        if (ret) {
            printf("create vin chn:[%d] failed\n", mVideoFirstInChn);
            return -1;
        }
    }

    if (mUseVdecNotVi) {
        srcWidth = mVdecParameter[0].videoWidth;
        srcHeight = mVdecParameter[0].videoHeight;
    }

    ret = getMedia()->getRga().createRgaChn(mInferRgaChn, 3, 3, mInputImagePixTypeEx, mInferImagePixType, 0, mImageFlipMode, false,
        srcWidth, srcHeight, 0, 0, mOriginWidth, mOriginHeight, 0, 0);
    if (ret) {
        printf("create rga chn:[%d] failed, return:[%d]\n", mInferRgaChn, ret);
        return -2;
    }

    ret = getMedia()->getRga().createRgaChn(mDispPrevRgaChn, 6, 6, mInputImagePixTypeEx, mOutputImagePixType, 0, mImageFlipMode, false,
        srcWidth, srcHeight, 0, 0, mOriginWidth, mOriginHeight, 0, 0);
    if (ret) {
        printf("create rga chn:[%d] failed\n", mDispPrevRgaChn);
        return -3;
    }

    ret = getMedia()->getRga().createRgaChn(mCropRgaChn, 3, 3, mOutputImagePixType, mOutputImagePixType, 0, RGA_FLIP_NULL, false,
        mOriginWidth, mOriginHeight, 0, 0, mOriginWidth, mOriginHeight);
    if (ret) {
        printf("create rga chn:[%d] failed\n", mCropRgaChn);
        return -4;
    }

    ret = getMedia()->getRga().createRgaChn(mZoomRgaChn, 3, 3, mOutputImagePixType, mOutputImagePixType, 0, RGA_FLIP_NULL, mPrimaryDispSwap,
        mOriginWidth, mOriginHeight, 0, 0, mPrimaryDispSwap ? mPrimaryDispHeight : mPrimaryDispWidth, mPrimaryDispSwap ? mPrimaryDispWidth : mPrimaryDispHeight);
    if (ret) {
        printf("create rga chn:[%d] failed\n", mZoomRgaChn);
        return -5;
    }

    // 使能视频编码存储且非视频解码源
    if (!mEMSConfig.disableVideoEncoderSave && !mUseVdecNotVi) {
        ret = getMedia()->getVenc().createVencChnEx(mVideoVencChn, 6, mOriginWidth, mOriginHeight, mInputImagePixType, mVideoVencType, mVideoVencRcMode,
            mEMSConfig.videoEncoderParam.videoFPS, mEMSConfig.videoEncoderParam.encodeProfile, NULL, this,
            mEMSConfig.videoEncoderParam.encodeBitRate, mEMSConfig.videoEncoderParam.encodeIFrameInterval, 0);
        if (ret == 0) {
            ret = getMedia()->getRga().createRgaChn(mVideoVencRgaChn, 6, 3, mOutputImagePixType, mInputImagePixType, 0, RGA_FLIP_NULL, false, 
                mOriginWidth, mOriginHeight, 0, 0, mOriginWidth, mOriginHeight, 0, 0);
            if (ret) {
                mVideoVencEnOK = false;
                printf("create rga chn:[%d] failed\n", mVideoVencRgaChn);
            } else {
                bEnableVenc = true;
            }
        } else {
            mVideoVencEnOK = false;
            printf("create venc chn:[%d] failed\n", mVideoVencChn);
        }
    }

    ret = getMedia()->getVo().createVoChn(mPrimaryVoChn, mPrimaryZpos, mPrimaryDispLayers, mPrimaryDispWidth, mPrimaryDispHeight, mPrimaryXoffset, mPrimaryYoffset,
        mPrimaryDispSwap, mOutputImagePixType, mDisplayDeviceCard.c_str());
    if (ret) {
        printf("create vo chn:[%d] failed", mPrimaryVoChn);
        return -6;
    }

    ret = getMedia()->getVo().createVoChn(mOverlayVoChn, mOverlayZpos, mOverlayDispLayers, mOverlayDispWidth, mOverlayDispHeight, mOverlayXoffset, mOverlayYoffset,
        mOverlayDispSwap, mOutputImagePixType, mDisplayDeviceCard.c_str());
    if (ret) {
        printf("create vo chn:[%d] failed", mOverlayVoChn);
        return -7;
    }

    if (mUseVdecNotVi) {
        ret = getMedia()->getSys().bindVdecRga(mVdecParameter[0].vdecChn, mInferRgaChn);
        if (ret) {
            printf("bind vdec chn:[%d] to rga chn:[%d] failed", mVdecParameter[0].vdecChn, mInferRgaChn);
            return -8;
        }

        ret = getMedia()->getSys().bindVdecRga(mVdecParameter[0].vdecChn, mDispPrevRgaChn);
        if (ret) {
            printf("bind vi chn:[%d] to rga chn:[%d] failed", mVideoFirstInChn, mDispPrevRgaChn);
            return -9;
        }
    } else {
        ret = getMedia()->getSys().bindViRga(mVideoFirstInChn, mInferRgaChn);
        if (ret) {
            printf("bind vi chn:[%d] to rga chn:[%d] failed", mVideoFirstInChn, mInferRgaChn);
            return -10;
        }

        ret = getMedia()->getSys().bindViRga(mVideoFirstInChn, mDispPrevRgaChn);
        if (ret) {
            printf("bind vi chn:[%d] to rga chn:[%d] failed", mVideoFirstInChn, mDispPrevRgaChn);
            return -11;
        }

        // 视频编码
        if (bEnableVenc) {
            ret = getMedia()->getSys().bindRgaVenc(mVideoVencRgaChn, mVideoVencChn);
            if (ret) {
                mVideoVencEnOK = false;
                printf("bind rga chn:[%d] to venc chn:[%d] failed", mVideoVencRgaChn, mVideoVencChn);
            } else {
                mVideoVencEnOK = true;
            }
        }
    }

    return 0;
}

void EMSDemoImpl::mediaDeinit()
{
    if (mUseVdecNotVi) {
        getMedia()->getSys().unbindVdecRga(mVdecParameter[0].vdecChn, mInferRgaChn);
        getMedia()->getSys().unbindVdecRga(mVdecParameter[0].vdecChn, mDispPrevRgaChn);
    } else {
        getMedia()->getSys().unbindViRga(mVideoFirstInChn, mInferRgaChn);
        getMedia()->getSys().unbindViRga(mVideoFirstInChn, mDispPrevRgaChn);

        if (!mEMSConfig.disableVideoEncoderSave) {
            getMedia()->getSys().unbindRgaVenc(mVideoVencRgaChn, mVideoVencChn);
        }
    }

    getMedia()->getVo().destroyVoChn(mPrimaryVoChn);
    getMedia()->getVo().destroyVoChn(mOverlayVoChn);

    getMedia()->getRga().destroyRgaChn(mInferRgaChn);
    getMedia()->getRga().destroyRgaChn(mDispPrevRgaChn);

    getMedia()->getRga().destroyRgaChn(mCropRgaChn);
    getMedia()->getRga().destroyRgaChn(mZoomRgaChn);

    if (!mEMSConfig.disableVideoEncoderSave) {
        mVideoVencEnOK = false;
        getMedia()->getVenc().destroyVencChn(mVideoVencChn);
    }

    if (mUseVdecNotVi) {
        getMedia()->getVdec().destroyVdecChn(mVdecParameter[0].vdecChn);
    } else {
        getMedia()->getVi().destroyViChn(mVideoFirstInChn);
    }
}

API_END_NAMESPACE(EMS)
