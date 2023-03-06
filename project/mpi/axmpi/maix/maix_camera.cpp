#include <unistd.h>
#include <ax_ivps_api.h>
#include <opencv2/opencv.hpp>

#include "joint/cv.hpp"
#include "maix_camera.hpp"
#include "../common/common_system.hpp"
#include "../common/common_camera.hpp"
#include "../common/common_function.hpp"

API_BEGIN_NAMESPACE(media)

#define IVPS_CHN_NUM    (1)
#define IVPS_GROUP_NUM  (3)

class AxCropResizeNV12 {
public:
    AxCropResizeNV12() = default;
    ~AxCropResizeNV12() {
        joint::free_cv_image(mInImage);
        joint::free_cv_image(mOutImage);
        delete mBbox;
    }

    int init(int inHeight, int inWidth, int modelHeight, int modelWidth, int modelType) {
        joint::aximg_t input;

        input.w = inWidth;
        input.h = inHeight;
        input.stride_w = input.w;
        input.color_space = AX_NPU_CV_FDT_NV12;
        mInImage = joint::alloc_cv_image(input);
        if (!mInImage) {
            maxix_error("alloc_cv_image input faliled");
            return -1;
        }

        joint::axbox_t box;
        box.h = inHeight;
        box.w = inWidth;
        box.x = 0;
        box.y = 0;
        mBbox = joint::filter_box(input, box);
        if (!mBbox) {
            maxix_error("bbox not legal");
            return -1;
        }

        joint::aximg_t output;
        output.w = modelWidth;
        output.h = modelHeight;
        output.stride_w = output.w;
        output.color_space = AX_NPU_CV_FDT_NV12;
        mOutImage = joint::alloc_cv_image(output);
        if (!mOutImage) {
            maxix_error("alloc_cv_image output faliled");
            return -1;
        }

        mModelType = modelType;
        return 0;
    }

    int cropResizeNV12(void *inputData) {
        int ret = joint::npu_crop_resize(mInImage, (const char *)inputData, mOutImage, mBbox, mModelType);
        if (ret != 0) {
            maxix_error("npu_crop_resize failed, return:[%d]", ret);
            return ret;
        }

        return 0;
    }

    int cropResizeNV12(void *inputData, void *outputData) {
        int ret = joint::npu_crop_resize(mInImage, (const char *)inputData, mOutImage, mBbox, mModelType);
        if (ret != AX_NPU_DEV_STATUS_SUCCESS) {
            maxix_error("npu_crop_resize failed, return:[%d]", ret);
            return ret;
        }

        memcpy(outputData, mOutImage->pVir, mOutImage->tStride.nW * mOutImage->nHeight * 3 / 2);
        return 0;
    }

public:
    joint::axnpu_cvbox_t *mBbox = nullptr;
    joint::axnpu_cvimg_t *mInImage = nullptr;
    joint::axnpu_cvimg_t *mOutImage = nullptr;
    int                  mModelType = AX_NPU_MODEL_TYPE_1_1_1;
};

struct maix_camera_priv_t {
    unsigned char    inited;
    unsigned char    vi_dev;
    unsigned short   vi_x;
    unsigned short   vi_y;
    unsigned short   vi_w;
    unsigned short   vi_h;
    bool             vi_flip;
    bool             vi_mirror;

    maix_image_t     *vi_img;
    AxCropResizeNV12 *nv12_resize_helper;
    void             *ivps_out_data;

    int (*init)(struct maix_camera *cam);
    int (*exit)(struct maix_camera *cam);
};

static int g_isp_force_loop_exit = 0;
static axcam_t gCams[MAX_CAMERAS] = {0};

static void *IspRun(void *arg)
{
    uint32_t camId = (uint32_t)arg;

    while (!g_isp_force_loop_exit) {
        if (gCams[camId].bOpen) {
            AX_ISP_Run(gCams[camId].nPipeId);
        }
    }

    return NULL;
}

