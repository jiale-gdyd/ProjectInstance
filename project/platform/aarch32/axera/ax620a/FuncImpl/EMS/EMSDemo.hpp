#ifndef AX620A_FUNCIMPL_EMS_DEMO_HPP
#define AX620A_FUNCIMPL_EMS_DEMO_HPP

#include <map>
#include <vector>
#include <memory>
#include <string.h>
#include <unistd.h>
#include <rtsp/rtsp.hpp>
#include <npu/Detector.hpp>
#include <opencv2/opencv.hpp>
#include <utils/RingBuffer.hpp>
#include <mpi/axmpi/mediaBase.hpp>

API_BEGIN_NAMESPACE(EMS)

class EMSDemoImpl : public media::MediaBase {
public:
    EMSDemoImpl();
    ~EMSDemoImpl();

    virtual int init() override;

private:
    int mediaInit();
    void mediaDeinit();

private:
    void osdProcessThread();
    static void inferenceProcess(media::axpipe_buffer_t *buff, void *user_data);
    static void RtspFrameHandler(void *arg, const char *tag, int frameType, int64_t timestamp, uint8_t *frame, uint32_t frameSize);

private:
    bool                                 mThreadFin;         // 线程退出标志
    std::thread                          mOsdThreadId;       // OSD线程句柄
    std::vector<media::axpipe_t>         mPipelines;         // pipeline信息
    std::vector<media::axpipe_t *>       mPipeNeedOsd;       // pipeline OSD

    int                                  mModelWidth;        // 模型输入宽度
    int                                  mModelHeight;       // 模型输入高度
    int                                  mModelFormat;       // 模型输入颜色
    int                                  mModelAlgoType;     // 模型算法类型
    std::shared_ptr<Ai::AiDetector>      pDetector;          // 算法检测类
    utils::RingBuffer<Ai::axdl_result_t> mAlgoResultRing;    // 算法结果

    rtsp::RTSPClient                     *mRtspClient;       // RTSP客户端句柄
};

API_END_NAMESPACE(EMS)

#endif
