#ifndef AXERA_MEDIABASE_HPP
#define AXERA_MEDIABASE_HPP

#include <thread>
#include <functional>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaIvps.hpp"
#include "pipeline/axpipe.hpp"
#include "common/common_camera.hpp"
#include "common/common_function.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

typedef struct {
    uint32_t sysCameraCase;
    uint32_t sysCameraHdrMode;
    uint32_t sysCameraSnsType;
    uint32_t cameraIvpsFrameRate;
} axcam_info_t;

typedef struct {
    int npuHdrMode;
} axnpu_info_t;

typedef struct {
    bool             enableCamera;          // 是否启用摄像头
    union {
        axcam_info_t cameraInfo;            // 摄像头配置信息(enableCamera启用时有效)
        axsys_args_t sysCommonArgs;         // 系统通用参数(enableCamera启用时，则不需要关心)
    };
    bool             enableNPU;             // 是否启用NPU
    axnpu_info_t     npuInitInfo;           // NPU初始化信息
} axsys_init_params_t;

class API_HIDDEN MediaApi {
public:
    MediaApi();
    ~MediaApi();

    int init(axsys_init_params_t *param);

    int run();
    void stop();

public:
    MediaIvps &getIvps() {
        return mMediaIvps;
    }

private:
    void ispRunThread(int camId);

private:
    bool                     mIspLoopOut;
    axsys_init_params_t      mSysInitParam;
    axcam_t                  mCamers[MAX_CAMERAS];
    std::vector<std::thread> mIspThreadId;

private:
    MediaIvps                mMediaIvps;
};

class API_EXPORT MediaBase {
public:
    MediaBase();
    virtual ~MediaBase();

    virtual int init() = 0;

public:
    media::MediaApi *getApi();

private:
    static media::MediaApi *mMediaApi;
};

API_END_NAMESPACE(media)

#endif
