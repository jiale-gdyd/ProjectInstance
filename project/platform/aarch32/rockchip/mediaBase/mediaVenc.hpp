#ifndef ROCKCHIP_MEDIA_VENC_HPP
#define ROCKCHIP_MEDIA_VENC_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <stdbool.h>
#include <utils/export.h>
#include <media/drm_media_api.h>

API_BEGIN_NAMESPACE(media)

class API_EXPORT MediaVenc {
public:
    MediaVenc();
    ~MediaVenc();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVenc(const MediaVenc &other) = delete;
    MediaVenc(MediaVenc &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVenc &operator=(const MediaVenc &other) = delete;
    MediaVenc &operator=(MediaVenc &&other) = delete;

    /**
     * 函数名称: vencChnStart
     * 功能描述: 判断视频编码通道启动状态
     * 输入参数: vencChn --> 视频编码通道号
     * 输出参数: 无
     * 返回说明: 已经启动返回true，否则返回false
     */
    bool vencChnStart(int vencChn);

    /**
     * 函数名称: destroyVencChn
     * 功能描述: 销毁视频编码通道
     * 输入参数: vencChn --> 视频编码通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int destroyVencChn(int vencChn);

    /**
     * 函数名称: createVencChn
     * 功能描述: 创建视频编码通道
     * 输入参数: vencChn       --> 视频编码通道号
     *           stVencChnAttr --> 视频编码属性
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVencChn(int vencChn, drm_venc_chn_attr_t *stVencChnAttr);

    /**
     * 函数名称: createVencChn
     * 功能描述: 创建视频编码通道
     * 输入参数: vencChn      --> 编码通道
     *           bufDepth     --> 设置缓存深度
     *           frameWidth   --> 编码帧宽度
     *           frameHeight  --> 编码帧高度
     *           imageType    --> 图像格式
     *           enCodecType  --> 编码类型
     *           rcMode       --> 编码模式
     *           frameRate    --> 编码帧率
     *           profile      --> H.264: 66[baseline]; 77[MP]; 100[HP];  H.265: default:Main;  Jpege/MJpege: default:Baseline
     *           callback     --> 回调处理函数
     *           bitrate      --> 编码码率
     *           iFrmInterval --> I帧间隔或islice帧间隔[1, 65536]
     *           rotation     --> 旋转角度(0, 90, 180, 270)
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVencChn(int vencChn, int bufDepth = 3,
        size_t frameWidth = 1280,
        size_t frameHeight = 720,
        drm_image_type_e imageType = DRM_IMAGE_TYPE_NV12,
        drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264,
        drm_venc_rc_mode_e rcMode = VENC_RC_MODE_H264VBR,
        size_t frameRate = 30,
        size_t profile = 66,
        OutCallbackFunction callback = NULL,
        size_t bitrate = 8000000,
        size_t iFrmInterval = 30,
        int rotation = VENC_ROTATION_0);

    /**
     * 函数名称: createVencChnEx
     * 功能描述: 创建视频编码通道
     * 输入参数: vencChn      --> 编码通道
     *           bufDepth     --> 设置缓存深度
     *           frameWidth   --> 编码帧宽度
     *           frameHeight  --> 编码帧高度
     *           imageType    --> 图像格式
     *           enCodecType  --> 编码类型
     *           rcMode       --> 编码模式
     *           frameRate    --> 编码帧率
     *           profile      --> H.264: 66[baseline]; 77[MP]; 100[HP];  H.265: default:Main;  Jpege/MJpege: default:Baseline
     *           callback     --> 回调处理函数
     *           userData     --> 回调函数用户数据
     *           bitrate      --> 编码码率
     *           iFrmInterval --> I帧间隔或islice帧间隔[1, 65536]
     *           rotation     --> 旋转角度(0, 90, 180, 270)
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVencChnEx(int vencChn, int bufDepth = 3,
        size_t frameWidth = 1280,
        size_t frameHeight = 720,
        drm_image_type_e imageType = DRM_IMAGE_TYPE_NV12,
        drm_codec_type_e enCodecType = DRM_CODEC_TYPE_H264,
        drm_venc_rc_mode_e rcMode = VENC_RC_MODE_H264VBR,
        size_t frameRate = 30,
        size_t profile = 66,
        OutCallbackFunctionEx callback = NULL,
        void *userData = NULL,
        size_t bitrate = 8000000,
        size_t iFrmInterval = 30,
        int rotation = VENC_ROTATION_0);

    /**
     * 函数名称: setVencChnFrameRate
     * 功能描述: 设置venc通道码率
     * 输入参数: vencChn   --> venc通道号
     *          fpsInNum  --> 输入帧率分子/时间线分子
     *          fpsInDen  --> 输入帧率分母/时间线分母
     *          fpsOutNum --> 输出帧率分子/时间线分子
     *          fpsOutDen --> 输出帧率分母/时间线分母
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setVencChnFrameRate(int vencChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen);

    /**
     * 函数名称: sendFrame
     * 功能描述: 将多媒体数据帧发送到指定的设备
     * 输入参数: modeIdChn    --> 设备通道号
     *          pstFrameInfo --> 多媒体数据帧指针
     *          modeId       --> 目标设备
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId = MOD_ID_VENC);

    /**
     * 函数名称: getFrame
     * 功能描述: 获取多媒体数据帧
     * 输入参数: modeIdChn   --> 设备通道号
     *          s32MilliSec --> 获取数据帧的超时时间
     *          modeId      --> 数据源设备
     * 输出参数: 无
     * 返回说明: 成功返回获取的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getFrame(int vencChn, int s32MilliSec = -1, int modeId = MOD_ID_VENC);

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
     * 返回说明: 成功返回0,其他则失败
     */
    int releaseFrame(media_buffer_t pstFrameInfo);

private:
    bool mVencChnStarted[DRM_VENC_CHANNEL_BUTT];    // 视频编码通道启动状态
};

API_END_NAMESPACE(media)

#endif
