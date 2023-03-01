#ifndef ROCKCHIP_MEDIA_RGA_HPP
#define ROCKCHIP_MEDIA_RGA_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <mutex>
#include <stdbool.h>
#include <utils/export.h>
#include <media/drm_media_api.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaRga {
public:
    MediaRga();
    ~MediaRga();

    // 删除了复制构造函数，删除了移动构造函数
    MediaRga(const MediaRga &other) = delete;
    MediaRga(MediaRga &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaRga &operator=(const MediaRga &other) = delete;
    MediaRga &operator=(MediaRga &&other) = delete;

    /**
     * 函数名称: rgaChnStart
     * 功能描述: 判断rga设备是否已经启动
     * 输入参数: rgaChn --> rga通道号
     * 输出参数: 无
     * 返回说明: 已经启动返回true，否则返回false
     */
    bool rgaChnStart(int rgaChn);

    /**
     * 函数名称: destroyRgaChn
     * 功能描述: 销毁rga通道
     * 输入参数: rgaChn --> rga通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int destroyRgaChn(int rgaChn);

    /**
     * 函数名称: createRgaChn
     * 功能描述: 创建并启动RGA通道(可以进行图像裁剪、格式转换、图像旋转等)
     * 输入参数: rgaChn      --> rga通道号
     *          bufPoolCnt  --> 开辟的缓存池数
     *          bufDepth    --> 设置缓存深度
     *          enInPixFmt  --> 输入图像像素格式
     *          enOutPixFmt --> 输出图像像素格式
     *          rotation    --> 旋转角度(仅支持0/90/180/270)
     *          inWidth     --> 输入图像宽度
     *          inHeight    --> 输入图像高度
     *          inXoffset   --> 输入图像裁剪X偏移
     *          inYoffset   --> 输入图像裁剪Y偏移
     *          outWidth    --> 输出图像宽度
     *          outHeight   --> 输出图像高度
     *          outXoffset  --> 输出图像裁剪X偏移(一般设置为0)
     *          outYoffset  --> 输出图像裁剪Y偏移(一般设置为0)
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createRgaChn(int rgaChn, int bufPoolCnt = 3, int bufDepth = 3,
        drm_image_type_e enInPixFmt = DRM_IMAGE_TYPE_NV12,
        drm_image_type_e enOutPixFmt = DRM_IMAGE_TYPE_RGB888,
        int rotation = 0,
        drm_rga_flip_e enFlip = RGA_FLIP_NULL,
        bool bWH2HW = false,
        size_t inWidth = 1280,
        size_t inHeight = 720,
        size_t inXoffset = 0,
        size_t inYoffset = 0,
        size_t outWidth = 1280,
        size_t outHeight = 720,
        size_t outXoffset = 0,
        size_t outYoffset = 0);

    /**
     * 函数名称: setRgaChnFrameRate
     * 功能描述: 设置rga通道码率
     * 输入参数: rgaChn   --> RGA通道号
     *          fpsInNum  --> 输入帧率分子/时间线分子
     *          fpsInDen  --> 输入帧率分母/时间线分母
     *          fpsOutNum --> 输出帧率分子/时间线分子
     *          fpsOutDen --> 输出帧率分母/时间线分母
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setRgaChnFrameRate(int rgaChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen);

    /**
     * 函数名称: getRgaChnAttr
     * 功能描述: 获取RGA通道属性
     * 输入参数: rgaChn     --> RGA通道号
     * 输出参数: pstRgaAttr --> 获取当前的RGA通道属性
     * 返回说明: 成功返回0,其他则失败
     */
    int getRgaChnAttr(int rgaChn, drm_rga_attr_t *pstRgaAttr);

    /**
     * 函数名称: setRgnChnAttr
     * 功能描述: 设置修改RGA通道属性
     * 输入参数: rgaChn      --> rga通道号
     *          rotation    --> 旋转角度(仅支持0/90/180/270)
     *          inWidth     --> 输入图像宽度
     *          inHeight    --> 输入图像高度
     *          inXoffset   --> 输入图像裁剪X偏移
     *          inYoffset   --> 输入图像裁剪Y偏移
     *          outWidth    --> 输出图像宽度
     *          outHeight   --> 输出图像高度
     *          outXoffset  --> 输出图像裁剪X偏移(一般设置为0)
     *          outYoffset  --> 输出图像裁剪Y偏移(一般设置为0)
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setRgnChnAttr(int rgaChn,
        int rotation = 0,
        drm_rga_flip_e enFlip = RGA_FLIP_NULL,
        bool bWH2HW = false,
        size_t inWidth = 1280,
        size_t inHeight = 720,
        size_t inXoffset = 0,
        size_t inYoffset = 0,
        size_t outWidth = 1280,
        size_t outHeight = 720,
        size_t outXoffset = 0,
        size_t outYoffset = 0);

    /**
     * 函数名称: convertFrame
     * 功能描述: 使用RGA进行格式转换以及裁剪等功能
     * 输入参数: modeIdChn      --> 使用的RGA通道
     *          origMediaFrame --> 原始media buffer帧
     *          s32MilliSec    --> 获取转换结果的等待超时时间
     *          waitConvUs     --> 等待转换完成时间
     * 输出参数: 无
     * 返回说明: 成功返回转换后的media buffer，失败则返回NULL
     */
    media_buffer_t convertFrame(int modeIdChn, media_buffer_t origMediaFrame, int s32MilliSec = -1, size_t waitConvUs = 50);

    /**
     * 函数名称: sendFrame
     * 功能描述: 将多媒体数据帧发送到指定的设备
     * 输入参数: modeIdChn    --> 设备通道号
     *          pstFrameInfo --> 多媒体数据帧指针
     *          modeId       --> 目标设备
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId = MOD_ID_RGA);

    /**
     * 函数名称: getFrame
     * 功能描述: 获取多媒体数据帧
     * 输入参数: modeIdChn   --> 设备通道号
     *          s32MilliSec --> 获取数据帧的超时时间
     *          modeId      --> 数据源设备
     * 输出参数: 无
     * 返回说明: 成功返回获取的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getFrame(int modeIdChn, int s32MilliSec = -1, int modeId = MOD_ID_RGA);

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
     * 函数名称: getFrameFd
     * 功能描述: 获取指向多媒体数据帧数据的文件描述符
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回指向多媒体数据帧数据的文件描述符，失败返回-1
     */
    int getFrameFd(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: releaseFrame
     * 功能描述: 释放多媒体数据帧
     * 输入参数: pstFrameInfo --> 多媒体数据帧指针
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int releaseFrame(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: setRgaRgnBitmap
     * 功能描述: 设置RGA通道区域位图
     * 输入参数: rgaChn    --> rgaCh通道号
     *          pstRgnInfo --> 区域信息
     *          pstBitmap  --> 位图信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setRgaRgnBitmap(int rgaChn, const drm_osd_region_info_t *pstRgnInfo, const drm_bitmap_t *pstBitmap);

    /**
     * 函数名称: setRgaRgnCover
     * 功能描述: 设置RGA通道区域覆盖
     * 输入参数: rgaChn       --> rgaCh通道号
     *          pstRgnInfo   --> 区域信息
     *          pstCoverInfo --> 覆盖信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setRgaRgnCover(int rgaChn, const drm_osd_region_info_t *pstRgnInfo, const drm_cover_info_t *pstCoverInfo);

private:
    bool       mRgaChnStarted[DRM_RGA_CHANNEL_BUTT];    // rga通道启动状态
    std::mutex mRgaAttrMutex[DRM_RGA_CHANNEL_BUTT];     // rga属性互斥锁
};

API_END_NAMESPACE(media)

#endif
