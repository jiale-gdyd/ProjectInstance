#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::osdProcessThread()
{
    Ai::axdl_result_t lastResult;
    std::map<int, Ai::axdl_canvas_t> pipeOsdCanvas;
    std::map<int, media::axivps_rgn_disp_grp_t> pipeOsdStruct;

    for (size_t i = 0; i < mPipeNeedOsd.size(); ++i) {
        pipeOsdCanvas[mPipeNeedOsd[i]->pipeId];
        pipeOsdStruct[mPipeNeedOsd[i]->pipeId];

        auto &canvas = pipeOsdCanvas[mPipeNeedOsd[i]->pipeId];
        auto &display = pipeOsdStruct[mPipeNeedOsd[i]->pipeId];

        memset(&display, 0, sizeof(media::axivps_rgn_disp_grp_t));
        canvas.channels = 4;
        canvas.data = (uint8_t *)malloc(mPipeNeedOsd[i]->ivpsConfig.ivpsWidth * mPipeNeedOsd[i]->ivpsConfig.ivpsHeight * 4);
        canvas.width = mPipeNeedOsd[i]->ivpsConfig.ivpsWidth;
        canvas.height = mPipeNeedOsd[i]->ivpsConfig.ivpsHeight;
    }

    while (!mThreadFin) {
        if (!mAlgoResultRing.isEmpty() && mAlgoResultRing.remove(lastResult)) {
            for (size_t i = 0; i < mPipeNeedOsd.size(); ++i) {
                auto &osdPipe = mPipeNeedOsd[i];
                if (osdPipe && osdPipe->ivpsConfig.osdRegions) {
                    Ai::axdl_canvas_t &imgOverlay = pipeOsdCanvas[osdPipe->pipeId];
                    media::axivps_rgn_disp_grp_t &display = pipeOsdStruct[osdPipe->pipeId];

                    memset(imgOverlay.data, 0, imgOverlay.width * imgOverlay.height * imgOverlay.channels);

                    /* 画图: 之后需要封装画图函数 */
                    cv::Mat frameImage = cv::Mat(imgOverlay.height, imgOverlay.width, CV_8UC4, imgOverlay.data);

                    display.nNum = 1;
                    display.tChnAttr.nAlpha = 1024;
                    display.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
                    display.tChnAttr.nZindex = 1;
                    display.tChnAttr.nBitColor.nColor = 0xFF0000;
                    display.tChnAttr.nBitColor.bEnable = AX_FALSE;
                    display.tChnAttr.nBitColor.nColorInv = 0xFF;
                    display.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

                    display.arrDisp[0].bShow = AX_TRUE;
                    display.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;

                    display.arrDisp[0].uDisp.tOSD.bEnable = AX_TRUE;
                    display.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
                    display.arrDisp[0].uDisp.tOSD.u32Zindex = 1;
                    display.arrDisp[0].uDisp.tOSD.u32ColorKey = 0x0;
                    display.arrDisp[0].uDisp.tOSD.u32BgColorLo = 0xFFFFFFFF;
                    display.arrDisp[0].uDisp.tOSD.u32BgColorHi = 0xFFFFFFFF;
                    display.arrDisp[0].uDisp.tOSD.u32BmpWidth = imgOverlay.width;
                    display.arrDisp[0].uDisp.tOSD.u32BmpHeight = imgOverlay.height;
                    display.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                    display.arrDisp[0].uDisp.tOSD.u32DstYoffset = osdPipe->outType == media::AXPIPE_OUTPUT_VO_SIPEED_SCREEN ? 32 : 0;
                    display.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                    display.arrDisp[0].uDisp.tOSD.pBitmap = imgOverlay.data;
                }
            }
        }

        usleep(1000);
    }

    for (size_t i = 0; i < mPipeNeedOsd.size(); i++) {
        auto canvas = pipeOsdCanvas[mPipeNeedOsd[i]->pipeId];
        free(canvas.data);
    }
}

API_END_NAMESPACE(EMS)
