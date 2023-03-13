#include "EMSDemo.hpp"

namespace EMS {
void EMSDemo::osdProcessThread()
{
    axpi::axpi_results_t results;
    std::map<int, axpi::axpi_canvas_t> pipesOsdCanvas;
    std::map<int, axpi::axivps_rgn_disp_grp_t> pipesOsdStruct;

    for (size_t i = 0; i < mPipesNeedOSD.size(); ++i) {
        pipesOsdCanvas[mPipesNeedOSD[i]->pipeId];
        pipesOsdStruct[mPipesNeedOSD[i]->pipeId];

        auto &canvas = pipesOsdCanvas[mPipesNeedOSD[i]->pipeId];
        auto &display = pipesOsdStruct[mPipesNeedOSD[i]->pipeId];

        memset(&display, 0, sizeof(axpi::axivps_rgn_disp_grp_t));
        canvas.channel = 4;
        canvas.width = mPipesNeedOSD[i]->ivpsConfig.ivpsWidth;
        canvas.height = mPipesNeedOSD[i]->ivpsConfig.ivpsHeight;
        canvas.data = new unsigned char[canvas.width * canvas.height * 4];
    }

    while (!mQuitThread) {
        memset(&results, 0, sizeof(results));
        if (!mJointResultsRing.isEmpty() && mJointResultsRing.remove(results)) {
            for (size_t i = 0; i < mPipesNeedOSD.size(); ++i) {
                auto &osdPipe = mPipesNeedOSD[i];
                if (osdPipe && osdPipe->ivpsConfig.osdRegions) {
                    axpi::axpi_canvas_t &imageOverlay = pipesOsdCanvas[osdPipe->pipeId];
                    axpi::axivps_rgn_disp_grp_t &display = pipesOsdStruct[osdPipe->pipeId];

                    if (imageOverlay.data) {
                        memset(imageOverlay.data, 0, imageOverlay.width * imageOverlay.height * imageOverlay.channel);
                        axpi::axpi_draw_results(mHandler, &imageOverlay, &results, 0.6, 1.0, 0, 0);

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
                        display.arrDisp[0].uDisp.tOSD.u32BmpWidth = imageOverlay.width;
                        display.arrDisp[0].uDisp.tOSD.u32BmpHeight = imageOverlay.height;
                        display.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                        display.arrDisp[0].uDisp.tOSD.u32DstYoffset = osdPipe->outType == axpi::AXPIPE_OUTPUT_VO_SIPEED_SCREEN ? 32 : 0;
                        display.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                        display.arrDisp[0].uDisp.tOSD.pBitmap = imageOverlay.data;

                        int ret = getApi()->getIvps().updateRegion(osdPipe->ivpsConfig.osdRgnHandle[0], &display);
                        if (ret != 0) {
                            static int failedCount = 0;
                            if ((++failedCount % 100) == 0) {
                                printf("ivps region update failed, return:[%d], handler:[%d]\n", failedCount, osdPipe->ivpsConfig.osdRgnHandle[0]);
                            }

                            std::this_thread::sleep_for(std::chrono::milliseconds(30));
                        }
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    for (size_t i = 0; i < mPipesNeedOSD.size(); ++i) {
        auto &canvas = pipesOsdCanvas[mPipesNeedOSD[i]->pipeId];
        if (canvas.data) {
            delete[] canvas.data;
        }
    }
}
}