static int ivpsInit()
{
    int s32Ret;
    int nGrp, nChn;
    AX_IVPS_GRP_ATTR_S stGrpAttr = {0};
    AX_IVPS_PIPELINE_ATTR_S stPipelineAttr = {0};

    s32Ret = AX_IVPS_Init();
    if (0 != s32Ret) {
        maxix_error("ivps init failed, return:[%d]", s32Ret);
        return -1;
    }

    stPipelineAttr.tFbInfo.PoolId = AX_INVALID_POOLID;
    stPipelineAttr.nOutChnNum = 3;

    for (nGrp = 0; nGrp < IVPS_GROUP_NUM; nGrp++) {
        stGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;
        s32Ret = AX_IVPS_CreateGrp(nGrp, &stGrpAttr);
        if (0 != s32Ret) {
            maxix_error("ivps create group:[%d] failed, return;[%d]", nGrp, s32Ret);
            return -2;
        }

        for (nChn = 0; nChn < IVPS_CHN_NUM; nChn++) {
            if (nGrp == 0) {
                stPipelineAttr.tFilter[nChn + 1][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nSrcFrameRate = 30;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nDstFrameRate = 30;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = 2688;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = 1520;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameWidth = 2688;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameHeight = 1520;
                stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_GDC;
                stPipelineAttr.tFilter[nChn + 1][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_0;
                stPipelineAttr.nOutFifoDepth[nChn] = 1;
            } else if (nGrp == 1) {
                stPipelineAttr.tFilter[nChn + 1][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nSrcFrameRate = 30;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nDstFrameRate = 30;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = 1920;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = 1080;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameWidth = 1920;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameHeight = 1080;
                stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.tFilter[nChn + 1][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_0;
                stPipelineAttr.nOutFifoDepth[nChn] = 1;
            } else {
                stPipelineAttr.tFilter[nChn + 1][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nSrcFrameRate = 30;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nDstFrameRate = 30;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = 720;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = 576;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameWidth = 720;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameHeight = 576;
                stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.tFilter[nChn + 1][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_0;
            }

            s32Ret = AX_IVPS_SetPipelineAttr(nGrp, &stPipelineAttr);
            if (0 != s32Ret) {
                maxix_error("ivps group:[%d] set pipeline attribute failed, return:[%d]", nGrp, s32Ret);
                return -3;
            }

            s32Ret = AX_IVPS_EnableChn(nGrp, nChn);
            if (0 != s32Ret) {
                maxix_error("ivps enable group:[%d] channel:[%d] failed, return:[%d]", nGrp, nChn, s32Ret);
                return -4;
            }
        }

        s32Ret = AX_IVPS_StartGrp(nGrp);
        if (0 != s32Ret) {
            maxix_error("ivps start group:[%d] failed, return:[%d]", nGrp, s32Ret);
            return -5;
        }
    }

    return 0;
}

static int ivpsDeInit()
{
    int s32Ret, nGrp, nChn;

    for (nGrp = 0; nGrp < IVPS_GROUP_NUM; nGrp++) {
        s32Ret = AX_IVPS_StopGrp(nGrp);
        if (0 != s32Ret) {
            maxix_error("ivps stop group:[%d] failed, return:[%d]", nGrp, s32Ret);
            return -1;
        }

        for (nChn = 0; nChn < IVPS_CHN_NUM; nChn++) {
            s32Ret = AX_IVPS_DisableChn(nGrp, nChn);
            if (0 != s32Ret) {
                maxix_error("ivps dislable group:[%d] channel:[%d] failed, return:[%d]", nGrp, nChn, s32Ret);
                return -2;
            }
        }

        s32Ret = AX_IVPS_DestoryGrp(nGrp);
        if (0 != s32Ret) {
            maxix_error("ivps destroy group:[%d] failed, return:[%d]", nGrp, s32Ret);
            return -3;
        }
    }

    s32Ret = AX_IVPS_Deinit();
    if (0 != s32Ret) {
        maxix_error("ivps deinit failed, return:[%d]",  s32Ret);
        return -4;
    }

    return 0;
}

static int bindViIvps()
{
    /**
     * VIN --> IVPS:
     *   (ModId   GrpId   ChnId) | (ModId   GrpId   ChnId)
     *   --------------------------------------------------
     *   (VIN        0       2) -> (IVPS     2       0)
     *   (VIN        0       1) -> (IVPS     1       0)
     *   (VIN        0       0) -> (IVPS     0       0)
     *   (IVPS       2       0)
     *   (IVPS       1       0)
     *   (IVPS       0       0)
     */
    for (int i = 0; i < 3; i++) {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VIN;
        srcMod.s32GrpId = 0;
        srcMod.s32ChnId = i;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = i;
        dstMod.s32ChnId = 0;
        AX_SYS_Link(&srcMod, &dstMod);
    }

    return 0;
}

static int unbindViIvps()
{
    /**
     * VIN --> IVPS:
     *   (ModId   GrpId   ChnId) | (ModId   GrpId   ChnId)
     *   --------------------------------------------------
     *   (VIN        0       2) -> (IVPS     2       0)
     *   (VIN        0       1) -> (IVPS     1       0)
     *   (VIN        0       0) -> (IVPS     0       0)
     *   (IVPS       2       0)
     *   (IVPS       1       0)
     *   (IVPS       0       0)
    */
    for (int i = 0; i < 3; i++)  {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VIN;
        srcMod.s32GrpId = 0;
        srcMod.s32ChnId = i;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = i;
        dstMod.s32ChnId = 0;
        AX_SYS_UnLink(&srcMod, &dstMod);
    }

    return 0;
}

static int vin_init_capture(struct maix_camera *camera)
{
    struct maix_camera_priv_t *priv = (maix_camera_priv_t *)camera->reserved;
    if (priv->vi_dev == 0) {
        ivpsInit();

        priv->nv12_resize_helper = new AxCropResizeNV12();
        priv->nv12_resize_helper->init(1080, 1920, priv->vi_h, priv->vi_w, AX_NPU_MODEL_TYPE_1_1_1);
        priv->ivps_out_data = malloc(1080 * 1920 * 3 / 2);

        priv->vi_img = NULL;
        priv->inited = 1;
    }

    return 0;
}

static int vin_exit_capture(struct maix_camera *camera)
{
    struct maix_camera_priv_t *priv = (maix_camera_priv_t *)camera->reserved;
    if (priv->inited) {
        priv->inited = 0;

        ivpsDeInit();

        delete priv->nv12_resize_helper;
        free(priv->ivps_out_data);

    }
    return priv->inited;
}

static int vin_start_capture(struct maix_camera *camera)
{
    struct maix_camera_priv_t *priv = (maix_camera_priv_t*)camera->reserved;
    if (priv->inited) {
        camera->fram_size = (camera->width * camera->height * 3);

        bindViIvps();
        return 0;
    }

    return -1;
}

static int vin_priv_capture_image(struct maix_camera *camera, struct maix_image **img)
{
    struct maix_camera_priv_t *priv = (maix_camera_priv_t *)camera->reserved;
    if (priv->vi_img == NULL) {
        priv->vi_img = maix_image_create(priv->vi_w, priv->vi_h, MAIX_IMAGE_MODE_RGB888, MAIX_IMAGE_LAYOUT_HWC, NULL, true);
        if (!priv->vi_img) {
            return -1;
        }
    }

    if (priv->inited) {
        AX_VIDEO_FRAME_S video_frame_s = {0};
        memset(&video_frame_s, 0, sizeof(AX_VIDEO_FRAME_S));
        AX_S32 ret = AX_IVPS_GetChnFrame(1, 0, &video_frame_s, 0);
        if (0 == ret) {
            int pixel_size = (int)video_frame_s.u32Width * (int)video_frame_s.u32Height;
            video_frame_s.u64VirAddr[0] = (AX_ULONG)AX_SYS_Mmap(video_frame_s.u64PhyAddr[0], pixel_size);
            video_frame_s.u64VirAddr[1] = (AX_ULONG)AX_SYS_Mmap(video_frame_s.u64PhyAddr[1], pixel_size / 2);
            memcpy(priv->ivps_out_data, (void *)video_frame_s.u64VirAddr[0], pixel_size);
            memcpy((void *)((char *)priv->ivps_out_data + pixel_size), (void *)video_frame_s.u64VirAddr[1], pixel_size / 2);

            priv->nv12_resize_helper->cropResizeNV12(priv->ivps_out_data);
            auto ret0 = AX_SYS_Munmap((void *)video_frame_s.u64VirAddr[0], pixel_size);
            auto ret1 = AX_SYS_Munmap((void *)video_frame_s.u64VirAddr[1], pixel_size / 2);

            AX_IVPS_ReleaseChnFrame(1, 0, &video_frame_s);
            if (ret0 || ret1) {
                maxix_error("AX_SYS_Munmap failed, ret0:[%X, ret1:[%X]", ret0, ret1);
            }

            cv::Mat input(priv->vi_h * 3 / 2, priv->vi_w, CV_8UC1, priv->nv12_resize_helper->mOutImage->pVir);
            cv::Mat rgb(priv->vi_h, priv->vi_w, CV_8UC3, priv->vi_img->data);
            cv::cvtColor(input, rgb, cv::COLOR_YUV2RGB_NV12);

            *img = priv->vi_img;
            return 0;
        } else {
            AX_IVPS_ReleaseChnFrame(1, 0, &video_frame_s);
            usleep(40 * 1000);
            return -2;
        }
    }

    return -3;
}

static int vin_priv_capture(struct maix_camera *camera, unsigned char *buf)
{
    struct maix_camera_priv_t *priv = (maix_camera_priv_t *)camera->reserved;
    if (priv->inited) {
        ;
    }

    return 1;
}

static int camera_priv_init(struct maix_camera *camera)
{
    struct maix_camera_priv_t *priv = (struct maix_camera_priv_t *)camera->reserved;
    if (NULL == priv) {
        maxix_error("private data is NULL");
        return -1;
    }

    priv->init = vin_init_capture;
    priv->exit = vin_exit_capture;
    camera->capture = vin_priv_capture;
    camera->start_capture = vin_start_capture;
    camera->capture_image = vin_priv_capture_image;

    return priv->init(camera);
}

void maix_camera_init()
{
    int axRet = 0;
    axsys_args_t tCommonArgs = {0};
    int eHdrMode = AX_SNS_LINEAR_MODE;
    int eSnsType = OMNIVISION_OS04A10;
    int eSysCase = SYS_CASE_SINGLE_GC4653;

    if (eSysCase == SYS_CASE_SINGLE_OS04A10) {
        tCommonArgs.camCount = 1;
        eSnsType = OMNIVISION_OS04A10;
        axisp_get_sns_config(OMNIVISION_OS04A10, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr, &gCams[0].stChnAttr);

        if (eHdrMode == AX_SNS_LINEAR_MODE){
            tCommonArgs.poolCfgCount = sizeof(gtSysCommPoolSingleOs04a10Sdr) / sizeof(gtSysCommPoolSingleOs04a10Sdr[0]);
            tCommonArgs.poolCfg  = gtSysCommPoolSingleOs04a10Sdr;
        } else if(eHdrMode == AX_SNS_HDR_2X_MODE){
            tCommonArgs.poolCfgCount = sizeof(gtSysCommPoolSingleOs04a10Hdr) / sizeof(gtSysCommPoolSingleOs04a10Hdr[0]);
            tCommonArgs.poolCfg  = gtSysCommPoolSingleOs04a10Hdr;
        }
    } else if (eSysCase == SYS_CASE_SINGLE_GC4653) {
        tCommonArgs.camCount = 1;
        eSnsType = GALAXYCORE_GC4653;
        tCommonArgs.poolCfgCount = sizeof(gtSysCommPoolSingleGc4653) / sizeof(gtSysCommPoolSingleGc4653[0]);
        tCommonArgs.poolCfg  = gtSysCommPoolSingleGc4653;
        axisp_get_sns_config(GALAXYCORE_GC4653, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr, &gCams[0].stChnAttr);
    }

    for (int i = 0; i < tCommonArgs.camCount; i++) {
        gCams[i].eSnsType = eSnsType;
        gCams[i].stSnsAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;
        gCams[i].stDevAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;
        gCams[i].stPipeAttr.eSnsMode = (AX_SNS_HDR_MODE_E)eHdrMode;

        if (i == 0) {
            gCams[i].nDevId = 0;
            gCams[i].nRxDev = AX_MIPI_RX_DEV_0;
            gCams[i].nPipeId = 0;
        } else if (i == 1) {
            gCams[i].nDevId = 2;
            gCams[i].nRxDev = AX_MIPI_RX_DEV_2;
            gCams[i].nPipeId = 2;
        }
    }

    AX_NPU_SDK_EX_ATTR_T sNpuAttr;
    sNpuAttr.eHardMode = AX_NPU_VIRTUAL_1_1;
    axRet = AX_NPU_SDK_EX_Init_with_attr(&sNpuAttr);
    if (0 != axRet) {
        maxix_error("AX_NPU_SDK_EX_Init_with_attr failed, return:[%d]", axRet);
        return;
    }

    axRet = axsys_init(&tCommonArgs);
    if (axRet) {
        maxix_error("isp sys init failed");
        return;
    }

    axcam_init();

    for (int i = 0; i < tCommonArgs.camCount; i++) {
        axRet = axcam_open(&gCams[i]);
        if (axRet) {
            maxix_error("camera:[%d] open failed", i);
            return;
        }

        gCams[i].bOpen = AX_TRUE;
        if (gCams[i].bOpen) {
            pthread_create(&gCams[i].tIspProcThread, NULL, IspRun, (AX_VOID *)i);
        }
    }

    g_isp_force_loop_exit = 0;
}

void maix_camera_exit()
{
    int axRet = 0;

    g_isp_force_loop_exit = 1;
    for (int i = 0; i < MAX_CAMERAS; i++) {
        if (gCams[i].bOpen) {
            pthread_cancel(gCams[i].tIspProcThread);
            axRet = pthread_join(gCams[i].tIspProcThread, NULL);
            if (axRet < 0) {
                maxix_error("isp run thread exit failed, return:[%d]", axRet);
            }
        }
    }

    for (int i = 0; i < MAX_CAMERAS; i++) {
        if (!gCams[i].bOpen) {
            continue;
        }

        axcam_close(&gCams[i]);
    }

    axcam_deinit();
    axsys_deinit();
}

maix_camera_t *maix_camera_create(int camId, int width, int height, bool mirror, bool flip)
{
    struct maix_camera *camera = (struct maix_camera *)malloc(sizeof(struct maix_camera));
    if (NULL == camera) {
        return NULL;
    }

    camera->width = width;
    camera->height = height;

    struct maix_camera_priv_t *priv = (struct maix_camera_priv_t *)malloc(sizeof(struct maix_camera_priv_t));
    if (NULL == priv) {
        free(camera);
        return NULL;
    }

    memset(priv, 0, sizeof(struct maix_camera_priv_t));
    camera->reserved = (void*)priv;

    priv->vi_dev = camId;
    priv->vi_x = 0;
    priv->vi_y = 0;
    priv->vi_w = width;
    priv->vi_h = height;
    priv->vi_flip = flip;
    priv->vi_mirror = mirror;
    priv->inited = 0;

    if (camera_priv_init(camera) != 0) {
        maix_camera_release(&camera);
        return NULL;
    }

    return camera;
}

void maix_camera_release(maix_camera_t **camera)
{
    if ((NULL == camera) || (NULL == *camera)) {
        return;
    }

    struct maix_camera_priv_t *priv = (struct maix_camera_priv_t *)(*camera)->reserved;
    if (priv) {
        if (priv->vi_img != NULL) {
            maix_image_release(&priv->vi_img);
        }
    
        if (priv->exit) {
            priv->exit(*camera);
        }

        free(priv);
    }

    free(*camera);
    *camera = NULL;
}

API_END_NAMESPACE(media)
