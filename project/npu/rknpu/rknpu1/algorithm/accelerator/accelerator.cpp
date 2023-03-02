#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif
#include <sys/mman.h>
#include <npu/private.h>
#include <libdrm/xf86drm.h>
#include <libdrm/drm_mode.h>
#include <rockchip/rkrga/drmrga.h>
#include <rockchip/rkrga/RgaApi.h>

#include "accelerator.hpp"

API_BEGIN_NAMESPACE(hardware)

Accelerator::Accelerator()
    : mInitFin(false), mSrcFmt(RK_FORMAT_BGR_888), mDstFmt(RK_FORMAT_BGR_888), 
    mDrmBuff(nullptr), mDrmBufFd(-1), mDrmBufSize(0), mDrmBufHandle(0)
{

}

Accelerator::~Accelerator()
{
    mInitFin = false;
    drm_buffer_release(mDrmBufFd, mDrmBufHandle, mDrmBuff, mDrmBufSize);
}

int Accelerator::init(int model_width, int model_height, int image_width, int image_height, int image_channles, int bpp, int src_fmt, int dst_fmt, std::string card)
{
    mBpp = bpp;
    mSrcFmt = src_fmt;
    mDstFmt = dst_fmt;
    mDrmCardDev = card;
    mImageChns = image_channles;
    mModelWidth = model_width;
    mImageWidth = image_width;
    mImageHeight = image_height;
    mModelHeight = model_height;

    mDrmBuff = drm_buffer_alloc(mImageWidth, mImageHeight, mImageChns * mBpp, &mDrmBufFd, &mDrmBufHandle, &mDrmBufSize);
    if (mDrmBuff == nullptr) {
        return -1;
    }

    mInitFin = true;
    return 0;
}

int Accelerator::imageResize(uint64_t dstPhyAddr)
{
    if (mInitFin && (dstPhyAddr != 0xFFFFFFFFFFFFFFFF)) {
        return rga_image_resize_fast(dstPhyAddr);
    }

    npu_error("mInitFin:[%s], dstPhyAddr:[0x%llX]", mInitFin ? "true" : "false", dstPhyAddr);
    return -1;
}

int Accelerator::imageResizeEx(void *imageData, void *dstVirAddr)
{
    if (mInitFin && dstVirAddr && imageData) {
        size_t size = mImageWidth * mImageHeight * mImageChns;

        bzero(mDrmBuff, size);
// #if defined(__ARM_NEON) || defined(__ARM_NEON__)
//         neon_memcpy((volatile void *)mDrmBuff, (volatile void *)imageData, size);
// #else
        memcpy(mDrmBuff, imageData, size);
// #endif
        return rga_image_resize_slow(dstVirAddr);
    }

    npu_error("mInitFin:[%s], imageData == nullptr:[%s], dstVirAddr == nullptr:[%s]", mInitFin ? "true" : "false", imageData == nullptr ? "true" : "false", dstVirAddr == nullptr ? "true" : "false");
    return -1;
}

int Accelerator::imageResize(void *imageData, int dstAddrFd)
{
    if (mInitFin && (dstAddrFd > 0) && imageData) {
        return rga_image_resize(imageData, dstAddrFd, NULL);
    }

    npu_error("mInitFin:[%s], dstAddrFd:[%d], imageData == nullptr:[%s]", mInitFin ? "true" : "false", dstAddrFd, imageData == nullptr ? "true" : "false");
    return -1;
}

int Accelerator::imageResize(void *imageData, void *dstVirAddr)
{
    if (mInitFin && dstVirAddr && imageData) {
        return rga_image_resize(imageData, -1, dstVirAddr);
    }

    npu_error("mInitFin:[%s], dstVirAddr == nullptr:[%s], imageData == nullptr:[%s]", mInitFin ? "true" : "false", dstVirAddr == nullptr ? "true" : "false", imageData == nullptr ? "true" : "false");
    return -1;
}

int Accelerator::imageResize(int src_fd, int src_w, int src_h, int src_fmt, int dst_fd, int dst_w, int dst_h, int dst_fmt)
{
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = src_fd;
    src.mmuFlag = 1;
    // src.virAddr = imageData;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = dst_fd;
    dst.mmuFlag = 1;
    // dst.virAddr = dst_buf;
    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);

    int ret = c_RkRgaBlit(&src, &dst, nullptr);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

int Accelerator::imageResize(int src_fd, int src_w, int src_h, int src_fmt, void *dst_buf, int dst_w, int dst_h, int dst_fmt)
{
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = src_fd;
    src.mmuFlag = 1;
    // src.virAddr = imageData;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = -1;
    dst.mmuFlag = 1;
    dst.virAddr = dst_buf;
    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);

    int ret = c_RkRgaBlit(&src, &dst, nullptr);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

