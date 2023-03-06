#ifndef MPI_AXMPI_MAIX_MAIX_DISPLAY_HPP
#define MPI_AXMPI_MAIX_MAIX_DISPLAY_HPP

#include "maix_image.hpp"

API_BEGIN_NAMESPACE(media)

typedef struct maix_camera {
    int          width;         // 摄像头采集图像的宽度
    int          height;        // 摄像头采集图像的高度
    unsigned int fram_size;     // 一帧图片占用内存大小

    /**
     * 函数名称: start_capture
     * 功能描述: 开始获取图像
     * 输入参数: cam --> camera对象
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int (*start_capture)(struct maix_camera *cam);

    /**
     * 函数名称: capture
     * 功能描述: 获取一帧图像
     * 输入参数: cam --> camera对象
     *          buf --> 图片缓存内存地址，buf至少要有`fram_size`这么大的空间
     * 输出参数: buf --> 图片缓存内存地址，buf至少要有`fram_size`这么大的空间
     * 返回说明: 成功返回0，其他则失败
     */
    int (*capture)(struct maix_camera *cam, unsigned char *buf);

    /**
     * 函数名称: capture_image
     * 功能描述: 获取一帧图像
     * 输入参数: cam --> camera对象
     * 输出参数: img --> img对象指针(由内部提供)
     * 返回说明: 成功返回0，其他则失败
     */
    int (*capture_image)(struct maix_camera *cam, struct maix_image **img);

    void *reserved;
} maix_camera_t;

/**
 * 函数名称: maix_camera_init
 * 功能描述: 摄像头环境初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 无
 */
void maix_camera_init();

/**
 * 函数名称: maix_camera_exit
 * 功能描述: 摄像头环境去初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 无
 */
void maix_camera_exit();

/**
 * 函数名称: maix_camera_create
 * 功能描述: 创建camera对象
 * 输入参数: camId  -> 摄像头ID(默认填0)
 *          width  --> 设置采集图像的宽度
 *          height --> 设置采集图像的高度
 *          mirror --> 设置摄像头水平翻转
 *          flip   --> 设置摄像头垂直翻转
 * 输出参数:
 * 返回说明:
 */
maix_camera_t *maix_camera_create(int camId, int width, int height, bool mirror, bool flip);

/**
 * 函数名称:
 * 功能描述:
 * 输入参数:
 * 输出参数:
 * 返回说明:
 */
void maix_camera_release(maix_camera_t **camera);

API_END_NAMESPACE(media)

#endif
