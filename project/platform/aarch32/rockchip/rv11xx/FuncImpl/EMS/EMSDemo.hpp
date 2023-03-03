#ifndef RV11XX_FUNCIMPL_EMS_DEMO_HPP
#define RV11XX_FUNCIMPL_EMS_DEMO_HPP

#include <linux/kconfig.h>

#include <map>
#include <vector>
#include <string>
#include <thread>
#include <fstream>
#include <algorithm>
#include <functional>
#include <unordered_map>

#include <unistd.h>
#include <sys/sysinfo.h>

#include <xlib/xlib.h>
#include <npu/Detector.hpp>
#include <opencv2/opencv.hpp>
#include <utils/RingBuffer.hpp>
#include <mpi/rkmpi/mediaBase.hpp>

#include <rtsp/rtsp.hpp>
#include <rtsp/SimpleRtspServer.hpp>

#include "EMSDemoMacros.h"
#include "EMSDemoParam.hpp"

API_BEGIN_NAMESPACE(EMS)

typedef std::map<std::string, std::vector<std::string>, std::greater<std::string>> video_map_vector_t;

class EMSDemoImpl : public media::MediaBase {
public:
    EMSDemoImpl();
    ~EMSDemoImpl();

    virtual int init();

private:
    int dispObjects(std::vector<Ai::bbox> lastResult);

private:
    int mediaInit();
    void mediaDeinit();

private:
    int rtspServerInit();
    static void rtspEncodeProcessCallback(media_buffer_t mediaFrame, void *user_data);

    int rtspClientInit();
    static void rtspClientFrameHandler(void *arg, int frameType, int64_t timestamp, uint8_t *frame, uint32_t frameSize);

private:
    void videoEncodeInit();
    void videoEncodeExit();

    int getDvrRecordList(std::string rootDir);

    void detectTFCardProcessThread();
    void saveEncVideoToTFCardProcessThread();

    static void videoEncodeProcessCallback(media_buffer_t mediaFrame, void *user_data);

    void videoRemoveProcessThread();
    void videoRemovePreProcessThread();

private:
    void algoInitThread();

    void dispPostThread();
    void drawPostThread();
    void inferPostThread();

    void dispStreamCaptureThread();
    void inferStreamCaptureThread();

private:
    void parseVideoDecodeParam(std::string confJsonFile);

    void parseApplicationConfigParam(std::string confJsonFile);
    void generateApplicationConfigParam(ems_config_t configParam);

private:
    void sendZoomFrame(media_buffer_t &mediaFrame, int voChn = DRM_VO_CHANNEL_00, bool bDisplay = false, int rgaCropChn = DRM_RGA_CHANNEL_14, int rgaZoomChn = DRM_RGA_CHANNEL_15);
    void sendRawFrame(media_buffer_t &mediaFrame, bool bDisplay, bool bFreeFrame = true, int voChn = DRM_VO_CHANNEL_01, int zPos = 1, drm_plane_type_e enDispLayer = VO_PLANE_OVERLAY, size_t dispWidth = 1280, size_t dispHeight = 720, size_t dispXoffset = 0, size_t dispYoffset = 0, bool bDispSwap = false, drm_image_type_e enDispType = DRM_IMAGE_TYPE_RGB888);

private:
    bool                                     mThreadQuit;                    // 线程退出标志
    bool                                     mAlgoInitOk;                    // 算法初始化完成

    size_t                                   mOriginChns;                    // 原始图像通道数
    size_t                                   mOriginWidth;                   // 经过前端处理后的实际图像宽度
    size_t                                   mOriginHeight;                  // 经过前端处理后的实际图像高度

    size_t                                   mPrimaryXoffset;                // primary层显示X偏移
    size_t                                   mPrimaryYoffset;                // primary层显示X偏移
    size_t                                   mPrimaryDispWidth;              // primary层显示宽度
    size_t                                   mPrimaryDispHeight;             // primary层显示高度
    size_t                                   mOverlayXoffset;                // overlay层显示X偏移
    size_t                                   mOverlayYoffset;                // overlay层显示X偏移
    size_t                                   mOverlayDispWidth;              // overlay层显示宽度
    size_t                                   mOverlayDispHeight;             // overlay层显示高度

    int                                      mCropRgaChn;                    // RGA裁剪通道
    int                                      mZoomRgaChn;                    // RGA缩放通道
    int                                      mInferRgaChn;                   // RGA推理通道
    int                                      mDispPrevRgaChn;                // RGA预显示通道
    int                                      mVideoFirstInChn;               // 视频输入源通道

    size_t                                   mVideoInputWidth;               // 视频输入宽度
    size_t                                   mVideoInputHeight;              // 视频输入高度
    std::string                              mVideoInputDevNode;             // 视频输入设备节点

    drm_rga_flip_e                           mImageFlipMode;                 // 图像镜像模式
    drm_image_type_e                         mVdecImagePixType;              // 编码图像类型
    drm_image_type_e                         mInferImagePixType;             // 推理像素类型
    drm_image_type_e                         mInputImagePixType;             // 输入像素类型
    drm_image_type_e                         mInputImagePixTypeEx;           // 输入像素类型(扩展)
    drm_image_type_e                         mOutputImagePixType;            // 输出像素类型

