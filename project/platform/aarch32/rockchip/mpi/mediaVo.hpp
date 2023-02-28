#ifndef ROCKCHIP_MEDIA_VO_HPP
#define ROCKCHIP_MEDIA_VO_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <stdbool.h>
#include <utils/export.h>
#include <media/drm_media_api.h>

API_BEGIN_NAMESPACE(media)

class API_EXPORT MediaVo {
public:
    MediaVo();
    ~MediaVo();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVo(const MediaVo &other) = delete;
    MediaVo(MediaVo &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVo &operator=(const MediaVo &other) = delete;
    MediaVo &operator=(MediaVo &&other) = delete;

    /**
     * 函数名称: voChnStart
     * 功能描述: 判断vo设备是否已经启动
     * 输入参数: voChn --> vo通道号
     * 输出参数: 无
     * 返回说明: 已经启动返回true，否则返回false
     */
    bool voChnStart(int voChn);

    /**
     * 函数名称: destroyVoChn
     * 功能描述: 销毁vo通道
     * 输入参数: voChn --> vo通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int destroyVoChn(int voChn);

    /**
     * 函数名称: createVoChn
     * 功能描述: 创建并启动vo通道
     * 输入参数: voChn       --> vo通道号
     *          zPOS        --> vo所属的ZPOS
     *          enDispLayer --> 显示层
     *          dispWidth   --> 显示宽度
     *          dispHeight  --> 显示高度
     *          dispXoffset --> 显示起始X偏移
     *          dispYoffset --> 显示起始Y偏移
     *          bWH2HW      --> 是否将输出的宽高进行调换
     *          enPixFmt    --> 显示像素格式
     *          dispCard    --> 显示卡
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVoChn(int voChn, int zPOS = 0, drm_plane_type_e enDispLayer = VO_PLANE_PRIMARY,
        size_t dispWidth = 1440, size_t dispHeight = 810, size_t dispXoffset = 0, size_t dispYoffset = 0,
        bool bWH2HW = false, drm_image_type_e enPixFmt = DRM_IMAGE_TYPE_RGB888,
        const char *dispCard = "/dev/dri/card0");

    /**
     * 函数名称: setVoChnFrameRate
     * 功能描述: 设置vo通道码率
     * 输入参数: voChn    --> vo通道号
     *          fpsInNum  --> 输入帧率分子/时间线分子
     *          fpsInDen  --> 输入帧率分母/时间线分母
     *          fpsOutNum --> 输出帧率分子/时间线分子
     *          fpsOutDen --> 输出帧率分母/时间线分母
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setVoChnFrameRate(int voChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen);

    /**
     * 函数名称: convertVoChn
     * 功能描述: 将多媒体数据经过rga处理之后再发送到vo通道
     * 输入参数: voChn       --> vo通道号
     *          rgaChn      --> rga通道号
     *          mediaFrame  --> 处理前的多媒体数据帧指针
     *          s32MilliSec --> 从rga通道获取数据的超时时间
     *          waitConvUs  --> 等待rga转换完成时间
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int convertVoChn(int voChn, int rgaChn, media_buffer_t mediaFrame, int s32MilliSec = -1, size_t waitConvUs = 50);

    /**
     * 函数名称: cropZoomVoChn
     * 功能描述: 将原始数据帧先经过rga裁剪之后再经过rga放大，最后输出
     * 输入参数: voChn       --> vo通道号
     *          cropRgaChn  --> rga裁剪通道
     *          zoomRgaChn  --> rga放大通道
     *          mediaFrame  --> 处理前的多媒体数据帧指针
     *          s32MilliSec --> 从rga通道获取数据的超时时间
     *          waitConvUs  --> 等待rga转换完成时间
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int cropZoomVoChn(int voChn, int cropRgaChn, int zoomRgaChn, media_buffer_t mediaFrame, int s32MilliSec = -1, size_t waitConvUs = 50);

    /**
     * 函数名称: sendZoomVoChnWithBind
     * 功能描述: 将cropRgaChn绑定到zoomRgaChn，然后将zoomRgaChn绑定到voChn。如果都绑定成功，后续只需要发送媒体数据帧到cropRgaChn通道即可
     * 输入参数: mediaFrame  --> 处理前的多媒体数据帧指针
     *          voChn       --> vo通道号
     *          cropRgaChn  --> rga裁剪通道
     *          zoomRgaChn  --> rga放大通道
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendZoomVoChnWithBind(media_buffer_t mediaFrame, int voChn, int cropRgaChn, int zoomRgaChn);

    /**
     * 函数名称: sendZoomRgaVmixVoChnWithBind
     * 功能描述: 将cropRgaChn绑定到zoomRgaChn，然后将zoomRgaChn绑定到[vmixDev:vmixChn], 最终[vmixDev:vmixChn]绑定到voChn。如果都绑定成功，后续只需要发送媒体数据帧到cropRgaChn通道即可
     * 输入参数: mediaFrame  --> 处理前的多媒体数据帧指针
     *          vmixDev     --> 视频混屏设备号
     *          vmixChn     --> 视频混屏通道号
     *          voChn       --> vo通道号
     *          cropRgaChn  --> rga裁剪通道
     *          zoomRgaChn  --> rga放大通道
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendZoomRgaVmixVoChnWithBind(media_buffer_t mediaFrame, int vmixDev, int vmixChn, int voChn, int cropRgaChn, int zoomRgaChn);

    /**
     * 函数名称: sendFrame
     * 功能描述: 将多媒体数据帧发送到指定的设备
     * 输入参数: modeIdChn    --> 设备通道号
     *          pstFrameInfo --> 多媒体数据帧指针
     *          modeId       --> 目标设备
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId = MOD_ID_VO);

    /**
     * 函数名称: getFrame
     * 功能描述: 获取多媒体数据帧
     * 输入参数: modeIdChn   --> 设备通道号
     *          s32MilliSec --> 获取数据帧的超时时间
     *          modeId      --> 数据源设备
     * 输出参数: 无
     * 返回说明: 成功返回获取的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getFrame(int modeIdChn, int s32MilliSec = -1, int modeId = MOD_ID_VO);

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
     * 函数名称: releaseFrame
     * 功能描述: 释放多媒体数据帧
     * 输入参数: pstFrameInfo --> 多媒体数据帧指针
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int releaseFrame(media_buffer_t pstFrameInfo);

private:
    int  mZoomVoChn[DRM_VO_CHANNEL_BUTT];                                                   // VO输出通道
    int  mCropRgaChn[DRM_VO_CHANNEL_BUTT];                                                  // RGA裁剪通道
    int  mZoomRgaChn[DRM_VO_CHANNEL_BUTT];                                                  // RGA缩放通道

    bool mBindRgaVo[DRM_VO_CHANNEL_BUTT];                                                   // 是否已经绑定
    bool mBindRgaRga[DRM_VO_CHANNEL_BUTT];                                                  // 是否已经绑定
    bool mVoChnStarted[DRM_VO_CHANNEL_BUTT];                                                // 视频输出通道启动状态

    int  mZoomVoChn2[DRM_VO_CHANNEL_BUTT];                                                  // VO输出通道
    int  mCropRgaChn2[DRM_VO_CHANNEL_BUTT][DRM_RGA_CHANNEL_BUTT];                           // RGA裁剪通道
    int  mZoomRgaChn2[DRM_VO_CHANNEL_BUTT][DRM_RGA_CHANNEL_BUTT];                           // RGA缩放通道

    bool mBindVmixVo[DRM_VO_CHANNEL_BUTT][DRM_VMIX_DEVICE_BUTT];                            // 是否已经绑定
    bool mBindRgaRga2[DRM_VO_CHANNEL_BUTT][DRM_RGA_CHANNEL_BUTT][DRM_RGA_CHANNEL_BUTT];     // 是否已经绑定
    bool mBindRgaVmix[DRM_VO_CHANNEL_BUTT][DRM_VMIX_CHANNEL_BUTT][DRM_RGA_CHANNEL_BUTT];    // 是否已经绑定
};

API_END_NAMESPACE(media)

#endif
