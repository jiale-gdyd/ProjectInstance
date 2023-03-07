#ifndef MPI_AXMPI_PIPELINE_AXPIPE_HPP
#define MPI_AXMPI_PIPELINE_AXPIPE_HPP

#include <string>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <functional>

#include <utils/export.h>
#include <mpi/axmpi/axapi.h>

API_BEGIN_NAMESPACE(media)

/* pipeline输入 */
enum {
    AXPIPE_INPUT_NONE = 0,
    AXPIPE_INPUT_VIN,
    AXPIPE_INPUT_USER,
    AXPIPE_INPUT_VDEC_H264,
    AXPIPE_INPUT_VDEC_JPEG,
};

/* pipeline输出 */
enum {
    AXPIPE_OUTPUT_NONE = 0,
    AXPIPE_OUTPUT_BUFF = 0x10,
    AXPIPE_OUTPUT_BUFF_RGB,
    AXPIPE_OUTPUT_BUFF_BGR,
    AXPIPE_OUTPUT_BUFF_NV12,
    AXPIPE_OUTPUT_BUFF_NV21,

    AXPIPE_OUTPUT_VENC = 0x20,
    AXPIPE_OUTPUT_VENC_MJPG,
    AXPIPE_OUTPUT_VENC_H264,
    AXPIPE_OUTPUT_VENC_H265,

    AXPIPE_OUTPUT_RTSP = 0x30,
    AXPIPE_OUTPUT_RTSP_H264,
    AXPIPE_OUTPUT_RTSP_H265,

    AXPIPE_OUTPUT_VO   = 0x40,
    AXPIPE_OUTPUT_VO_USER_SCREEN,
    AXPIPE_OUTPUT_VO_SIPEED_SCREEN,
};

/* pipeline ivps配置 */
typedef struct {
    int       ivpsGroup;                    // IVPS组
    int       osdRegions;                   // rgn的个数，设为0则表示不进行osd，每一个rgn可以绘制32个目标
    int       osdRgnHandle[OSD_RGN_BUTT];   // rgn的句柄
    int       ivpsFramerate;                // IPVS输出帧率
    int       ivpsRotate;                   // 旋转角度(0, 90, 180, 270)
    int       ivpsWidth;                    // 帧宽度(必须为偶数)
    int       ivpsHeight;                   // 帧高度(必须为偶数)
    bool      bIvpsMirror;                  // 是否镜像
    bool      bIvpsFlip;                    // 是否翻转
    bool      bLetterBbox;                  // 是否填充(一般用于ai检测)
    int       fifoCount;                    // 0表示不输出，1-4表示队列的个数，大于0则可以在调用回调输出图像
    pthread_t tid;                          // 内部使用
} axpipe_ivps_config_t;

/* pipeline vdec配置 */
typedef struct {
    int       vdecGroup;                    // 视频解码组
    int       poolId;                       // 内部使用
    pthread_t tid;                          // 内部使用
} axpipe_vdec_config_t;

/* pipeline venc配置 */
typedef struct {
    int         vencChannel;                // 视频编码通道
    uint16_t    rtspPort;                   // rtsp服务器端口号
    std::string endPoint;                   // rtsp节点名称(URL)
    pthread_t   tid;                        // 内部使用
} axpipe_venc_config_t;

/* pipeline user vo配置 */
typedef struct {
    int         dispChannel;                // 显示通道
    std::string dispDevStr;                 // 显示设备配置(例如, "dsi0@480x854@60")
} axpipe_vo_config_t;

typedef struct {
    int      pipeId;                        // pipeline的ID号
    int      outType;                       // 输出类型
    int      width;                         // 宽度
    int      height;                        // 高度
    int      size;                          // 字节数
    int      stride;                        // 跨度
    int      dataType;                      // 数据类型
    void     *virAddr;                      // 虚拟地址
    uint64_t phyAddr;                       // 物理地址
    void     *userData;                     // 用户数据
} axpipe_buffer_t;

// typedef void (*axpipe_frame_t)(axpipe_buffer_t *buff, void *user_data);
using axpipe_frame_t = std::function<void (axpipe_buffer_t *buff, void *user_data)>;

typedef struct {
    bool                 bEnable;           // 是否启用
    int                  pipeId;            // pipeline的ID号，不可重复
    int                  inType;            // 输入类型
    int                  outType;           // 输出类型
    volatile bool        bThreadQuit;       // 如果又线程，则可以控制线程退出
    int                  vinPipe;           // 输入管道
    int                  vinChn;            // 输入通道
    axpipe_vo_config_t   voConfig;          // 显示设备配置
    axpipe_vdec_config_t vdecConfig;        // 视频解码配置
    axpipe_venc_config_t vencConfig;        // 视频编码配置
    axpipe_ivps_config_t ivpsConfig;        // IVPS配置
    axpipe_frame_t       frameCallback;     // 帧回调函数
    void                 *userData;         // 用户数据
} axpipe_t;

/**
 * 函数名称: axpipe_create
 * 功能描述: 创建pipeline
 * 输入参数: pipe --> pipeline信息
 * 输出参数: pipe --> pipeline信息
 * 返回说明: 成功返回0，其他则创建失败
 */
int axpipe_create(axpipe_t *pipe);

/**
 * 函数名称: axpipe_release
 * 功能描述: 释放pipeline
 * 输入参数: pipe --> pipeline信息
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则创建失败
 */
int axpipe_release(axpipe_t *pipe);

/**
 * 函数名称: axpipe_user_input
 * 功能描述: 这里认为pipe指针含有pipes个axpipe_t结构体，并且每一个axpipe_t的输入类型inType是一样的，此函数会将同一张图片发送到所有pipes条链路中
 * 输入参数: pipe  --> pipeline信息
 *          pipes --> axpipe_t结构体数
 *          buff  --> pipeline数据信息
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则创建失败
 */
int axpipe_user_input(axpipe_t *pipe, int pipes, axpipe_buffer_t *buff);

API_END_NAMESPACE(media)

#endif