    bool                                     mPrimaryDispSwap;               // primary层宽高显示是否交换
    bool                                     mOverlayDispSwap;               // overlay层宽高显示是否交换
    std::string                              mDisplayDeviceCard;             // 显示视频卡设备名称

    int                                      mOverlayZpos;                   // Overlay层所处Z轴
    int                                      mPrimaryZpos;                   // Primary层所处Z轴
    int                                      mOverlayVoChn;                  // Overlay层输出通道
    int                                      mPrimaryVoChn;                  // Primary层输出通道
    bool                                     mOverlayVoDisp;                 // 是否显示Overlay层
    bool                                     mPrimaryVoDisp;                 // 是否显示Primary层
    drm_plane_type_e                         mPrimaryDispLayers;             // Primary显示层
    drm_plane_type_e                         mOverlayDispLayers;             // Overlay显示层

    bool                                     mVideoVencEnOK;                 // 视频编码通道使能成功
    int                                      mVideoVencChn;                  // 视频编码通道
    drm_codec_type_e                         mVideoVencType;                 // 编码类型
    drm_venc_rc_mode_e                       mVideoVencRcMode;               // 编码模式

    std::thread                              mDrawPostThreadId;
    std::thread                              mDispPostThreadId;              // 显示处理线程句柄
    std::thread                              mInferPostThreadId;             // 推理后处理线程句柄
    std::thread                              mDispFrameCapThreadId;          // 显示捕获视频帧线程句柄
    std::thread                              mInferFrameCapThreadId;         // 推理捕获视频帧线程句柄

    std::thread                              mVideoRemoveThreadId;           // 移除视频线程句柄
    std::thread                              mVideoRemovePreprocThreadId;    // 移除视频前处理线程句柄

    std::thread                              mVideoDetectTFCardThreadId;     // 检测TF卡线程句柄
    std::thread                              mSaveVideo2TFCardThreadId;      // 存储是到TF卡线程句柄

    utils::RingBuffer<int>                   mAlgForwardFishRing;            // 算法前处理完成标志
    utils::RingBuffer<media_buffer_t>        mDispRawFrameRing;              // 捕获的视频原始帧队列
    utils::RingBuffer<media_buffer_t>        mSendDrawFrameRing;             // 发送绘制完成的帧队列
    utils::RingBuffer<std::vector<Ai::bbox>> mAlgExtractResultRing;          // cpu后处理结果

    bool                                     mVideoRemoveStart;              // 删除视频开始
    bool                                     mVideoEncodeQuit;               // 编码退出
    bool                                     mVideoEncodeStart;              // 编码开始
    bool                                     mVideoEncodeCloseFile;          // 关闭编码文件

    utils::RingBuffer<media_buffer_t>        mVideoVencFrame;                // 编码视频帧
    utils::RingBuffer<std::string>           mSaveVideoFileName;             // 保存的视频文件名

    utils::RingBuffer<bool>                  mRemoveVideoRing;               // 删除视频
    utils::RingBuffer<std::string>           mRemoveVideoName;               // 需要删除的视频文件或文件夹

    std::mutex                               mGetDvrVideoMutex;              // 获取视频列表互斥锁
    video_map_vector_t                       mVideoMapVecList;               // 录像视频文件列表信息
    std::vector<std::string>                 mVideolistVector;               // 单个文件夹内视频详细列表

    bool                                     mDetectTFCardWinthin;           // 检测到TF卡已插入
    size_t                                   mInsertTFCardFreeCapMB;         // 插入的TF检测到剩余容量(MB)
    size_t                                   mInsertTFCardTotalCapMB;        // 插入的TF检测到总容量(MB)
    size_t                                   mInsertTFCardFreeCapGB;         // 插入的TF检测到剩余容量(GB)
    size_t                                   mInsertTFCardTotalCapGB;        // 插入的TF检测到总容量(GB)

    bool                                     mUseVdecNotVi;                  // 使用视频解码作为视频源
    std::vector<media_vdec_param_t>          mVdecParameter;                 // 解码参数信息

    XMainLoop                                *mMainLoop;                     // 事件循环句柄
    XMainContext                             *mMainContex;                   // 程序上下文句柄

    static ems_config_t                      mEMSConfig;                     // 应用配置参数

    std::shared_ptr<Ai::AiDetector>          pDetector;                      // 算法检测类

    bool                                     mRtspVencEnOK;                  // 视频编码通道使能成功
    int                                      mRtspVencChn;                   // 视频编码通道
    int                                      mRtspVencRgaChn;                // 视频编码RGA通道
    std::shared_ptr<rtsp::SimpleRTSPServer>  pSimpleServer;                  // RTSP服务器句柄

    std::shared_ptr<rtsp::RTSPClient>        pRTSPClient;                    // RTSP客户端

    cv::Mat                                  mBlendImage;                    // 混合图片
};

API_END_NAMESPACE(EMS)

#endif