int Accelerator::imageResize(void *src_buf, int src_w, int src_h, int src_fmt, int dst_fd, int dst_w, int dst_h, int dst_fmt)
{
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = -1;
    src.mmuFlag = 1;
    src.virAddr = src_buf;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = dst_fd;
    dst.mmuFlag = 1;
    //dst.virAddr = dst_buf;
    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);

    int ret = c_RkRgaBlit(&src, &dst, nullptr);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

int Accelerator::imageResize(void *src_buf, int src_w, int src_h, int src_fmt, void *dst_buf, int dst_w, int dst_h, int dst_fmt)
{
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = -1;
    src.mmuFlag = 1;
    src.virAddr = src_buf;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = -1;
    dst.mmuFlag = 1;
    dst.virAddr = dst_buf;
    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);

    int ret = c_RkRgaBlit(&src, &dst, nullptr);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

int Accelerator::releaseTensorMemory(std::vector<rknn_tensor_mem> &mem)
{
    if (mInitFin) {
        int ret = 0;
        for (size_t i = 0; i < mem.size(); ++i) {
            ret += release_tensor_memory(&mem[i]);
        }

        return ret;
    }

    npu_error("mInitFin:[%s]", mInitFin ? "true" : "false");
    return -1;
}

int Accelerator::createTensorMemory(std::vector<rknn_tensor_mem> &mem, std::vector<rknn_tensor_attr> &attr, bool bFloat, const char *tag)
{
    if (mInitFin) {
        for (size_t i = 0; i < mem.size(); ++i) {
            size_t size = bFloat ? attr[i].size * sizeof(float) : attr[i].size * sizeof(uint8_t);
            int ret = create_tensor_memory(&mem[i], size, i, tag);
            if (ret != 0) {
                npu_error("rknn_create_memory:[%s - %d] failed, return:[%d]", tag, (int)i, ret);
                return -1;
            }
        }

        return 0;
    }

    npu_error("mInitFin:[%s]", mInitFin ? "true" : "false");
    return -1;
}

int Accelerator::release_tensor_memory(rknn_tensor_mem *mem)
{
    return drm_buffer_release(mem->fd, mem->handle, mem->logical_addr, mem->size);
}

int Accelerator::create_tensor_memory(rknn_tensor_mem *mem, size_t size, size_t index, const char *tag)
{
#define STRIDE_ALIGN(n, align)  ((n) + ((align) - 1)) & ~((align) - 1)

    int width = 4096;
    int height = 1;
    int channel = 1;

    if (size <= 4096) {
        width = STRIDE_ALIGN(size, 256);
        height = 1;
        channel = 1;
    } else {
        width = 4096;
        height = (STRIDE_ALIGN(size, 4096)) / 4096;

        if (height > 4096) {
            height = 4096;
            channel = (STRIDE_ALIGN(size, 4096 * 4096)) / 4096 / 4096;
        }
    }

    mem->logical_addr = drm_buffer_alloc(width, height, channel * mBpp, &mem->fd, &mem->handle, (size_t *)&mem->size);
    npu_info("[%s memory] index:[%d], logical_addr:[%p], physical_addr:[0x%llX], fd:[%d], handle:[%u], want:[%7d] bytes, try:[%7d] bytes, actual:[%7d] bytes",
        tag, (int)index, mem->logical_addr, mem->physical_addr, mem->fd, mem->handle, size, width * height * channel, mem->size);

    return 0;
}

