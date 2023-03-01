#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::drawPostThread()
{
    int cpus = 0;
    cpu_set_t cpuset;
    int usePrevResultCount = 0;
    media_buffer_t mediaFrame = NULL;
    std::vector<Ai::bbox> prevResult, lastResult;

    pthread_setname_np(pthread_self(), "drawPostThread");

    cpus = get_nprocs();
    CPU_ZERO(&cpuset);
    CPU_SET(cpus <= 1 ? 0 : cpus - 1, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (!mThreadQuit) {
        if (!mDispRawFrameRing.isEmpty() && mDispRawFrameRing.remove(mediaFrame)) {
            if (!mAlgExtractResultRing.isEmpty() && mAlgExtractResultRing.remove(lastResult)) {
                prevResult = lastResult;
                usePrevResultCount = 0;
            } else {
                lastResult = prevResult;
                if (++usePrevResultCount > 1) {
                    usePrevResultCount = 0;
                    prevResult.clear();
                }
            }

            if (lastResult.size() > 0) {
                dispObjects(mediaFrame, lastResult);
            }

            if (!mSendDrawFrameRing.insert(mediaFrame)) {
                getMedia()->getRga().releaseFrame(mediaFrame);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void EMSDemoImpl::dispPostThread()
{
    media_buffer_t dispFrame = NULL;

    pthread_setname_np(pthread_self(), "dispPostThread");

    while (!mThreadQuit) {
        if (!mSendDrawFrameRing.isEmpty() && mSendDrawFrameRing.remove(dispFrame)) {
            sendZoomFrame(dispFrame, mPrimaryVoChn, mPrimaryVoDisp, mCropRgaChn, mZoomRgaChn);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

API_END_NAMESPACE(EMS)
