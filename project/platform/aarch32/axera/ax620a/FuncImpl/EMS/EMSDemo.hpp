#pragma once

#include <map>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cstring>

#include <utils/RingBuffer.hpp>
#include <mpi/axmpi/mediaBase.hpp>

namespace ems {
class EMSDemo : public axpi::MediaBase {
public:
    EMSDemo();
    ~EMSDemo();

    int init();

private:
    /* pipeline初始化 */
    int pipelineCreate();

    /* pipeline去初始化 */
    int pipelineRelease();

private:
    /* OSD绘制信息线程 */
    void osdProcessThread();

    /* 算法推理回调函数 */
    static void inferenceProcess(axpi::axpipe_buffer_t *buff, void *user_data, void *user_data2);

private:
    bool                                    mInitFin;           // EMS是否初始化完成
    bool                                    mAlgoInit;          // 算法是否初始化成功
    bool                                    mQuitThread;        // 线程是否可以退出了

    size_t                                  mAlgoWidth;         // 算法模型输入宽度
    size_t                                  mAlgoHeight;        // 算法模型输出高度

    int                                     mCamFPS;            // 摄像头帧率
    int                                     mIvpsFPS;           // IVPS输出帧率

    int                                     mVoRotateAngle;     // 输出显示屏旋转角度(0/90/180/270)
    int                                     mScreenWidth;       // 输出显示屏宽度
    int                                     mScreenHeight;      // 输出显示屏高度

    std::thread                             mOsdThreadId;       // OSD叠加信息线程句柄

    void                                    *mHandler;          // 算法持有句柄

    int                                     mStartID;           // pipeline特有ID
    std::vector<axpi::axpipe_t>             mPipelines;         // 初始化pipeline信息
    std::vector<axpi::axpipe_t *>           mPipesNeedOSD;      // 需要进行OSD的pipeline

    utils::RingBuffer<axpi::axpi_results_t> mJointResultsRing;  // 算法推理结果
};
}