void *Accelerator::drm_buffer_alloc(int TexWidth, int TexHeight, int bpp, int *fd, unsigned int *handle, size_t *actual_size)
{
    void *vir_addr = nullptr;
    struct drm_prime_handle fd_args;
    struct drm_mode_map_dumb mmap_arg;
    struct drm_mode_create_dumb alloc_arg;
    struct drm_mode_destroy_dumb destory_arg;

    memset(&alloc_arg, 0x00, sizeof(alloc_arg));
    alloc_arg.bpp = bpp;
    alloc_arg.width = TexWidth;
    alloc_arg.height = TexHeight;
    // alloc_arg.flags = ROCKCHIP_BO_CONTIG;

    // 获取handle和size
    int ret = drmIoctl(mDrmFd, DRM_IOCTL_MODE_CREATE_DUMB, &alloc_arg);
    if (ret) {
        npu_error("failed to create dumb buffer:[%s]", strerror(errno));
        return nullptr;
    }

    if (handle != nullptr) {
        *handle = alloc_arg.handle;
    }

    if (actual_size != nullptr) {
        *actual_size = alloc_arg.size;
    }

    // 获取fd
    memset(&fd_args, 0x00, sizeof(fd_args));
    fd_args.fd = -1;
    fd_args.handle = alloc_arg.handle;;
    fd_args.flags = 0;
    ret = drmIoctl(mDrmFd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &fd_args);
    if (ret) {
        npu_error("drmIoctl failed return:[%d], errstr:[%s], handle:[0x%X]", ret, strerror(errno), fd_args.handle);
        return nullptr;
    }

    if (fd != nullptr) {
        *fd = fd_args.fd;
    }

    // 获取虚拟地址
    memset(&mmap_arg, 0x00, sizeof(mmap_arg));
    mmap_arg.handle = alloc_arg.handle;

    ret = drmIoctl(mDrmFd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
    if (ret) {
        npu_error("failed to create map dumb:[%s]", strerror(errno));
        vir_addr = nullptr;
        goto destory_dumb;
    }

    vir_addr = mmap(0, alloc_arg.size, PROT_READ | PROT_WRITE, MAP_SHARED, mDrmFd, mmap_arg.offset);
    if (vir_addr == MAP_FAILED) {
        npu_error("failed to mmap buffer:[%s]", strerror(errno));
        goto destory_dumb;
    }

    return vir_addr;

destory_dumb:
    memset(&destory_arg, 0x00, sizeof(destory_arg));
    destory_arg.handle = alloc_arg.handle;
    ret = drmIoctl(mDrmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory_arg);
    if (ret) {
        npu_error("failed to destory dumb:[%d]", ret);
    }

    return nullptr;
}

int Accelerator::drm_buffer_release(int buf_fd, int handle, void *drm_buf, size_t size)
{
    if (drm_buf == nullptr) {
        npu_error("drm buffer is nullptr");
        return -1;
    }

    munmap(drm_buf, size);

    struct drm_mode_destroy_dumb destory_arg;
    memset(&destory_arg, 0x00, sizeof(destory_arg));
    destory_arg.handle = handle;

    int ret = drmIoctl(mDrmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &destory_arg);
    if (ret) {
        npu_error("failed to destory dumb:[%d], error:[%s]", ret, strerror(errno));
    }

    if (buf_fd > 0) {
        close(buf_fd);
    }

    return ret;
}

int Accelerator::rga_image_resize_slow(void *dstVirAddr)
{
    int ret = 0;
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = -1;
    src.mmuFlag = 1;
    src.virAddr = (void *)mDrmBuff;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = -1;
    dst.mmuFlag = 1;
    dst.virAddr = dstVirAddr;

    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, mImageWidth, mImageHeight, mImageWidth, mImageHeight, mSrcFmt);
    rga_set_rect(&dst.rect, 0, 0, mModelWidth, mModelHeight, mModelWidth, mModelHeight, mDstFmt);

    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

int Accelerator::rga_image_resize_fast(uint64_t dstPhyAddr)
{
    int ret = 0;
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = mDrmBufFd;
    src.mmuFlag = 1;
    // src.virAddr = (void *)psrc;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = -1;
    dst.mmuFlag = 0;

#if defined(__arm__)
    dst.phyAddr = (void *)((uint32_t)dstPhyAddr);
#else
    dst.phyAddr = (void *)dstPhyAddr;
#endif

    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, mImageWidth, mImageHeight, mImageWidth, mImageHeight, mSrcFmt);
    rga_set_rect(&dst.rect, 0, 0, mModelWidth, mModelHeight, mModelWidth, mModelHeight, mDstFmt);

    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

int Accelerator::rga_image_resize(void *imageData, int dstFd, void *dstVirAddr)
{
    int ret = 0;
    rga_info_t src, dst;

    memset(&src, 0x00, sizeof(rga_info_t));
    src.fd = mDrmBufFd;
    src.mmuFlag = 1;
    // src.virAddr = (void *)psrc;

    memset(&dst, 0x00, sizeof(rga_info_t));
    dst.fd = -1;
    dst.mmuFlag = 0;

#if defined(__arm__)
    dst.phyAddr = (void *)((uint32_t)dstVirAddr);
#else
    dst.phyAddr = (void *)dstVirAddr;
#endif

    dst.nn.nn_flag = 0;

    rga_set_rect(&src.rect, 0, 0, mImageWidth, mImageHeight, mImageWidth, mImageHeight, mSrcFmt);
    rga_set_rect(&dst.rect, 0, 0, mModelWidth, mModelHeight, mModelWidth, mModelHeight, mDstFmt);

    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret) {
        npu_error("c_RkRgaBlit error:[%s]", strerror(errno));
        return -1;
    }

    return 0;
}

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
void Accelerator::neon_memcpy(volatile void *dst, volatile void *src, int sz)
{
#if 0
    if (sz & 63) {
        sz = (sz & -64) + 64;
    }

    asm volatile (
        "NEONCopyPLD: \n"
        " VLDM %[src]!,{d0-d7} \n"
        " VSTM %[dst]!,{d0-d7} \n"
        " SUBS %[sz],%[sz],#0x40 \n"
        " BGT NEONCopyPLD \n"
        : [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz)
        :
        : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory"
    );
#endif
}
#endif

API_END_NAMESPACE(hardware)
