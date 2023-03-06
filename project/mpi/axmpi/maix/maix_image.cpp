#include "maix_image.hpp"

API_BEGIN_NAMESPACE(media)

static int maix_image_soft_convert(struct maix_image *obj, int mode, struct maix_image **new_img)
{
    int err = 0;
    int new_mem = 0;

    if (new_img == NULL) {
        return -1;
    }

    if (mode == obj->mode) {
        return 0;
    }

    if ((obj->width == 0) || (obj->height == 0) || (obj->data == NULL)) {
        return -2;
    }

    if ((*new_img) == NULL) {
        *new_img = maix_image_create(obj->width, obj->height, obj->mode, obj->layout, NULL, true);
        if (!(*new_img)) {
            return -3;
        }

        new_mem = 1;
    } else {
        if ((*new_img)->data == NULL) {
            (*new_img)->data = malloc(obj->width * obj->height * 3);
            if(!(*new_img)->data) {
                return -4;
            }

            (*new_img)->bDataAlloc = true;
            new_mem = 2;
        }

        (*new_img)->layout = obj->layout;
        (*new_img)->width = obj->width;
        (*new_img)->height = obj->height;
    }

    switch (obj->mode) {
        case MAIX_IMAGE_MODE_RGB888: {
            switch (mode) {
                case MAIX_IMAGE_MODE_RGB565: {
                    if ((obj == *new_img) || (obj->width != (*new_img)->width) || (obj->height != (*new_img)->height)) {
                        return -5;
                    }

                    uint8_t *rgb888 = (uint8_t *)obj->data;
                    uint16_t *rgb565 = (uint16_t *)(*new_img)->data;
                    for (uint16_t *end = rgb565 + obj->width * obj->height; rgb565 < end; rgb565 += 1, rgb888 += 3) {
                        *rgb565 = ((((rgb888[0] >> 3) & 31) << 11) | (((rgb888[1] >> 2) & 63) << 5) | ((rgb888[2] >> 3) & 31));
                    }

                    (*new_img)->mode = mode;
                    break;
                }

                case MAIX_IMAGE_MODE_BGR888: {
                    uint8_t *rgb = (uint8_t *)(obj->data), *bgr = (uint8_t *)(*new_img)->data;
                    for (uint8_t *end = rgb + obj->width * obj->height * 3; rgb < end; rgb += 3, bgr += 3) {
                        bgr[2] = rgb[0], bgr[1] = rgb[1], bgr[0] = rgb[2];
                    }

                    (*new_img)->mode = mode;
                    break;
                }

                case MAIX_IMAGE_MODE_RGBA8888: {
                    uint8_t *rgb888 = (uint8_t *)obj->data;
                    uint32_t *rgba8888 = (uint32_t *)(*new_img)->data;
                    for (uint32_t *end = rgba8888 + obj->width * obj->height; rgba8888 < end; rgba8888 += 1, rgb888 += 3) {
                        *rgba8888 = (0xFF000000) | ((rgb888[0] << 16) & 0xFF0000) | ((rgb888[1] << 8) & 0xFF00) | (rgb888[2] & 0xFF);
                    }

                    (*new_img)->mode = mode;
                    break;
                }

                default:
                    err = -6;
                    break;
            }
            break;
        }

        default:
            err = -7;
            break;
    }

    if (err != 0) {
        if (new_mem == 2) {
            free((*new_img)->data);
            (*new_img)->data = NULL;
        } else if (new_mem == 1) {
            maix_image_release(new_img);
        }
    }

    return err;
}

static int maix_image_soft_resize(struct maix_image *obj, int w, int h, struct maix_image **new_img)
{
    return -1;
}

static int maix_image_soft_crop(struct maix_image *obj, int x, int y, int w, int h, struct maix_image **new_img)
{
    return -1;
}

int maix_image_init()
{
    return 0;
}

int maix_image_exit()
{
    return 0;
}

maix_image_t *maix_image_create(int width, int height, int mode, int layout, void *data, bool bDataAlloc)
{
    uint64_t img_size = 0;

    if (!(mode == MAIX_IMAGE_MODE_RGB565
        || mode == MAIX_IMAGE_MODE_RGB888
        || mode == MAIX_IMAGE_MODE_BGR888
        || mode == MAIX_IMAGE_MODE_RGBA8888
        || mode == MAIX_IMAGE_MODE_YUV420SP_NV21
        || mode == MAIX_IMAGE_MODE_YUV422_YUYV
        || mode == MAIX_IMAGE_MODE_GRAY))
    {
        return NULL;
    }

    if (!((width == 0) || (height == 0) || (mode == MAIX_IMAGE_MODE_INVALID) || !bDataAlloc)) {
        if (!data) {
            switch(mode) {
                case MAIX_IMAGE_MODE_RGBA8888:
                    img_size = width * height * 4;
                    break;

                case MAIX_IMAGE_MODE_BGR888:
                case MAIX_IMAGE_MODE_RGB888:
                    img_size = width * height * 3;
                    break;

                case MAIX_IMAGE_MODE_RGB565:
                case MAIX_IMAGE_MODE_YUV422_YUYV:
                    img_size = width * height * 2;
                    break;

                case MAIX_IMAGE_MODE_YUV420SP_NV21:
                    img_size = width * height * 3 / 2;
                    break;

                case MAIX_IMAGE_MODE_GRAY:
                    img_size = width * height;
                    break;

                default:
                    return NULL;
            }

            data = malloc(img_size);
            if (!data) {
                return NULL;
            }

            bDataAlloc = true;
        }
    }

    maix_image_t *img = (maix_image_t *)malloc(sizeof(maix_image_t));
    if (!img) {
        return NULL;
    }

    img->width = width;
    img->height = height;
    img->mode = mode;
    img->layout = layout;
    img->data = data;
    img->bDataAlloc = bDataAlloc;

    img->crop = maix_image_soft_crop;
    img->resize = maix_image_soft_resize;
    img->convert = maix_image_soft_convert;

    return img;
}

void maix_image_release(maix_image_t **obj)
{
    if ((NULL == obj) || (NULL == *obj)) {
        return;
    }

    if ((*obj)->bDataAlloc) {
        free((*obj)->data);
        (*obj)->data = NULL;
    }

    free(*obj);
    *obj = NULL;
}

API_END_NAMESPACE(media)
