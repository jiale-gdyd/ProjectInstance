#ifndef ROCKCHIP_MEDIA_VDEC_HPP
#define ROCKCHIP_MEDIA_VDEC_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <map>
#include <thread>
#include <vector>
#include <string>
#include <pthread.h>
#include <stdbool.h>

#include <utils/export.h>
#include <media/drm_media_api.h>

typedef struct {
    int                      vdecChn;            // 解码通道
    bool                     bDecLoop;           // 周期性解码(如果只有一个解码文件，则周期性解码该文件，如果存在多个解码文件，则依次循环解码文件)
    bool                     hardwareAlloc;      // 硬件内存分配，默认为false
    size_t                   allocFlag;          // 内存分配标志，1: 没有缓存的属性; 2: 物理地址连续; 其他: 缓存缓冲区类型默认值
    size_t                   intervalMs;         // 解码间隔时间
    size_t                   oneFramSize;        // 解码一帧大小
    size_t                   videoWidth;         // 解码视频/图片像素宽度
    size_t                   videoHeight;        // 解码视频/图片像素高度
    drm_codec_type_e         codecType;          // 解码文件类型(h264、h265、jpeg、mjpeg)
    std::vector<std::string> codecFile;          // 解码文件
} media_vdec_param_t;

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaVdec {
public:
    MediaVdec();
    ~MediaVdec();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVdec(const MediaVdec &other) = delete;
    MediaVdec(MediaVdec &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVdec &operator=(const MediaVdec &other) = delete;
    MediaVdec &operator=(MediaVdec &&other) = delete;

    /**
     * 函数名称: destroyVdecChn
     * 功能描述: 销毁视频解码通道
     * 输入参数: vdecChn --> 视频解码通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int destroyVdecChn(int vdecChn);

    /**
     * 函数名称: createVdecChn
     * 功能描述: 创建并启动视频解码通道
     * 输入参数: vdecChn     --> 视频解码通道号
     *          enCodecType --> 解码文件类型
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVdecChn(int vdecChn, drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264);

    /**
     * 函数名称: createVdecChn
     * 功能描述: 创建并启动视频解码通道
     * 输入参数: vdecChn         --> 视频解码通道号
     *          fileName        --> 解码文件
     *          enCodecType     --> 解码文件类型
     *          decLoop         --> 周期性解码
     *          decOneFrameSize --> 解码一帧大小
     *          decInterMs      --> 帧解码间隔(毫秒)
     *          bHardware       --> 是否采用硬件方式分配内存
     *          allocFlag       --> 分配标志, 1: 没有缓存的属性; 2: 物理地址连续; 其他: 缓存缓冲区类型默认值
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVdecChn(int vdecChn, std::string fileName, drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264, bool decLoop = true, size_t decOneFrameSize = 6144, size_t decInterMs = 10, bool bHardware = false, size_t allocFlag = 0);
    int createVdecChn(int vdecChn, std::vector<std::string> fileName, drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264, bool decLoop = true, size_t decOneFrameSize = 6144, size_t decInterMs = 10, bool bHardware = false, size_t allocFlag = 0);

    /**
     * 函数名称: setVdecChnFrameRate
     * 功能描述: 设置vdec通道码率
     * 输入参数: vdecChn   --> vdec通道号
     *          fpsInNum  --> 输入帧率分子/时间线分子
     *          fpsInDen  --> 输入帧率分母/时间线分母
     *          fpsOutNum --> 输出帧率分子/时间线分子
     *          fpsOutDen --> 输出帧率分母/时间线分母
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setVdecChnFrameRate(int vdecChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen);

    /**
     * 函数名称: sendFrame
     * 功能描述: 将多媒体数据帧发送到指定的设备
     * 输入参数: modeIdChn    --> 设备通道号
     *          pstFrameInfo --> 多媒体数据帧指针
     *          modeId       --> 目标设备
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId = MOD_ID_VDEC);

    /**
     * 函数名称: getFrame
     * 功能描述: 获取多媒体数据帧
     * 输入参数: modeIdChn   --> 设备通道号
     *          s32MilliSec --> 获取数据帧的超时时间
     *          modeId      --> 数据源设备
     * 输出参数: 无
     * 返回说明: 成功返回获取的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getFrame(int modeIdChn, int s32MilliSec = -1, int modeId = MOD_ID_VDEC);

    /**
     * 函数名称: getFrameDataPtr
     * 功能描述: 获取指向多媒体数据帧数据的指针
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回指向多媒体数据帧数据的指针，失败返回NULL
     */
    void *getFrameDataPtr(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: getFrameData
     * 功能描述: 获取指向多媒体数据帧数据的指针
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回指向多媒体数据帧数据的指针，失败返回NULL
     */
    void *getFrameData(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: setFrameSize
     * 功能描述: 设置多媒体数据帧数据大小
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     *          size          --> 多媒体数据帧数据大小
     * 输出参数: 无
     * 返回说明: 成功返回0，其他失败
     */
    int setFrameSize(media_buffer_t pstFrameInfo, unsigned int size);

    /**
     * 函数名称: createFrame
     * 功能描述: 创建多媒体数据帧
     * 输入参数: size      --> 数据帧数据大小
     *           bHardware --> 是否采用硬件方式分配内存
     *           flag      --> 分配标志
     * 输出参数: 无
     * 返回说明: 成功返回只想多媒体数据帧得指针，失败返回NULL
     */
    media_buffer_t createFrame(unsigned int size, bool bHardware, unsigned char flag);

    /**
     * 函数名称: createImageFrame
     * 功能描述: 创建多媒体图像数据帧
     * 输入参数: pstImageInfo --> 图像信息
     *           bHardware    --> 是否采用硬件方式分配内存
     *           flag         --> 分配标志
     * 输出参数: 无
     * 返回说明: 成功返回只想多媒体数据帧得指针，失败返回NULL
     */
    media_buffer_t createImageFrame(mb_image_info_t *pstImageInfo, bool bHardware, unsigned char flag);

    /**
     * 函数名称: releaseFrame
     * 功能描述: 释放多媒体数据帧
     * 输入参数: pstFrameInfo --> 多媒体数据帧指针
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int releaseFrame(media_buffer_t pstFrameInfo);

private:
    /**
     * 函数名称: videoDecoderProcessThread
     * 功能描述: 视频及图片解码处理线程
     * 输入参数: param --> 视频解码线程参数
     * 输出参数: 无
     * 返回说明: 返回NULL
     */
    static void *videoDecoderProcessThread(void *param);
    static void *videoDecoderProcessThreadMulti(void *param);

private:
    typedef enum vdec_state {
        VDEC_STOP  = 0,                                             // 解码暂停
        VDEC_START = 1,                                             // 解码开始
        VDEC_PAUSE = 2,                                             // 解码暂停
        VDEC_QUIT  = 3,                                             // 解码退出
    } vdec_state_e;

private:
    bool         mVdecThreadQuit;                                   // 解码线程退出
    pthread_t    mVdecThreadId[DRM_VDEC_CHANNEL_BUTT];              // 视频解码处理线程句柄
    vdec_state_e mVdecChnState[DRM_VDEC_CHANNEL_BUTT];              // 解码进行状态
    int          mDecodeFileCount[DRM_VDEC_CHANNEL_BUTT];           // 解码文件数
    int          mCurrDecodeFileIndex[DRM_VDEC_CHANNEL_BUTT];       // 当前解码文件缩影
};

API_END_NAMESPACE(media)

#endif