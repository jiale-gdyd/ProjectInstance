#ifndef ROCKCHIP_MEDIA_VMIX_HPP
#define ROCKCHIP_MEDIA_VMIX_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <stdbool.h>
#include <utils/export.h>
#include <media/drm_media_api.h>

typedef struct stVmixChnInfo {
    size_t         offsetX;        // 显示X偏移
    size_t         offsetY;        // 显示Y偏移
    size_t         areaWidth;      // 显示区域宽度
    size_t         areaHeight;     // 显示区域高度
    size_t         rotateAngle;    // 旋转角度(仅支持0, 90, 180, 270)(输出填写)
    drm_rga_flip_e enFlip;         // 镜像模式(0: 正常; 1: 水平镜像; 2: 垂直镜像; 3: 水平垂直镜像)(输出填写)
} vmix_chn_info_t;

typedef struct stVmixLineInfo {
    size_t   width;               // 当进行水平布局区域间隔上，为线宽，height为区域高度
    size_t   height;              // 当进行垂直布局区域间隔上，为线高，width为区域宽度
    size_t   offsetX;             // 显示X偏移
    size_t   offsetY;             // 显示Y偏移
    uint32_t lineColor;           // 画线颜色(RGBA)
} vmix_line_info_t;

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaVmix {
public:
    MediaVmix();
    ~MediaVmix();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVmix(const MediaVmix &other) = delete;
    MediaVmix(MediaVmix &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVmix &operator=(const MediaVmix &other) = delete;
    MediaVmix &operator=(MediaVmix &&other) = delete;

    /**
     * 函数名称: vmixChnStart
     * 功能描述: 判断视频混屏设备通道启动状态
     * 输入参数: vmixDev --> 视频混屏设备号
     *          vmixChn --> 视频混屏通道号
     * 输出参数: 无
     * 返回说明: 已经启动返回true，否则返回false
     */
    bool vmixChnStart(int vmixDev, int vmixChn);

    /**
     * 函数名称: createVmixChn
     * 功能描述: 创建视频混屏设备通道(布局是从左至右，从上往下)
     * 输入参数: vmixDev        --> 视频混屏设备号
     *          mixOutFps      --> 混合后的帧率
     *          mixOutWidth    --> 混合后的显示宽度(不是某一个布局的宽度，最终输出显示的宽度)
     *          mixOutHeight   --> 混合后的显示高度(不是某一个布局的高度，最终输出显示的高度)
     *          bEnableBufPool --> 是否使能缓存池
     *          bufPoolCnt     --> 开启的缓存池个数
     *          inImgType      --> 输入图像类型
     *          inInfo         --> 混合输入图像信息
     *          outInfo        --> 混合输出图像信息
     *          bDrawLine      --> 是否绘制区域分割线
     *          lineInfo       --> 区域分割线信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createVmixChn(int vmixDev, size_t mixOutFps, size_t mixOutWidth, size_t mixOutHeight, bool bEnableBufPool, size_t bufPoolCnt, drm_image_type_e inImgType,
        std::vector<vmix_chn_info_t> inInfo, std::vector<vmix_chn_info_t> outInfo, bool bDrawLine = false, std::vector<vmix_line_info_t> lineInfo = {});

    /**
     * 函数名称: destroyVmixChn
     * 功能描述: 销毁视频混屏设备通道
     * 输入参数: vmixDev --> 视频混屏设备号
     *          vmixChn --> 视频混屏通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int destroyVmixChn(int vmixDev, int vmixChn);

    /**
     * 函数名称: showVmixChn
     * 功能描述: 显示vmix通道
     * 输入参数: vmixDev --> 视频混屏设备号
     *          vmixChn --> 视频混屏通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int showVmixChn(int vmixDev, int vmixChn);

    /**
     * 函数名称: hideVmixChn
     * 功能描述: 隐藏vmix通道
     * 输入参数: vmixDev --> 视频混屏设备号
     *          vmixChn --> 视频混屏通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int hideVmixChn(int vmixDev, int vmixChn);

    /**
     * 函数名称: setVmixRgnBitmap
     * 功能描述: 设置VMIX通道区域位图
     * 输入参数: vmixDev    --> 视频混屏设备号
     *          vmixChn    --> 视频混屏通道号
     *          pstRgnInfo --> 区域信息
     *          pstBitmap  --> 位图信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setVmixRgnBitmap(int vmixDev, int vmixChn, const drm_osd_region_info_t *pstRgnInfo, const drm_bitmap_t *pstBitmap);

    /**
     * 函数名称: setVmixRgnCover
     * 功能描述: 设置VMIX通道区域覆盖
     * 输入参数: vmixDev    --> 视频混屏设备号
     *          vmixChn    --> 视频混屏通道号
     *          pstRgnInfo   --> 区域信息
     *          pstCoverInfo --> 覆盖信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setVmixRgnCover(int vmixDev, int vmixChn, const drm_osd_region_info_t *pstRgnInfo, const drm_cover_info_t *pstCoverInfo);

    /**
     * 函数名称: sendFrame
     * 功能描述: 将多媒体数据帧发送到指定的设备
     * 输入参数: modeIdChn    --> 设备通道号
     *          pstFrameInfo --> 多媒体数据帧指针
     *          modeId       --> 目标设备
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId = MOD_ID_VMIX);

    /**
     * 函数名称: getFrame
     * 功能描述: 获取多媒体数据帧
     * 输入参数: modeIdChn   --> 设备通道号
     *          s32MilliSec --> 获取数据帧的超时时间
     *          modeId      --> 数据源设备
     * 输出参数: 无
     * 返回说明: 成功返回获取的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getFrame(int modeIdChn, int s32MilliSec = -1, int modeId = MOD_ID_VMIX);

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
    bool mVmixChnStarted[DRM_VMIX_DEVICE_BUTT][DRM_VMIX_CHANNEL_BUTT]; // 视频混屏设备通道启动状态
};

API_END_NAMESPACE(media)

#endif
