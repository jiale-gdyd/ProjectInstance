#ifndef MPI_AXMPI_MAIX_MAIX_IMAGE_HPP
#define MPI_AXMPI_MAIX_MAIX_IMAGE_HPP

#include "private.hpp"

API_BEGIN_NAMESPACE(media)

enum {
    MAIX_IMAGE_MODE_INVALID = 0,
    MAIX_IMAGE_MODE_BINARY,
    MAIX_IMAGE_MODE_GRAY,
    MAIX_IMAGE_MODE_RGB888,
    MAIX_IMAGE_MODE_RGB565,
    MAIX_IMAGE_MODE_RGBA8888,
    MAIX_IMAGE_MODE_YUV420SP_NV21,
    MAIX_IMAGE_MODE_YUV422_YUYV,
    MAIX_IMAGE_MODE_BGR888,
};

enum {
    MAIX_IMAGE_LAYOUT_HWC = 0,
    MAIX_IMAGE_LAYOUT_CHW,
};

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    } rgb888;
} maix_image_color_t;

#define MAIX_RGB(r, b, g)     (maix_image_color_t){ .rgb888 = {r, b, b} }

typedef struct maix_image {
    int  width;         // 图像宽度
    int  height;        // 图像高度
    int  mode;          // 图像模式(例如，MAIX_IMAGE_RGB888)
    int  layout;        // 图像布局(例如，MAIX_IMAGE_LAYOUT_HWC)
    void *data;         // 图像数据指针
    bool bDataAlloc;    // data是否是用户自分配内存，如果是，调用销毁时需要释放数据

    /**
     * 函数名称: convert
     * 功能描述: 图像转换
     * 输入参数: obj     --> struct maix_image图像对象
     *          mode    --> 图像模式(例如，MAIX_IMAGE_MODE_RGB888)
     * 输出参数: new_img --> 如果是maix_image_t指针的地址且为NULL，将自动创建新的图像对象和数据内存;
     *                      如果参数为NULL，返回负数; 如果参数是maix_image_t指针的地址且是maix_image_t对象的地址，则自动创建对象的数据内存
     * 返回说明: 成功返回0，其他则失败
     */
    int (*convert)(struct maix_image *obj, int mode, struct maix_image **new_img);

    /**
     * 函数名称: putText
     * 功能描述: 在图像上绘制UTF-8字符串
     * 输入参数: obj   --> struct maix_image图像对象
     *          str   --> 需要绘制的字符串信息
     *          x     --> 绘制起点坐标X
     *          y     --> 绘制起点坐标Y
     *          size  --> 绘制字体大小
     *          color --> 绘制字体颜色(MAIX_RGB(r, b, g))
     * 输出参数: bg    --> 图像背景色
     * 返回说明: 成功返回0，其他则失败
     */
    int (*putText)(struct maix_image *obj, const char *str, int x, int y, int size, maix_image_color_t color, maix_image_color_t *bg);

    /**
     * 函数名称: rectangle
     * 功能描述: 在图像上绘制矩形框
     * 输入参数: obj       --> struct maix_image图像对象
     *          x         --> 绘制矩形框的起点坐标X
     *          y         --> 绘制矩形框的起点坐标Y
     *          w         --> 绘制矩形框的宽度
     *          h         --> 绘制矩形框的高度
     *          color     --> 绘制矩形框的颜色(MAIX_RGB(r, b, g))
     *          fill      --> 是否填充矩形框
     *          thickness --> 线条大小
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int (*rectangle)(struct maix_image *obj, int x, int y, int w, int h, maix_image_color_t color, bool fill, int thickness);

    /**
     * 函数名称: resize
     * 功能描述: 图像缩放
     * 输入参数: obj     --> struct maix_image图像对象
     *          w       --> 缩放宽度
     *          h       --> 缩放高度
     * 输出参数: new_img --> 如果是maix_image_t指针的地址且为NULL，将自动创建新的图像对象和数据内存;
     *                      如果参数为NULL，返回负数; 如果参数是maix_image_t指针的地址且是maix_image_t对象的地址，则自动创建对象的数据内存
     * 返回说明: 成功返回0，其他则失败
     */
    int (*resize)(struct maix_image *obj, int w, int h, struct maix_image **new_img);

    /**
     * 函数名称: crop
     * 功能描述: 图像裁剪
     * 输入参数: obj     --> struct maix_image图像对象
     *          x       --> 裁剪图像的起始坐标X
     *          y       --> 裁剪图像的起始坐标Y
     *          w       --> 需要裁剪的宽度
     *          h       --> 需要裁剪的高度
     * 输出参数: new_img --> 如果是maix_image_t指针的地址且为NULL，将自动创建新的图像对象和数据内存;
     *                      如果参数为NULL，返回负数; 如果参数是maix_image_t指针的地址且是maix_image_t对象的地址，则自动创建对象的数据内存
     * 返回说明: 成功返回0，其他则失败
     */
    int (*crop)(struct maix_image *obj, int x, int y, int w, int h, struct maix_image **new_img);
} maix_image_t;

/**
 * 函数名称: maix_image_init
 * 功能描述: 图像模块初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int maix_image_init();

/**
 * 函数名称: maix_image_exit
 * 功能描述: 图像模块去初始化
 * 输入参数: 无
 * 输出参数: 无
 * 返回说明: 成功返回0，其他则失败
 */
int maix_image_exit();

/**
 * 函数名称: maix_image_create
 * 功能描述: 创建一帧图像
 * 输入参数: width      --> 图像宽度
 *          height     --> 图像高度
 *          mode       --> 图像模式(例如，MAIX_IMAGE_MODE_RGB888)
 *          layout     --> 图像布局(例如，MAIX_IMAGE_LAYOUT_HWC)
 *          data       --> 图像数据指针
 *          bDataAlloc --> data是否是用户自分配内存
 * 输出参数: 无
 * 返回说明: 成功返回maix_image_t对象，失败则凡虎NULL
 */
maix_image_t *maix_image_create(int width, int height, int mode, int layout, void *data, bool bDataAlloc);

/**
 * 函数名称: maix_image_release
 * 功能描述: 释放一帧图像
 * 输入参数: obj --> 图像对象
 * 输出参数: 无
 * 返回说明: 无
 */
void maix_image_release(maix_image_t **obj);

API_END_NAMESPACE(media)

#endif
