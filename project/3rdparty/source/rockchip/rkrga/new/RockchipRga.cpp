#define LOG_NDEBUG 0

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG     "rockchiprga"
#endif

#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <libdrm/drm.h>
#include <rockchip/rkrgax/im2d.h>
#include <rockchip/rkrgax/rgadbg.h>
#include <rockchip/rkrgax/NormalRga.h>
#include <rockchip/rkrgax/RockchipRga.h>

static int local_drmIoctl(int fd, unsigned long request, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while ((ret == -1) && ((errno == EINTR) || (errno == EAGAIN)));

    return ret;
}

static int local_drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    int ret;
    struct drm_prime_handle args;

    memset(&args, 0, sizeof(args));
    args.fd = -1;
    args.handle = handle;
    args.flags = flags;
    ret = local_drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
    if (ret) {
        return ret;
    }

    *prime_fd = args.fd;
    return 0;
}

RGA_SINGLETON_STATIC_INSTANCE(RockchipRga)

RockchipRga::RockchipRga() : mSupportRga(false), mLogOnce(0), mLogAlways(0), mContext(NULL)
{
    RkRgaInit();
    rga_info("%s", RGA_API_FULL_VERSION);
}

RockchipRga::~RockchipRga()
{
    RgaDeInit(&mContext);
}

int RockchipRga::RkRgaInit()
{
    int ret = 0;
    if (mSupportRga) {
        return 0;
    }

    ret = RgaInit(&mContext);
    if (ret >= 0) {
        mSupportRga = true;
    } else {
        mSupportRga = false;
    }

    return ret;
}

void RockchipRga::RkRgaDeInit()
{
    if (mSupportRga) {
        RgaDeInit(&mContext);
    }

    mSupportRga = false;
}

void RockchipRga::RkRgaGetContext(void **ctx)
{
    *ctx = mContext;
}

int RockchipRga::RkRgaAllocBuffer(int drm_fd, bo_t *bo_info, int width, int height, int bpp, int flags)
{
    int ret;
    struct drm_mode_create_dumb arg;

    memset(&arg, 0, sizeof(arg));
    arg.bpp = bpp;
    arg.width = width;
    arg.height = height;
    arg.flags = flags;

    ret = local_drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
    if (ret) {
        rga_error("failed to create dumb buffer, errstr:[%s]", strerror(errno));
        return ret;
    }

    bo_info->handle = arg.handle;
    bo_info->size = arg.size;
    bo_info->pitch = arg.pitch;

    return 0;
}

int RockchipRga::RkRgaFreeBuffer(int drm_fd, bo_t *bo_info)
{
    int ret;
    struct drm_mode_destroy_dumb arg;

    if (bo_info->handle <= 0) {
        return -EINVAL;
    }

    memset(&arg, 0, sizeof(arg));
    arg.handle = bo_info->handle;
    ret = local_drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
    if (ret) {
        rga_error("failed to destroy dumb buffer, errstr:[%s]", strerror(errno));
        return -errno;
    }
    bo_info->handle = 0;

    return 0;
}

int RockchipRga::RkRgaGetAllocBufferExt(bo_t *bo_info, int width, int height, int bpp, int flags)
{
    int ret;
    int drm_fd;
    int flag = O_RDWR;
    static const char *card = "/dev/dri/card0";

#ifdef O_CLOEXEC
    flag |= O_CLOEXEC;
#endif
    bo_info->fd = -1;
    bo_info->handle = 0;
    drm_fd = open(card, flag);
    if (drm_fd < 0) {
        rga_error("Fail to open %s, errstr:[%m]", card);
        return -errno;
    }

    ret = RkRgaAllocBuffer(drm_fd, bo_info, width, height, bpp, flags);
    if (ret) {
        close(drm_fd);
        return ret;
    }

    bo_info->fd = drm_fd;
    return 0;
}

int RockchipRga::RkRgaGetAllocBuffer(bo_t *bo_info, int width, int height, int bpp)
{
    return RkRgaGetAllocBufferExt(bo_info, width, height, bpp, 0);
}

