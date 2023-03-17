#include "EdgeBoxDemo.hpp"

namespace edge {
void EdgeBoxDemo::osdProcessThread(int threadId)
{
    axpi::axpi_results_t result;
    std::map<int, axpi::axpi_canvas_t> pipesOsdCanvas;
    std::map<int, axpi::axivps_rgn_disp_grp_t> pipesOsdStruct;

    for (size_t i = 0; i < mPipesNeedOSD[threadId].size(); ++i) {
        pipesOsdCanvas[mPipesNeedOSD[threadId][i]->pipeId];
        pipesOsdStruct[mPipesNeedOSD[threadId][i]->pipeId];

        auto &canvas = pipesOsdCanvas[mPipesNeedOSD[threadId][i]->pipeId];
        auto &display = pipesOsdStruct[mPipesNeedOSD[threadId][i]->pipeId];
        memset(&display, 0, sizeof(axpi::axivps_rgn_disp_grp_t));

        canvas.channel = 4;
        canvas.width = mPipesNeedOSD[threadId][i]->ivps.width;
        canvas.height = mPipesNeedOSD[threadId][i]->ivps.height;
        canvas.data = new unsigned char[canvas.width * canvas.height * canvas.channel];
    }

    while (!mQuitThread) {
        if (!mJointResultsRing[threadId].isEmpty() && mJointResultsRing[threadId].remove(result)) {
            for (size_t i = 0; i < mPipesNeedOSD[threadId].size(); ++i) {
                auto &osdPipe = mPipesNeedOSD[threadId][i];
                if (osdPipe && osdPipe->ivps.regions) {
                    axpi::axpi_canvas_t &imageOverlay = pipesOsdCanvas[osdPipe->pipeId];
                    axpi::axivps_rgn_disp_grp_t &dispaly = pipesOsdStruct[osdPipe->pipeId];

                    if (imageOverlay.data) {
                        memset(imageOverlay.data, 0, imageOverlay.width * imageOverlay.height * imageOverlay.channel);
                        axpi::axpi_draw_results(mHandler, &imageOverlay, &result, 0.6, 1.0, 0, 0);

                        dispaly.nNum = 1;
                        dispaly.tChnAttr.nAlpha = 1024;
                        dispaly.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
                        dispaly.tChnAttr.nZindex = 1;
                        dispaly.tChnAttr.nBitColor.nColor = 0xFF0000;
                        dispaly.tChnAttr.nBitColor.bEnable = AX_FALSE;
                        dispaly.tChnAttr.nBitColor.nColorInv = 0xFF;
                        dispaly.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

                        dispaly.arrDisp[0].bShow = AX_TRUE;
                        dispaly.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
                        dispaly.arrDisp[0].uDisp.tOSD.bEnable = AX_TRUE;
                        dispaly.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
                        dispaly.arrDisp[0].uDisp.tOSD.u32Zindex = 1;
                        dispaly.arrDisp[0].uDisp.tOSD.u32ColorKey = 0x0;
                        dispaly.arrDisp[0].uDisp.tOSD.u32BgColorLo = 0xFFFFFFFF;
                        dispaly.arrDisp[0].uDisp.tOSD.u32BgColorHi = 0xFFFFFFFF;
                        dispaly.arrDisp[0].uDisp.tOSD.u32BmpWidth = imageOverlay.width;
                        dispaly.arrDisp[0].uDisp.tOSD.u32BmpHeight = imageOverlay.height;
                        dispaly.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                        dispaly.arrDisp[0].uDisp.tOSD.u32DstYoffset = osdPipe->outType == axpi::AXPIPE_OUTPUT_VO_SIPEED_SCREEN ? 32 : 0;
                        dispaly.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                        dispaly.arrDisp[0].uDisp.tOSD.pBitmap = imageOverlay.data;

                        int ret = getApi()->getIvps().updateRegion(osdPipe->ivps.handler[0], &dispaly);
                        if (ret != 0) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(30));
                        }
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    for (size_t i = 0; i < mPipesNeedOSD[threadId].size(); ++i) {
        auto &canvas = pipesOsdCanvas[mPipesNeedOSD[threadId][i]->pipeId];
        if (canvas.data) {
            delete[] canvas.data;
        }
    }
}
}
