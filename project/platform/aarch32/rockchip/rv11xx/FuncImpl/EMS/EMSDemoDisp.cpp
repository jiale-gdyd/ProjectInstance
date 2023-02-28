#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::drawPostThread()
{
    int cpus = 0;
    cpu_set_t cpuset;
    media_buffer_t mediaFrame = NULL;

    pthread_setname_np(pthread_self(), "drawPostThread");

    cpus = get_nprocs();
    CPU_ZERO(&cpuset);
    CPU_SET(cpus <= 1 ? 0 : cpus - 1, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (!mThreadQuit) {
        if (!mDispRawFrameRing.isEmpty() && mDispRawFrameRing.remove(mediaFrame)) {
            /* 在这里可以绘制信息，然后在下面发送出去进行显示 */

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
