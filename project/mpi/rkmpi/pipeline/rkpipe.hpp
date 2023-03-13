#ifndef ROCKCHIP_MPI_PIPELINE_HPP
#define ROCKCHIP_MPI_PIPELINE_HPP

#include <string>
#include <cstdbool>
#include <functional>

#include <pthread.h>
#include <utils/export.h>
#include <media/drm_media_api.h>
#include <media/drm_media_buffer.h>

API_BEGIN_NAMESPACE(mpi)

/* pipeline输入枚举 */
enum {
    RKPIPE_INPUT_NONE = 0,                  // 输入类型未知
    RKPIPE_INPUT_VIN,                       // 摄像头为输入源
    RKPIPE_INPUT_USER,                      // 用户自定义输入
    RKPIPE_INPUT_VDEC_H264,                 // H264视频解码源
    RKPIPE_INPUT_VDEC_H265,                 // H265视频解码源
    RKPIPE_INPUT_VDEC_JPEG,                 // JPEG解码源
};

/* pipeline输出枚举 */
enum {
    RKPIPE_OUTPUT_NONE = 0,                 // 输出类型未知
    RKPIPE_OUTPUT_BUFF = 0x100,             // 输出缓存
    RKPIPE_OUTPUT_BUFF_RGB,                 // 输出缓存为RGB格式数据
    RKPIPE_OUTPUT_BUFF_BGR,                 // 输出缓存为BGR格式数据
    RKPIPE_OUTPUT_BUFF_NV12,                // 输出缓存为NV12格式数据
    RKPIPE_OUTPUT_BUFF_NV16,                // 输出缓存为NV16格式数据
    RKPIPE_OUTPUT_BUFF_NV21,                // 输出缓存为NV21格式数据

    RKPIPE_OUTPUT_VENC = 0x200,             // 输出VENC
    RKPIPE_OUTPUT_VENC_MJPG,                // 输出MJPG编码数据
    RKPIPE_OUTPUT_VENC_H264,                // 输出h264编码数据
    RKPIPE_OUTPUT_VENC_H265,                // 输出h265编码数据

    RKPIPE_OUTPUT_RTSP = 0x300,             // 输出RTSP
    RKPIPE_OUTPUT_RTSP_H264,                // 输出H264编码RTSP流
    RKPIPE_OUTPUT_RTSP_H265,                // 输出H265编码RTSP流

    RKPIPE_OUTPUT_VO   = 0x400,             // 视频输出
    RKPIPE_OUTPUT_VO_USER_SCREEN,           // 视频输出到用户显示屏
};

/* pipeline绑定关系 */
enum {
    BIND_VIN_RGA,
    BIND_VIN_VENC,
    BIND_RGA_RGA,
    BIND_VDEC_RGA,
};

/* pipeline vin配置 */
typedef struct {
    std::string device;                     // 摄像头设备节点
    bool        allocDMA;                   // 使用DMA分配缓存
    uint32_t    bufDepth;                   // 分配缓存深度(个数)
    uint32_t    bufPoolCnt;                 // 分配缓存池实际个数
    uint32_t    vinWidth;                   // 摄像头捕获像素宽度
    uint32_t    vinHeight;                  // 摄像头捕获像素高度
    uint32_t    pixFormat;                  // 摄像头捕获像素格式
    uint32_t    vinChannel;                 // 摄像头捕获通道号码
} rkvin_config_t;

/* pipeline rga配置 */
typedef struct {
    bool     swapWH;                        // 交换宽高
    uint32_t flipMode;                      // 翻转模式(0: 保持原样; 1: 水平翻转; 2: 垂直翻转; 3: 水平垂直同时翻转)
    uint32_t rotateAngle;                   // 旋转角度(0: 不旋转; 90: 顺时针旋转90度; 180: 顺时针旋转180度; 270: 顺时针旋转270度)
    uint32_t rgaChannel;                    // RGA通道号
    uint32_t bufDepth;                      // 分配缓存深度(个数)
    uint32_t bufPoolCnt;                    // 分配缓存池实际个数
    uint32_t rgaInPixFmt;                   // 输入到RGA的像素格式
    uint32_t rgaOutPixFmt;                  // RGA输出的像素格式
    uint32_t inputWidth;                    // 输入到RGA的像素宽度
    uint32_t inputHeight;                   // 输入到RGA的像素高度
    uint32_t inputXOffset;                  // 输入到RGA的X方向的偏移
    uint32_t inputYOffset;                  // 输入到RGA的Y方向的偏移
    uint32_t outputWidth;                   // RGA输出的像素宽度
    uint32_t outputHeight;                  // RGA输出的像素高度
    uint32_t outputXOffset;                 // RGA输出的X方向的偏移
    uint32_t outputYOffset;                 // RGA输出的Y方向的偏移
} rkrga_config_t;

