#ifndef ROCKCHIP_MEDIA_VIN_HPP
#define ROCKCHIP_MEDIA_VIN_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <utils/export.h>
#include <media/drm_media_api.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaVin {
public:
    MediaVin();
    ~MediaVin();

    // 删除了复制构造函数，删除了移动构造函数
    MediaVin(const MediaVin &other) = delete;
    MediaVin(MediaVin &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaVin &operator=(const MediaVin &other) = delete;
    MediaVin &operator=(MediaVin &&other) = delete;

    /**
     * 函数名称: viChnStarted
     * 功能描述: 判断vi设备是否已经启动
     * 输入参数: vinChn --> vi通道号
     * 输出参数: 无
     * 返回说明: 已经启动返回true，否则返回false
     */
    bool viChnStarted(int vinChn);

    /**
     * 函数名称: destroyViChn
     * 功能描述: 销毁vi通道
     * 输入参数: vinChn --> vi通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int destroyViChn(int vinChn);

    /**
     * 函数名称: createViChn
     * 功能描述: 创建并启动vi通道
     * 输入参数: vinChn        --> vi通道号
     *          bufPoolCnt    --> 开辟的缓存池数
     *          bufDepth      --> 设置缓存深度
     *          devNode       --> vi设备节点
     *          capWidth      --> 捕获的vi数据流像素宽度
     *          capHeight     --> 捕获的vi数据流像素高度
     *          enPixFmt      --> 捕获的vi数据流像素格式
     *          bAllocWithDMA --> 是否采用DMA方式分配内存
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int createViChn(int vinChn, int bufPoolCnt = 3, int bufDepth = 3, const char *devNode = "/dev/video0",
        size_t capWidth = 1280, size_t capHeight = 720, drm_image_type_e enPixFmt = DRM_IMAGE_TYPE_NV12, bool bAllocWithDMA = false);

    /**
     * 函数名称: setViChnUserPic
     * 功能描述: 使用用户图片设置到VI的指定通道
     * 输入参数: vinChn   --> vi通道号
     *          width    --> 图像宽度
     *          height   --> 图像高度
     *          picData  --> 图像数据
     *          enPixFmt --> 图像像素格式
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setViChnUserPic(int vinChn, size_t width, size_t height, void *picData, drm_image_type_e enPixFmt = DRM_IMAGE_TYPE_NV16);

    /**
     * 函数名称: enableViChnUserPic
     * 功能描述: 使能VI通道用户设置的图片
     * 输入参数: vinChn --> vi通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int enableViChnUserPic(int vinChn);

    /**
     * 函数名称: disableViChnUserPic
     * 功能描述: 禁止VI通道用户设置的图片
     * 输入参数: vinChn --> vi通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int disableViChnUserPic(int vinChn);

    /**
     * 函数名称: setViChnFrameRate
     * 功能描述: 设置vi通道码率
     * 输入参数: vinChn    --> vi通道号
     *          fpsInNum  --> 输入帧率分子/时间线分子
     *          fpsInDen  --> 输入帧率分母/时间线分母
     *          fpsOutNum --> 输出帧率分子/时间线分子
     *          fpsOutDen --> 输出帧率分母/时间线分母
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setViChnFrameRate(int vinChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen);

    /**
     * 函数名称: startViChnStream
     * 功能描述: 启动VI通道数据流
     * 输入参数: vinChn --> vi通道号
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int startViChnStream(int vinChn);

    /**
     * 函数名称: getViChnCaptureStatus
     * 功能描述: 获取指定VI通道捕获数据源状态(用于检测摄像头是否丢失)
     * 输入参数: vinChn --> vi通道号
     * 输出参数: 无
     * 返回说明: 正常状态返回0，其他则失败
     */
    int getViChnCaptureStatus(int vinChn);

    /**
     * 函数名称: getViChnStatus
     * 功能描述: 获取指定VI通道数据源状态(用于检测摄像头是否丢失)
     * 输入参数: vinChn   --> vi通道号
     *          timeoutS --> 检测超时时间(秒)，大于该时间则认为输入源存在问题
     * 输出参数: 无
     * 返回说明: 如果输入源超时，则返回超时时间
     */
    int getViChnStatus(int vinChn, size_t timeoutS = 3);

    /**
     * 函数名称: setViRgnBitmap
     * 功能描述: 设置VI通道区域位图
     * 输入参数: vinChn     --> vi通道号
     *          pstRgnInfo --> 区域信息
     *          pstBitmap  --> 位图信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setViRgnBitmap(int vinChn, const drm_osd_region_info_t *pstRgnInfo, const drm_bitmap_t *pstBitmap);

    /**
     * 函数名称: setViRgnCover
     * 功能描述: 设置VI通道区域覆盖
     * 输入参数: vinChn       --> vi通道号
     *          pstRgnInfo   --> 区域信息
     *          pstCoverInfo --> 覆盖信息
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setViRgnCover(int vinChn, const drm_osd_region_info_t *pstRgnInfo, const drm_cover_info_t *pstCoverInfo);

    /**
     * 函数名称: sendFrame
     * 功能描述: 将多媒体数据帧发送到指定的设备
     * 输入参数: modeIdChn    --> 设备通道号
     *          pstFrameInfo --> 多媒体数据帧指针
     *          modeId       --> 目标设备
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId = MOD_ID_VI);

    /**
     * 函数名称: getFrame
     * 功能描述: 获取多媒体数据帧
     * 输入参数: modeIdChn   --> 设备通道号
     *          s32MilliSec --> 获取数据帧的超时时间
     *          modeId      --> 数据源设备
     * 输出参数: 无
     * 返回说明: 成功返回获取的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getFrame(int vinChn, int s32MilliSec = -1, int modeId = MOD_ID_VI);

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
    bool mViChnStarted[DRM_VI_CHANNEL_BUTT];  // 视频输入通道启动状态
};

API_END_NAMESPACE(media)

#endif
