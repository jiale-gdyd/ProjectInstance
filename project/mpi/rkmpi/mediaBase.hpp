#ifndef ROCKCHIP_MEDIABASE_HPP
#define ROCKCHIP_MEDIABASE_HPP

#include <mutex>
#include <atomic>
#include <condition_variable>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaVo.hpp"
#include "mediaVin.hpp"
#include "mediaRga.hpp"
#include "mediaRgn.hpp"
#include "mediaSys.hpp"
#include "mediaVenc.hpp"
#include "mediaVdec.hpp"
#include "mediaVmix.hpp"

#include <media/drm_media_api.h>
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaApi {
public:
    MediaApi();
    ~MediaApi();

    int run();
    void stop();

public:
    MediaVo &getVo() {
        return mMediaVo;
    }

    MediaVin &getVi() {
        return mMediaVin;
    }

    MediaRga &getRga() {
        return mMediaRga;
    }

    MediaRgn &getRgn() {
        return mMediaRgn;
    }

    MediaSys &getSys() {
        return mMediaSys;
    }

    MediaVenc &getVenc() {
        return mMediaVenc;
    }

    MediaVdec &getVdec() {
        return mMediaVdec;
    }

    MediaVmix &getVmix() {
        return mMediaVmix;
    }

private:
    class API_HIDDEN semaphore {
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
    MediaVo   mMediaVo;
    MediaVin  mMediaVin;
    MediaRga  mMediaRga;
    MediaRgn  mMediaRgn;
    MediaSys  mMediaSys;
    MediaVenc mMediaVenc;
    MediaVdec mMediaVdec;
    MediaVmix mMediaVmix;

private:
    semaphore mSemaphore;
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
