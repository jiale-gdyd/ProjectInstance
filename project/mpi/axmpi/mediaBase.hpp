#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "axpi.hpp"
#include "axpipe.hpp"
#include "mediaIvps.hpp"
#include "private/common/common/common_system.hpp"
#include "private/common/common/common_camera.hpp"
#include "private/common/common/common_function.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

namespace axpi {
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
    bool         enableCamera;          // 是否启用摄像头
    axcam_info_t cameraInfo;            // 摄像头配置信息(enableCamera启用时有效)
    axsys_args_t sysCommonArgs;         // 系统通用参数(enableCamera启用时，则不需要关心)
    bool         enableNPU;             // 是否启用NPU
    axnpu_info_t npuInitInfo;           // NPU初始化信息
} axsys_init_params_t;

class MediaApi {
public:
    MediaApi();
    ~MediaApi();

    int init(axsys_init_params_t *param);
    int camInit();

    int run();
    void stop();

public:
    MediaIvps &getIvps() {
        return mMediaIvps;
    }

private:
    void ispRunThread(int camId);

private:
    class semaphore {
    public:
        explicit semaphore(size_t initial = 0) {
            mCount = initial;
        }

        ~semaphore() {

        }

        void post(size_t n = 1) {
            std::unique_lock<std::recursive_mutex> lock(mMutex);
            mCount += n;
            if (n == 1) {
                mCondition.notify_one();
            } else {
                mCondition.notify_all();
            }
        }

        void wait() {
            std::unique_lock<std::recursive_mutex> lock(mMutex);
            while (mCount == 0) {
                mCondition.wait(lock);
            }

            --mCount;
        }

    private:
        size_t                      mCount;
        std::recursive_mutex        mMutex;
        std::condition_variable_any mCondition;
    };

private:
    bool                     mIspLoopOut;
    semaphore                mSemaphore;
    axsys_init_params_t      mSysInitParam;
    axcam_t                  mCamers[MAX_CAMERAS];
    std::vector<std::thread> mIspThreadId;

private:
    MediaIvps                mMediaIvps;
};

class MediaBase {
public:
    MediaBase();
    virtual ~MediaBase();

    virtual int init() = 0;

public:
    axpi::MediaApi *getApi();

private:
    static axpi::MediaApi *mMediaApi;
};
}