int RockchipRga::RkRgaGetAllocBufferCache(bo_t *bo_info, int width, int height, int bpp)
{
    return RkRgaGetAllocBufferExt(bo_info, width, height, bpp, ROCKCHIP_BO_CACHABLE);
}

int RockchipRga::RkRgaGetMmap(bo_t *bo_info)
{
    int ret;
    void *map;
    struct drm_mode_map_dumb arg;

    memset(&arg, 0, sizeof(arg));
    arg.handle = bo_info->handle;
    ret = local_drmIoctl(bo_info->fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
    if (ret) {
        return ret;
    }

    map = mmap64(0, bo_info->size, PROT_READ | PROT_WRITE, MAP_SHARED, bo_info->fd, arg.offset);
    if (map == MAP_FAILED) {
        return -EINVAL;
    }
    bo_info->ptr = map;

    return 0;
}

int RockchipRga::RkRgaUnmap(bo_t *bo_info)
{
    munmap(bo_info->ptr, bo_info->size);
    bo_info->ptr = NULL;
    return 0;
}

int RockchipRga::RkRgaFree(bo_t *bo_info)
{
    int ret;
    if (bo_info->fd < 0) {
        return -EINVAL;
    }

    ret = RkRgaFreeBuffer(bo_info->fd, bo_info);
    close(bo_info->fd);
    bo_info->fd = -1;

    return ret;
}

int RockchipRga::RkRgaGetBufferFd(bo_t *bo_info, int *fd)
{
    int ret = 0;
    ret = local_drmPrimeHandleToFD(bo_info->fd, bo_info->handle, DRM_CLOEXEC | DRM_RDWR, fd);
    return ret;
}

int RockchipRga::RkRgaBlit(rga_infos *src, rga_infos *dst, rga_infos *src1)
{
    int ret = 0;

    ret = RgaBlit(src, dst, src1);
    if (ret) {
        RkRgaLogOutUserPara(src);
        RkRgaLogOutUserPara(dst);
        RkRgaLogOutUserPara(src1);
    }

    return ret;
}

int RockchipRga::RkRgaFlush()
{
    int ret = 0;

    ret = RgaFlush();
    if (ret) {
        rga_error("RgaFlush Failed");
    }

    return ret;
}

int RockchipRga::RkRgaCollorFill(rga_infos *dst)
{
    int ret = 0;
    ret = RgaCollorFill(dst);
    return ret;
}

int RockchipRga::RkRgaCollorPalette(rga_infos *src, rga_infos *dst, rga_infos *lut)
{
    int ret = 0;

    ret = RgaCollorPalette(src, dst, lut);
    if (ret) {
        RkRgaLogOutUserPara(src);
        RkRgaLogOutUserPara(dst);
    }

    return ret;
}

int RockchipRga::RkRgaLogOutUserPara(rga_infos *rgaInfo)
{
    if (!rgaInfo) {
        return -EINVAL;
    }

    rga_info("handl-fd-vir-phy-hnd-format:[%d, %d, %p, %p, %lx, %d]", rgaInfo->handle, rgaInfo->fd, rgaInfo->virAddr, rgaInfo->phyAddr, (unsigned long)rgaInfo->hnd, rgaInfo->format);
    rga_info("rect:[%d, %d, %d, %d, %d, %d, %d, %d]", rgaInfo->rect.xoffset, rgaInfo->rect.yoffset, rgaInfo->rect.width, rgaInfo->rect.height, rgaInfo->rect.wstride, rgaInfo->rect.hstride, rgaInfo->rect.format, rgaInfo->rect.size);
    rga_info("f-blend-size-rotation-col-log-mmu:[%d, %x, %d, %d, %d, %d, %d]", rgaInfo->format, rgaInfo->blend, rgaInfo->bufferSize, rgaInfo->rotation, rgaInfo->color, rgaInfo->testLog, rgaInfo->mmuFlag);

    return 0;
}
