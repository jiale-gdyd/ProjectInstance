#ifndef ROCKCHIP_MEDIA_SYS_HPP
#define ROCKCHIP_MEDIA_SYS_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <utils/export.h>
#include <media/drm_media_api.h>

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaSys {
public:
    MediaSys();
    ~MediaSys();

    // 删除了复制构造函数，删除了移动构造函数
    MediaSys(const MediaSys &other) = delete;
    MediaSys(MediaSys &&other) = delete;

    // 已删除副本分配，已删除移动分配
    MediaSys &operator=(const MediaSys &other) = delete;
    MediaSys &operator=(MediaSys &&other) = delete;

    int init();
    void deinit();

    int bind(drm_chn_t src, drm_chn_t dst);
    int unbind(drm_chn_t src, drm_chn_t dst);

    int bindViVo(drm_chn_t vi, drm_chn_t vo);
    int bindViRga(drm_chn_t vi, drm_chn_t rga);
    int bindRgaVo(drm_chn_t rga, drm_chn_t vo);
    int bindViVenc(drm_chn_t vi, drm_chn_t venc);
    int bindViVmix(drm_chn_t vi, drm_chn_t vmix);
    int bindVdecVo(drm_chn_t vdec, drm_chn_t vo);
    int bindVmixVo(drm_chn_t vmix, drm_chn_t vo);
    int bindRgaVenc(drm_chn_t rga, drm_chn_t venc);
    int bindRgaVmix(drm_chn_t rga, drm_chn_t vmix);
    int bindVdecRga(drm_chn_t vdec, drm_chn_t rga);
    int bindVdecVenc(drm_chn_t vdec, drm_chn_t venc);
    int bindRgaRga(drm_chn_t rgaSrc, drm_chn_t rgaDst);

    int unbindViVo(drm_chn_t vi, drm_chn_t vo);
    int unbindViRga(drm_chn_t vi, drm_chn_t rga);
    int unbindRgaVo(drm_chn_t rga, drm_chn_t vo);
    int unbindViVenc(drm_chn_t vi, drm_chn_t venc);
    int unbindViVmix(drm_chn_t vi, drm_chn_t vmix);
    int unbindVdecVo(drm_chn_t vdec, drm_chn_t vo);
    int unbindVmixVo(drm_chn_t vmix, drm_chn_t vo);
    int unbindRgaVenc(drm_chn_t rga, drm_chn_t vmix);
    int unbindRgaVmix(drm_chn_t rga, drm_chn_t venc);
    int unbindVdecRga(drm_chn_t vdec, drm_chn_t rga);
    int unbindVdecVenc(drm_chn_t vdec, drm_chn_t venc);
    int unbindRgaRga(drm_chn_t rgaSrc, drm_chn_t rgaDst);

    int bindViVo(int viDevId, int viChn, int voDevId, int voChn);
    int bindViRga(int viDevId, int viChn, int rgaDevId, int rgaChn);
    int bindRgaVo(int rgaDevId, int rgaChn, int voDevId, int voChn);
    int bindViVenc(int viDevId, int viChn, int vencDevId, int vencChn);
    int bindViVmix(int viDevId, int viChn, int vmixDevId, int vmixChn);
    int bindVdecVo(int vdecDevId, int vdecChn, int voDevId, int voChn);
    int bindVmixVo(int vmixDevId, int vmixChn, int voDevId, int voChn);
    int bindRgaVenc(int rgaDevId, int rgaChn, int vencDevId, int vencChn);
    int bindRgaVmix(int rgaDevId, int rgaChn, int vmixDevId, int vmixChn);
    int bindVdecRga(int vdecDevId, int vdecChn, int rgaDevId, int rgaChn);
    int bindVdecVenc(int vdecDevId, int vdecChn, int vencDevId, int vencChn);
    int bindRgaRga(int rgaSrcDevId, int rgaSrcChn, int rgaDstDevId, int rgaDstChn);

    int unbindViVo(int viDevId, int viChn, int voDevId, int voChn);
    int unbindViRga(int viDevId, int viChn, int rgaDevId, int rgaChn);
    int unbindRgaVo(int rgaDevId, int rgaChn, int voDevId, int voChn);
    int unbindViVenc(int viDevId, int viChn, int vencDevId, int vencChn);
    int unbindViVmix(int viDevId, int viChn, int vmixDevId, int vmixChn);
    int unbindVdecVo(int vdecDevId, int vdecChn, int voDevId, int voChn);
    int unbindVmixVo(int vmixDevId, int vmixChn, int voDevId, int voChn);
    int unbindRgaVenc(int rgaDevId, int rgaChn, int vencDevId, int vencChn);
    int unbindRgaVmix(int rgaDevId, int rgaChn, int vmixDevId, int vmixChn);
    int unbindVdecRga(int vdecDevId, int vdecChn, int rgaDevId, int rgaChn);
    int unbindVdecVenc(int vdecDevId, int vdecChn, int vencDevId, int vencChn);
    int unbindRgaRga(int rgaSrcDevId, int rgaSrcChn, int rgaDstDevId, int rgaDstChn);

    /* 默认devid或pipe为0的绑定与解绑处理 */
    int bindViVo(int viChn, int voChn);
    int bindViRga(int viChn, int rgaChn);
    int bindRgaVo(int rgaChn, int voChn);
    int bindViVenc(int viChn, int vencChn);
    int bindViVmix(int viChn, int vmixChn);
    int bindVdecVo(int vdecChn, int voChn);
    int bindVmixVo(int vmixChn, int voChn);
    int bindRgaVenc(int rgaChn, int vencChn);
    int bindRgaVmix(int rgaChn, int vmixChn);
    int bindVdecRga(int vdecChn, int rgaChn);
    int bindVdecVenc(int vdecChn, int vencChn);
    int bindRgaRga(int rgaSrcChn, int rgaDstChn);

    int unbindViVo(int viChn, int voChn);
    int unbindViRga(int viChn, int rgaChn);
    int unbindRgaVo(int rgaChn, int voChn);
    int unbindViVenc(int viChn, int vencChn);
    int unbindViVmix(int viChn, int vmixChn);
    int unbindVdecVo(int vdecChn, int voChn);
    int unbindVmixVo(int vmixChn, int voChn);
    int unbindRgaVenc(int rgaChn, int vencChn);
    int unbindVdecRga(int vdecChn, int rgaChn);
    int unbindRgaVmix(int rgaChn, int vmixChn);
    int unbindVdecVenc(int vdecChn, int vencChn);
    int unbindRgaRga(int rgaSrcChn, int rgaDstChn);

    /**
     * 函数名称: setChnFrameRate
     * 功能描述: 设置通道码率
     * 输入参数: modeId   --> 目标设备
     *          modeIdChn --> vdec通道号
     *          fpsInNum  --> 输入帧率分子/时间线分子
     *          fpsInDen  --> 输入帧率分母/时间线分母
     *          fpsOutNum --> 输出帧率分子/时间线分子
     *          fpsOutDen --> 输出帧率分母/时间线分母
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int setChnFrameRate(int modeId, int modeIdChn, int fpsInNum, int fpsInDen, int fpsOutNum, int fpsOutDen);

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
     * 函数名称: getFrameFd
     * 功能描述: 获取多媒体帧的DMA文件描述符
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回DMA文件描述符，失败返回<0
     */
    int getFrameFd(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: getDmaDeviceFd
     * 功能描述: 获取多媒体帧的DMA设备文件描述符
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回DMA设备文件描述符，失败返回<0
     */
    int getDmaDeviceFd(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: getDmaBufferHandle
     * 功能描述: 获取多媒体帧的DMA缓存句柄
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回DMA缓存句柄，失败返回<0
     */
    int getDmaBufferHandle(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: getFrameSize
     * 功能描述: 获取多媒体数据帧的大小
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 返回多媒体数据帧的大小
     */
    size_t getFrameSize(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: setFrameSize
     * 功能描述: 设置多媒体数据帧大小
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     *          size         --> 设置大小
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int setFrameSize(media_buffer_t pstFrameInfo, size_t size);

    /**
     * 函数名称: getFrameModeId
     * 功能描述: 获取多媒体数据帧的模式ID
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 返回多媒体数据帧的模式ID
     */
    mod_id_e getFrameModeId(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: getFrameChnId
     * 功能描述: 获取多媒体数据帧的通道ID
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 返回多媒体数据帧的通道ID
     */
    int getFrameChnId(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: getFrameTimestamp
     * 功能描述: 获取多媒体数据帧的时间戳
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 返回多媒体数据帧的时间戳
     */
    uint64_t getFrameTimestamp(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: setFrameTimestamp
     * 功能描述: 设置多媒体数据帧时间戳
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     *          timestamp    --> 时间戳
     * 输出参数: 无
     * 返回说明: 成功返回0，其他失败
     */
    int setFrameTimestamp(media_buffer_t pstFrameInfo, uint64_t timestamp);

    /**
     * 函数名称: releaseFrame
     * 功能描述: 释放多媒体数据帧
     * 输入参数: pstFrameInfo --> 多媒体数据帧指针
     * 输出参数: 无
     * 返回说明: 成功返回0,其他则失败
     */
    int releaseFrame(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: createFrame
     * 功能描述: 创建多媒体数据帧
     * 输入参数: size      --> 创建的数据帧大小
     *          bHardware --> 是否采用硬件方式分配内存
     *          flags     --> 分配内存标志
     * 输出参数: 无
     * 返回说明: 成功返回多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t createFrame(size_t size, bool bHardware = true, unsigned char flags = 0);

    /**
     * 函数名称: createImageFrame
     * 功能描述: 创建图片形式的多媒体数据帧
     * 输入参数: pstImageInfo --> 图像多媒体数据帧结构
     *          bHardware    --> 是否采用硬件方式分配内存
     *          flags        --> 分配内存标志
     * 输出参数: 无
     * 返回说明: 成功返回图片形式的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t createImageFrame(mb_image_info_t *pstImageInfo, bool bHardware = true, unsigned char flags = 0);

    /**
     * 函数名称: convertToImageFrame
     * 功能描述: 将多媒体数据帧转为图片形式的多媒体数据帧
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     *          pstImageInfo --> 图像多媒体数据帧结构
     * 输出参数: 无
     * 返回说明: 成功返回图片形式的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t convertToImageFrame(media_buffer_t pstFrameInfo, mb_image_info_t *pstImageInfo);

    /**
     * 函数名称: getFrameImageInfo
     * 功能描述: 获取多媒体数据帧中的图像信息结构
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     *           pstImageInfo --> 多媒体数据帧中的图像信息结构
     * 输出参数: 无
     * 返回说明: 成功返回0，其他失败
     */
    int getFrameImageInfo(media_buffer_t pstFrameInfo, mb_image_info_t *pstImageInfo);

    /**
     * 函数名称: copyFrame
     * 功能描述: 拷贝多媒体数据帧
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     *          bZeroCopy    --> 是否采用零拷贝
     * 输出参数: 无
     * 返回说明: 成功返回拷贝的多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t copyFrame(media_buffer_t pstFrameInfo, bool bZeroCopy = true);

    /**
     * 函数名称: getFrameFlags
     * 功能描述: 获取多媒体数据帧标志信息
     * 输入参数: pstFrameInfo --> 多媒体数据帧
     * 输出参数: 无
     * 返回说明: 成功返回多媒体数据帧标志信息，<0失败
     */
    int getFrameFlags(media_buffer_t pstFrameInfo);

    /**
     * 函数名称: beginCPUAccess
     * 功能描述: 开始CPU访问多媒体数据帧
     * 输入参数: mediaFrame --> 原始多媒体数据帧
     *          readOnly   --> 是否只读
     * 输出参数: 无
     * 返回说明: 成功返回0，其他失败
     */
    int beginCPUAccess(media_buffer_t mediaFrame, bool readOnly);

    /**
     * 函数名称: endCPUAccess
     * 功能描述: 结束CPU访问多媒体数据帧
     * 输入参数: mediaFrame --> 原始多媒体数据帧
     *          readOnly   --> 是否只读
     * 输出参数: 无
     * 返回说明: 成功返回0，其他失败
     */
    int endCPUAccess(media_buffer_t mediaFrame, bool readOnly);

    /**
     * 函数名称: createFramePools
     * 功能描述: 创建多媒体数据帧池
     * 输入参数: pstPoolParam --> 多媒体数据帧池参数信息
     * 输出参数: 无
     * 返回说明: 成功返回多媒体数据帧池指针，失败返回NULL
     */
    media_buffer_pool_t createFramePool(mb_pool_param_t *pstPoolParam);

    /**
     * 函数名称: releaseFramePool
     * 功能描述: 释放多媒体数据帧池
     * 输入参数: pstFramePool --> 多媒体数据帧池
     * 输出参数: 无
     * 返回说明: 成功返回0，其他失败
     */
    int releaseFramePool(media_buffer_pool_t pstFramePool);

    /**
     * 函数名称: getPoolFrame
     * 功能描述: 从多媒体数据帧池中获取多媒体数据帧
     * 输入参数: pstFramePool --> 多媒体数据帧池
     * 输出参数: 无
     * 返回说明: 成功返回多媒体数据帧指针，失败返回NULL
     */
    media_buffer_t getPoolFrame(media_buffer_pool_t pstFramePool, bool bBlock);

    /**
     * 函数名称: registerMediaOutCb
     * 功能描述: 注册多媒体帧输出回调函数
     * 输入参数: stChn    --> 多媒体通道信息
     *           callback --> 回调处理函数
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int registerMediaOutCb(drm_chn_t stChn, OutCallbackFunction callback);

    /**
     * 函数名称: registerMediaOutCb
     * 功能描述: 注册多媒体帧输出回调函数
     * 输入参数: stChn     --> 多媒体通道信息
     *           callback  --> 回调处理函数
     *           user_data --> 用户数据
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int registerMediaOutCbEx(drm_chn_t stChn, OutCallbackFunctionEx callback, void *user_data);

    /**
     * 函数名称: setupEnableRGAFlushCache
     * 功能描述: 设置RGA能锁定CPU进行缓存冲刷(可能会造成一些耗时)
     * 输入参数: bEnable --> 是否使能
     * 输出参数: 无
     * 返回说明: 返回设置状态
     */
    bool setupEnableRGAFlushCache(bool bEnable);
};

API_END_NAMESPACE(media)

#endif
