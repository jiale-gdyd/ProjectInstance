#ifndef MPI_AXMPI_MAIX_MAIX_DISPLAY_HPP
#define MPI_AXMPI_MAIX_MAIX_DISPLAY_HPP

#include "maix_image.hpp"

API_BEGIN_NAMESPACE(media)

typedef struct maix_disp {
    int width;      // FB的宽度
    int height;     // FB的高度
    int bpp;        // FB的数据格式: RGB8, RGB565, RGB888, RGB8888

    /**
     * 函数名称: draw
     * 功能描述: 绘制图像到framebuffer ，根据width * height * bpp进行memcpy
     * 输入参数: disp --> disp对象
     *          buf  --> 自行根据系统情况准备图像内存地址
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int (*draw)(struct maix_disp *disp, unsigned char *buf);

    /**
     * 函数名称: draw_image
     * 功能描述: 绘制图像到framebuffer
     * 输入参数: disp --> disp对象
     *          img  --> 传入maix_image对象，内部会自行转换并显示
     * 输出参数: 无
     * 返回说明: 成功返回0，其他则失败
     */
    int (*draw_image)(struct maix_disp *disp, struct maix_image *img);

    void *reserved;
} maix_disp_t;

/**
 * @brief 创建disp对象
 *
 * @param [in] fbiopan: draw 是否启用 fbiopan
 * @return 创建的对象；NULL:出错
*/
/**
 * 函数名称: maix_display_create
 * 功能描述: 创建display对象
 * 输入参数: bFbiopan --> draw是否启用fbiopan
 * 输出参数: 无
 * 返回说明: 成功返回创建的对象，失败则返回NULL
 */
struct maix_disp *maix_display_create(bool bFbiopan);

/**
 * 函数名称: maix_display_release
 * 功能描述: 释放display对象
 * 输入参数: disp --> display对象
 * 输出参数: 无
 * 返回说明: 无
 */
void maix_display_release(struct maix_disp **disp);

API_END_NAMESPACE(media)


#endif
