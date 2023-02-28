#ifndef RV11XX_FUNCIMPL_EMS_DEMO_PARAM_HPP
#define RV11XX_FUNCIMPL_EMS_DEMO_PARAM_HPP

#include <string>

typedef struct _stVideoEncoderParam {
    size_t      videoFPS;                                // 视频帧率
    size_t      encodeProfile;                           // 编码配置
    size_t      encodeBitRate;                           // 编码码率
    size_t      encodeOneVideoMinute;                    // 编码一个视频的分钟数
    size_t      encodeIFrameInterval;                    // 视频编码I帧间隔
    size_t      saveDevLimitCapGB;                       // 存储设备最小容量(GB)
    std::string saveMountPoint;                          // 存储介质挂载点
    std::string saveDeviceNode;                          // 存储介质设备节点
} venc_param_t;

typedef struct _stAppConfigParam {
    bool         enableThisChannel;                      // 使能该通道
    bool         enableRGAFlushCache;                    // 使能RGA冲刷CPU缓存
    bool         disableVideoEncoderSave;                // 禁用视频编码存储
    venc_param_t videoEncoderParam;                      // 视频编码参数(disableVideoEncoderSave为false时有用)
} ems_config_t;

#endif