/* pipeline venc配置 */
typedef struct {
    uint32_t    channel;                // VENC通道号
    uint32_t    bufDepth;                   // 分配缓存深度(个数)
    uint32_t    bufPoolCnt;                 // 分配缓存池实际个数
    uint32_t    vencWidth;                  // VENC编码像素宽度
    uint32_t    vencHeight;                 // VENC编码像素高度
    uint32_t    vencRcMode;                 // VENC编码RC模式
    uint32_t    vencCodecType;              // VENC编码格式
    uint32_t    vencFrameRate;              // VENC编码帧率
    uint32_t    vencProfile;                // VENC编码配置
    uint32_t    vencBitrate;                // VENC编码码率
    uint32_t    vencIFrameInternal;         // VENC编码I帧间隔
    uint32_t    vencRotateAngle;            // VENC编码旋转角度(0: 不旋转; 90: 顺时针旋转90度; 180: 顺时针旋转180度; 270: 顺时针旋转270度)

    uint16_t    rtspServerPort;             // RTSP服务端口
    std::string rtspServerEndPoint;         // RTSP服务URL

    pthread_t   threadId;                   // 内部使用
} rkvenc_config_t;

/* pipeline vdec配置 */
typedef struct {
    uint32_t    vdecChannel;                // VDEC通道号
    uint32_t    vecnCodecType;              // VDEC解码格式
    std::string vdecFile;                   // VDEC解码文件名
    bool        loopVdec;                   // 循环解码文件
    pthread_t   threadId;                   // 内部使用
} rkvdev_config_t;

/* pipeline vo配置 */
typedef struct {
    bool        swapWH;                     // 交换宽高
    uint32_t    voChannel;                  // 视频输出通道号
    uint32_t    voDispZPos;                 // 视频输出Z轴号(0/1)
    uint32_t    voDispLayer;                // 视频输出显示层(primary/overlay)
    uint32_t    voDispWidth;                // 视频输出像素宽度
    uint32_t    voDispHeight;               // 视频输出像素高度
    uint32_t    voDispXOffset;              // 视频输出X方向偏移
    uint32_t    voDispYOffset;              // 视频输出Y方向偏移
    uint32_t    voDispPixFormat;            // 视频输出像素格式
    std::string device;                     // 视频显示设备
} rkvo_config_t;

/* pipeline buffer配置 */
typedef struct {
    uint32_t id;                            // pipeline的ID
    uint32_t size;                          // 缓存数据字节总数
    uint64_t timestamp;                     // 缓存数据时间戳
    void     *viraddr;                      // 缓存数据所在虚拟地址
    void     *userdata;                     // 缓存用户自定义数据
} rkpipe_buffer_t;

using rkpipe_frame_t = std::function<void (rkpipe_buffer_t *buff, void *user_data)>;

/* pipeline配置 */
typedef struct {
    bool            enable;                 // 是否启用
    bool            quitThread;             // 如果有线程，则可以控制线程退出
    uint32_t        pipelineId;             // pipeline的ID号(不可重复)
    uint32_t        inputType;              // 输入类型(输入枚举值)
    uint32_t        outputType;             // 输出类型(输出枚举值)

    int             bindRelation;           // 绑定关系

    rkvo_config_t   vo;                     // vo配置
    rkvin_config_t  vin;                    // vin配置
    rkrga_config_t  rga;                    // rga配置
    rkvenc_config_t venc;                   // venc配置
    rkvdev_config_t vdec;                   // vdec配置

    rkpipe_frame_t  callback;               // 缓存帧回调函数
    void            *userdata;              // 用户自定义参数
} rkpipe_t;

/**
 * 函数名称: rkpipe_create
 * 功能描述: 创建pipeline
 * 输入参数: pipe --> pipeline信息
 * 输出参数: pipe --> pipeline信息
 * 返回说明: 成功返回0，其他则创建失败
 */
int rkpipe_create(rkpipe_t *pipe);

/**
 * 函数名称: rkpipe_release
 * 功能描述: 释放pipeline
 * 输入参数: pipe --> pipeline信息
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则创建失败
 */
int rkpipe_release(rkpipe_t *pipe);

API_END_NAMESPACE(mpi)

#endif
