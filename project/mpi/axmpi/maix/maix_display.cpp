#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types_c.h>

#include "maix_display.hpp"

API_BEGIN_NAMESPACE(media)

#ifndef FBDEV_PATH
#define FBDEV_PATH          "/dev/fb0"
#endif

struct maix_disp_priv_t {
    char                     *fbp;
    int                      fbfd;
    bool                     fbiopan;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    int (*init)(struct maix_disp *disp);
    int (*exit)(struct maix_disp *disp);
};

static int disp_draw_image(struct maix_disp *disp, struct maix_image *img)
{
    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t *)disp->reserved;

    if ((disp->width == img->height) && (disp->height == img->width)) {
        cv::Mat src = cv::Mat(img->height, img->width, CV_8UC3, img->data);
        cv::rotate(src, src, cv::ROTATE_90_COUNTERCLOCKWISE);
        uint8_t *rgb888 = (uint8_t *)src.data;
        uint32_t *rgba8888 = (uint32_t *)priv->fbp;

        for (uint32_t *end = rgba8888 + img->width * img->height; rgba8888 != end; rgba8888 += 1, rgb888 += 3) {
            *rgba8888 = (0xFF000000) | ((rgb888[0] << 16) & 0xFF0000) | ((rgb888[1] << 8) & 0xFF00) | (rgb888[2] & 0xFF);
        }
    } else {
        uint8_t *rgb888 = (uint8_t *)img->data;
        uint32_t *rgba8888 = (uint32_t *)priv->fbp;

        for (uint32_t *end = rgba8888 + img->width * img->height; rgba8888 != end; rgba8888 += 1, rgb888 += 3) {
            *rgba8888 = (0xFF000000) | ((rgb888[0] << 16) & 0xFF0000) | ((rgb888[1] << 8) & 0xFF00) | (rgb888[2] & 0xFF);
        }
    }

    priv->vinfo.yoffset = 0;
    priv->vinfo.reserved[0] = 0;
    priv->vinfo.reserved[1] = 0;
    priv->vinfo.reserved[2] = disp->width;
    priv->vinfo.reserved[3] = disp->height;

    if (priv->fbiopan) {
        if (ioctl(priv->fbfd, FBIOPAN_DISPLAY, &priv->vinfo)) {
            maxix_error("ioctl FBIOPAN_DISPLAY, errstr:[%s]", strerror(errno));
            return -1;
        }
    }

    return 0;
}

static int disp_draw(struct maix_disp *disp, unsigned char *buf)
{
    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t *)disp->reserved;

    memcpy((unsigned char *)priv->fbp, buf, disp->width * disp->height * disp->bpp);

    priv->vinfo.yoffset = 0;
    priv->vinfo.reserved[0] = 0;
    priv->vinfo.reserved[1] = 0;
    priv->vinfo.reserved[2] = disp->width;
    priv->vinfo.reserved[3] = disp->height;

    if (priv->fbiopan) {
        if (ioctl(priv->fbfd, FBIOPAN_DISPLAY, &priv->vinfo)) {
            maxix_error("ioctl FBIOPAN_DISPLAY, errstr:[%s]", strerror(errno));
            return -1;
        }
    }

    return 0;
}

static int priv_exit(struct maix_disp *disp)
{
    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t *)disp->reserved;
    if (priv->fbp) {
        munmap(priv->fbp, priv->finfo.smem_len);
        close(priv->fbfd);
    }

    return 0;
}

static int priv_init(struct maix_disp *disp)
{
    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t *)disp->reserved;
    if (priv->fbp == NULL) {
        priv->fbfd = open(FBDEV_PATH, O_RDWR);
        if (-1 == priv->fbfd) {
            maxix_error("open %s failed", FBDEV_PATH);
            return -1;
        }

        if (ioctl(priv->fbfd, FBIOGET_FSCREENINFO, &priv->finfo) == -1) {
            maxix_error("error reading fixed information");
            return -2;
        }

        if (ioctl(priv->fbfd, FBIOGET_VSCREENINFO, &priv->vinfo) == -1) {
            maxix_error("error reading variable information");
            return -3;
        }

        priv->fbp = (char *)mmap(0, priv->finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, priv->fbfd, 0);
        if ((intptr_t)priv->fbp == -1) {
            maxix_error("failed to map framebuffer device to memory");
            return -4;
        }

        disp->width = priv->vinfo.xres;
        disp->height = priv->vinfo.yres;
        disp->bpp = priv->vinfo.bits_per_pixel / 8;

        return 0;
    }

    return -5;
}

static int disp_priv_init(struct maix_disp *disp)
{
    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t *)disp->reserved;

    disp->draw = disp_draw;
    disp->draw_image = disp_draw_image;

    priv->init = priv_init;
    priv->exit = priv_exit;

    return priv->init(disp);
}

struct maix_disp *maix_display_create(bool bFbiopan)
{
    struct maix_disp *disp = (struct maix_disp*)malloc(sizeof(struct maix_disp));
    if (NULL == disp) {
        return NULL;
    }

    memset(disp, 0, sizeof(struct maix_disp));

    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t *)malloc(sizeof(struct maix_disp_priv_t));
    if (NULL == priv) {
        free(disp);
        return NULL;
    }

    memset(priv, 0, sizeof(struct maix_disp_priv_t));
    disp->reserved = (void *)priv;

    if (disp_priv_init(disp) != 0) {
        priv->fbiopan = bFbiopan;
        maix_display_release(&disp);
        return NULL;
    }

    return disp;
}

void maix_display_release(struct maix_disp **disp)
{
    if ((NULL == disp) || (*disp == NULL)) {
        return;
    }

    struct maix_disp_priv_t *priv = (struct maix_disp_priv_t*)(*disp)->reserved;
    if (priv) {
        if (priv->exit) {
            priv->exit(*disp);
        }

        free(priv);
    }

    free(*disp);
    *disp = NULL;
}

API_END_NAMESPACE(media)
