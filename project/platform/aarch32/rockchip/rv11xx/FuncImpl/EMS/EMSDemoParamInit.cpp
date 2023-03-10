#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

ems_config_t EMSDemoImpl::mEMSConfig = {
    .enableThisChannel       = true,                        // 使能该通道
    .enableRGAFlushCache     = true,                        // 使能RGA冲刷CPU缓存
    .disableVideoEncoderSave = true,                        // 禁用视频编码存储
    .disableVideoRtspServer  = false,                       // 禁用视频编码推送RTSP服务器
    .videoEncoderParam = {
        .videoFPS             = 30,                         // 视频帧率
        .encodeProfile        = 66,                         // 编码配置
        .encodeBitRate        = 8000000,                    // 编码码率
        .encodeOneVideoMinute = 2,                          // 编码一个视频的分钟数
        .encodeIFrameInterval = 25,                         // 视频编码I帧间隔
        .saveDevLimitCapGB    = 2,                          // 存储设备最小容量(GB)
        .saveMountPoint       = "/sdcard/",                 // 存储介质挂载点
        .saveDeviceNode       = "/dev/mmcblk2p1"            // 存储介质设备节点
    },
    .videoRtspServerParam = {
        .videoFPS             = 30,                         // 视频帧率
        .encodeProfile        = 66,                         // 编码配置
        .encodeBitRate        = 8000000,                    // 编码码率
        .encodeIFrameInterval = 25,                         // 视频编码I帧间隔
        .serverPort           = 554,                        // RTSP服务器端口号
        .streamPath           = "live/main_stream"          // RTSP服务器流地址
    },
};

API_END_NAMESPACE(EMS)
