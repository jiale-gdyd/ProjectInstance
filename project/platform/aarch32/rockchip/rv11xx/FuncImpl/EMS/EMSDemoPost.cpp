#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::algoInitThread()
{
    pthread_setname_np(pthread_self(), "algoInitThread");

#if defined(CONFIG_RKNPU)
    pDetector = std::make_shared<Ai::AiDetector>();
    if (!pDetector) {
        printf("std::make_shared<Ai::AiDetector>() failed\n");
        return;
    }

    int ret = pDetector->init("/opt/aure/model/yoloxs_512x288.rknn", mOriginWidth, mOriginHeight, 7, 0.5f, 0.45f, Ai::YOLOXS, Ai::AUTHOR_JIALELU);
    // int ret = pDetector->init("/opt/aure/model/yolov5s_512x288.rknn", mOriginWidth, mOriginHeight, 5, 0.5f, 0.45f, Ai::YOLOV5S, Ai::AUTHOR_JIALELU);
    // int ret = pDetector->init("/opt/aure/model/yoloxtiny_face_512x288.rknn", mOriginWidth, mOriginHeight, 1, 0.5f, 0.45f, Ai::YOLOX_TINY_FACE, Ai::AUTHOR_JIALELU);
    // int ret = pDetector->init("/opt/aure/model/yoloxnano_face_512x288.rknn", mOriginWidth, mOriginHeight, 1, 0.5f, 0.45f, Ai::YOLOX_NANO_FACE, Ai::AUTHOR_JIALELU);
    // int ret = pDetector->init("/opt/aure/model/yoloxnano_face_tiny_512x288.rknn", mOriginWidth, mOriginHeight, 1, 0.5f, 0.45f, Ai::YOLOX_NANO_FACE_TINY, Ai::AUTHOR_JIALELU);

    if (ret != 0) {
        printf("pDetector->init failed, return:[%d]\n", ret);
        return;
    }

    mAlgoInitOk = true;
#endif
}

void EMSDemoImpl::inferPostThread()
{
    int cpus = 0;
    int flag = 0;
    cpu_set_t cpuset;
    std::vector<Ai::bbox> lastResult;

    pthread_setname_np(pthread_self(), "algoPostThread");

    cpus = get_nprocs();
    CPU_ZERO(&cpuset);
    CPU_SET(cpus <= 1 ? 0 : cpus - 2, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (!mThreadQuit) {
        if (!mAlgForwardFishRing.isEmpty() && mAlgForwardFishRing.remove(flag)) {
            if (flag == 1) {
                if (mAlgoInitOk) {
#if defined(CONFIG_RKNPU)
                    lastResult.clear();
                    int ret = pDetector->extract(lastResult);
                    if ((lastResult.size() > 0) && (ret == 0)) {
                        mAlgExtractResultRing.insert(lastResult);
                    }
#endif
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

API_END_NAMESPACE(EMS)
