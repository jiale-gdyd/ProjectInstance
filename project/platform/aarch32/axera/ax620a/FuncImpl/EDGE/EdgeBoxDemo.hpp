#pragma once

#include <map>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cstring>

#include <rtsp/rtsp.hpp>
#include <utils/RingBuffer.hpp>
#include <mpi/axmpi/mediaBase.hpp>

namespace edge {
#define MAX_PIPELINE            (2)
#define MAX_RTSPURLS            (4)

#define POINTER_TO_INT(p)       ((int)(long)(p))
#define POINTER_TO_UINT(p)      ((unsigned int)(unsigned long)(p))

#define INT_TO_POINTER(i)       ((void *)(long)(i))
#define UINT_TO_POINTER(u)      ((void *)(unsigned long)(u))

class EdgeBoxDemo : public axpi::MediaBase {
public:
    EdgeBoxDemo();
    ~EdgeBoxDemo();

    int init();

private:
    /* pipeline初始化 */
    int pipelineCreate();

    /* pipeline去初始化 */
    int pipelineRelease();

private:
    /* rtsp client初始化 */
    int rtspClientCreate();

    /* rtsp client去初始化 */
    int rtspClientRelease();

private:
    /* OSD绘制信息线程 */
    void osdProcessThread(int threadId);

    /* 算法推理回调函数 */
    static void inferenceProcess(axpi::axpipe_buffer_t *buff, void *user_data, void *user_data2);

    /* rtsp帧回调处理函数 */
    static void rtspClientFrameHandler(void *arg, const char *tag, int frameType, int64_t timestamp, uint8_t *frame, uint32_t frameSize);

private:
    bool                                     mInitFin;                          // 是否初始化完成
    bool                                     mAlgoInit;                         // 算法是否初始化成功
    bool                                     mQuitThread;                       // 线程是否可以退出了

    size_t                                   mAlgoWidth;                        // 算法模型输入宽度
    size_t                                   mAlgoHeight;                       // 算法模型输出高度

    int                                      mCamFPS;                           // 摄像头帧率
    int                                      mIvpsFPS;                          // IVPS输出帧率

    void                                     *mHandler;                         // 算法持有句柄
    std::vector<std::vector<axpi::axpipe_t>> mPipelines;                        // 初始化pipeline信息

    std::vector<std::thread>                 mOsdThreadId;                      // OSD叠加信息线程句柄
    std::vector<axpi::axpipe_t *>            mPipesNeedOSD[MAX_RTSPURLS];       // 需要进行OSD的pipeline

    std::vector<std::string>                 mRtspURL;                          // rtspURL
    std::vector<rtsp::RTSPClient *>          mRtspClient;                       // RTSP客户端

    utils::RingBuffer<axpi::axpi_results_t>  mJointResultsRing[MAX_RTSPURLS];   // 算法推理结果
};
}
