#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

int EMSDemoImpl::mediaInit()
{
    int ret = -1;
    bool bEnableRtsp = false;
    bool bEnableVenc = false;
    bool bEnableVdecNotVI = false;
    size_t srcWidth = mOriginWidth, srcHeight = mOriginHeight;

    if (mUseVdecNotVi && (mVdecParameter.size() > 0)) {
        ret = getApi()->getVdec().createVdecChn(mVdecParameter[0].vdecChn, mVdecParameter[0].codecFile, mVdecParameter[0].codecType,
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
        ret = getApi()->getVi().createViChn(mVideoFirstInChn, 6, 3, mVideoInputDevNode.c_str(), mVideoInputWidth, mVideoInputHeight, mInputImagePixType);
        if (ret) {
            printf("create vin chn:[%d] failed\n", mVideoFirstInChn);
            return -1;
        }
    }

    if (mUseVdecNotVi) {
        srcWidth = mVdecParameter[0].videoWidth;
        srcHeight = mVdecParameter[0].videoHeight;
    }

    // 推理 vi/vdec --> rga --> infer
    ret = getApi()->getRga().createRgaChn(mInferRgaChn, 3, 3, mInputImagePixTypeEx, mInferImagePixType, 0, mImageFlipMode, false,
        srcWidth, srcHeight, 0, 0, mOriginWidth, mOriginHeight, 0, 0);
    if (ret) {
        printf("create rga chn:[%d] failed, return:[%d]\n", mInferRgaChn, ret);
        return -2;
    }

    // 显示 vi/vdec --> rga --> vo
    ret = getApi()->getRga().createRgaChn(mDispPrevRgaChn, 6, 6, mInputImagePixTypeEx, mOutputImagePixType, 0, mImageFlipMode, false,
        srcWidth, srcHeight, 0, 0, mOriginWidth, mOriginHeight, 0, 0);
    if (ret) {
        printf("create rga chn:[%d] failed\n", mDispPrevRgaChn);
        return -3;
    }

    // 裁剪 --> rga --> rga --> vo
    ret = getApi()->getRga().createRgaChn(mCropRgaChn, 3, 3, mOutputImagePixType, mOutputImagePixType, 0, RGA_FLIP_NULL, false,
        mOriginWidth, mOriginHeight, 0, 0, mOriginWidth, mOriginHeight);
    if (ret) {
        printf("create rga chn:[%d] failed\n", mCropRgaChn);
        return -4;
    }

    // 缩放 --> rga --> rga --> vo
    ret = getApi()->getRga().createRgaChn(mZoomRgaChn, 3, 3, mOutputImagePixType, mOutputImagePixType, 0, RGA_FLIP_NULL, mPrimaryDispSwap,
        mOriginWidth, mOriginHeight, 0, 0, mPrimaryDispSwap ? mPrimaryDispHeight : mPrimaryDispWidth, mPrimaryDispSwap ? mPrimaryDispWidth : mPrimaryDispHeight);
    if (ret) {
        printf("create rga chn:[%d] failed\n", mZoomRgaChn);
        return -5;
    }

    // 使能视频编码存储且非视频解码源
    if (!mEMSConfig.disableVideoEncoderSave && !mUseVdecNotVi) {
        ret = getApi()->getVenc().createVencChnEx(mVideoVencChn, 6, mOriginWidth, mOriginHeight, mInputImagePixType, mVideoVencType, mVideoVencRcMode,
            mEMSConfig.videoEncoderParam.videoFPS, mEMSConfig.videoEncoderParam.encodeProfile, videoEncodeProcessCallback, this,
            mEMSConfig.videoEncoderParam.encodeBitRate, mEMSConfig.videoEncoderParam.encodeIFrameInterval, 0);
        if (ret == 0) {
            bEnableVenc = true;
        } else {
            mVideoVencEnOK = false;
            printf("create venc chn:[%d] failed\n", mVideoVencChn);
        }
    }

    // RTSP
    if (!mEMSConfig.disableVideoRtspServer) {
        ret = getApi()->getVenc().createVencChnEx(mRtspVencChn, 3, mOriginWidth, mOriginHeight, mInputImagePixType, mVideoVencType, mVideoVencRcMode,
            mEMSConfig.videoRtspServerParam.videoFPS, mEMSConfig.videoRtspServerParam.encodeProfile, rtspEncodeProcessCallback, this,
            mEMSConfig.videoRtspServerParam.encodeBitRate, mEMSConfig.videoRtspServerParam.encodeIFrameInterval, 0);
        if (ret == 0) {
            ret = getApi()->getRga().createRgaChn(mRtspVencRgaChn, 6, 3, mOutputImagePixType, mInputImagePixType, 0, RGA_FLIP_NULL, false, 
                mOriginWidth, mOriginHeight, 0, 0, mOriginWidth, mOriginHeight, 0, 0);
            if (ret) {
                printf("create rga chn:[%d] failed\n", mRtspVencRgaChn);
            } else {
                bEnableRtsp = true;
            }
        } else {
            printf("create venc chn:[%d] failed\n", mRtspVencChn);
        }
    }

    ret = getApi()->getVo().createVoChn(mPrimaryVoChn, mPrimaryZpos, mPrimaryDispLayers, mPrimaryDispWidth, mPrimaryDispHeight, mPrimaryXoffset, mPrimaryYoffset,
        mPrimaryDispSwap, mOutputImagePixType, mDisplayDeviceCard.c_str());
    if (ret) {
        printf("create vo chn:[%d] failed", mPrimaryVoChn);
        return -6;
    }

    ret = getApi()->getVo().createVoChn(mOverlayVoChn, mOverlayZpos, mOverlayDispLayers, mOverlayDispWidth, mOverlayDispHeight, mOverlayXoffset, mOverlayYoffset,
        mOverlayDispSwap, mOutputImagePixType, mDisplayDeviceCard.c_str());
    if (ret) {
        printf("create vo chn:[%d] failed", mOverlayVoChn);
        return -7;
    }

    if (mUseVdecNotVi) {
        ret = getApi()->getSys().bindVdecRga(mVdecParameter[0].vdecChn, mInferRgaChn);
        if (ret) {
            printf("bind vdec chn:[%d] to rga chn:[%d] failed", mVdecParameter[0].vdecChn, mInferRgaChn);
            return -8;
        }

        ret = getApi()->getSys().bindVdecRga(mVdecParameter[0].vdecChn, mDispPrevRgaChn);
        if (ret) {
            printf("bind vi chn:[%d] to rga chn:[%d] failed", mVideoFirstInChn, mDispPrevRgaChn);
            return -9;
        }
    } else {
        ret = getApi()->getSys().bindViRga(mVideoFirstInChn, mInferRgaChn);
        if (ret) {
            printf("bind vi chn:[%d] to rga chn:[%d] failed", mVideoFirstInChn, mInferRgaChn);
            return -10;
        }

        ret = getApi()->getSys().bindViRga(mVideoFirstInChn, mDispPrevRgaChn);
        if (ret) {
            printf("bind vi chn:[%d] to rga chn:[%d] failed", mVideoFirstInChn, mDispPrevRgaChn);
            return -11;
        }

        if (bEnableVenc) {
            ret = getApi()->getSys().bindViVenc(mVideoFirstInChn, mVideoVencChn);
            if (ret) {
                mVideoVencEnOK = false;
                printf("bind vi chn:[%d] to venc chn:[%d] failed", mVideoFirstInChn, mVideoVencChn);
                return -12;
            }

            mVideoVencEnOK = true;
        }
    }

    // RTSP
    if (bEnableRtsp) {
        ret = getApi()->getSys().bindRgaVenc(mRtspVencRgaChn, mRtspVencChn);
        if (ret) {
            printf("bind rga chn:[%d] to venc chn:[%d] failed", mRtspVencRgaChn, mRtspVencChn);
        } else {
            mRtspVencEnOK = true;
        }
    }

    return 0;
}

void EMSDemoImpl::mediaDeinit()
{
    if (mUseVdecNotVi) {
        getApi()->getSys().unbindVdecRga(mVdecParameter[0].vdecChn, mInferRgaChn);
        getApi()->getSys().unbindVdecRga(mVdecParameter[0].vdecChn, mDispPrevRgaChn);
    } else {
        getApi()->getSys().unbindViRga(mVideoFirstInChn, mInferRgaChn);
        getApi()->getSys().unbindViRga(mVideoFirstInChn, mDispPrevRgaChn);

        getApi()->getSys().unbindViVenc(mVideoFirstInChn, mVideoVencChn);
    }

    getApi()->getSys().unbindRgaVenc(mRtspVencRgaChn, mRtspVencChn);

    getApi()->getVo().destroyVoChn(mPrimaryVoChn);
    getApi()->getVo().destroyVoChn(mOverlayVoChn);

    getApi()->getRga().destroyRgaChn(mInferRgaChn);
    getApi()->getRga().destroyRgaChn(mDispPrevRgaChn);

    getApi()->getRga().destroyRgaChn(mCropRgaChn);
    getApi()->getRga().destroyRgaChn(mZoomRgaChn);

    getApi()->getRga().destroyRgaChn(mRtspVencRgaChn);

    if (!mEMSConfig.disableVideoEncoderSave) {
        mVideoVencEnOK = false;
        getApi()->getVenc().destroyVencChn(mVideoVencChn);
    }

    if (!mEMSConfig.disableVideoRtspServer) {
        mRtspVencEnOK = false;
        getApi()->getVenc().destroyVencChn(mRtspVencChn);
    }

    if (mUseVdecNotVi) {
        getApi()->getVdec().destroyVdecChn(mVdecParameter[0].vdecChn);
    } else {
        getApi()->getVi().destroyViChn(mVideoFirstInChn);
    }
}

API_END_NAMESPACE(EMS)
