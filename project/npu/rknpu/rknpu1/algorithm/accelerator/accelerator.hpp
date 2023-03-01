#ifndef AARCH32_ROCKCHIP_ALGORITHM_ACCELERATOR_HPP
#define AARCH32_ROCKCHIP_ALGORITHM_ACCELERATOR_HPP

#include <string>
#include <vector>
#include <stdbool.h>
#include <utils/export.h>
#include <rknn/rknn_runtime.h>
#include <rockchip/rkrga/rga.h>

API_BEGIN_NAMESPACE(hardware)

class API_HIDDEN Accelerator {
public:
    Accelerator();
    ~Accelerator();

    int init(int model_width, int model_height, int image_width, int image_height, int image_channles = 3, int bpp = 8, int src_fmt = RK_FORMAT_BGR_888, int dst_fmt = RK_FORMAT_BGR_888, std::string card = "/dev/dri/card0");

    int imageResize(uint64_t dstPhyAddr);
    int imageResizeEx(void *imageData, void *dstVirAddr);

    int imageResize(void *imageData, int dstAddrFd);
    int imageResize(void *imageData, void *dstVirAddr);

    int imageResize(int src_fd, int src_w, int src_h, int src_fmt, int dst_fd, int dst_w, int dst_h, int dst_fmt);
    int imageResize(int src_fd, int src_w, int src_h, int src_fmt, void *dst_buf, int dst_w, int dst_h, int dst_fmt);
    int imageResize(void *src_buf, int src_w, int src_h, int src_fmt, int dst_fd, int dst_w, int dst_h, int dst_fmt);
    int imageResize(void *src_buf, int src_w, int src_h, int src_fmt, void *dst_buf, int dst_w, int dst_h, int dst_fmt);

    int releaseTensorMemory(std::vector<rknn_tensor_mem> &mem);
    int createTensorMemory(std::vector<rknn_tensor_mem> &mem, std::vector<rknn_tensor_attr> &attr, bool bFloat, const char *tag);

private:
    int rga_image_resize_slow(void *dstVirAddr);
    int rga_image_resize_fast(uint64_t dstPhyAddr);
    int rga_image_resize(void *imageData, int dstFd, void *dstVirAddr);

private:
    int drm_buffer_release(int buf_fd, int handle, void *drm_buf, size_t size);
    void *drm_buffer_alloc(int TexWidth, int TexHeight, int bpp, int *fd, unsigned int *handle, size_t *actual_size);

private:
    int release_tensor_memory(rknn_tensor_mem *mem);
    int create_tensor_memory(rknn_tensor_mem *mem, size_t size, size_t index, const char *tag);

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    void neon_memcpy(volatile void *dst, volatile void *src, int sz);
#endif

private:
    Accelerator(Accelerator const &) = delete;
    Accelerator &operator=(Accelerator const &) = delete;

private:
    bool        mInitFin;

    int         mDrmFd;
    int         mDrmBufFd;
    void        *mDrmBuff;
    size_t      mDrmBufSize;
    uint32_t    mDrmBufHandle;

    int         mBpp;
    int         mSrcFmt;
    int         mDstFmt;
    int         mImageChns;
    int         mModelWidth;
    int         mImageWidth;
    int         mImageHeight;
    int         mModelHeight;
    std::string mDrmCardDev;
};

API_END_NAMESPACE(hardware)

#endif
