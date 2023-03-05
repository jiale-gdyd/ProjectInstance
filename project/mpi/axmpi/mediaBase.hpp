#ifndef AXERA_MEDIABASE_HPP
#define AXERA_MEDIABASE_HPP

#include <thread>
#include <functional>

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "pipeline/axpipe.hpp"
#include "common/common_camera.hpp"
#include "common/common_function.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaApi {
public:
    MediaApi();
    ~MediaApi();

    int init(int sysCase = SYS_CASE_SINGLE_GC4653, int hdrMode = AX_SNS_LINEAR_MODE, int snsType = GALAXYCORE_GC4653, int frameRate = 25);

private:
    void ispRunThread(int camId);

private:
    std::vector<std::thread> mIspThreadId;

private:
    bool                     mIspLoopOut;
    axsys_args_t             mCommonAgrs;
    axcam_t                  mCamers[MAX_CAMERAS];
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
