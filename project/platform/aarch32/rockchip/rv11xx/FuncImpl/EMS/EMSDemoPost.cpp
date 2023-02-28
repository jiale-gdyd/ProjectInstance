#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::algoInitThread()
{
    return;

    mAlgoInitOk = true;
}

void EMSDemoImpl::inferPostThread()
{
    int cpus = 0;
    int flag = 0;
    cpu_set_t cpuset;

    pthread_setname_np(pthread_self(), "algoPostThread");

    cpus = get_nprocs();
    CPU_ZERO(&cpuset);
    CPU_SET(cpus <= 1 ? 0 : cpus - 2, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    while (!mThreadQuit) {
        if (!mAlgForwardFishRing.isEmpty() && mAlgForwardFishRing.remove(flag)) {
            if (flag == 1) {
                if (mAlgoInitOk) {
                    /* 这里可以取算法运算的结果，然后进行处理 */
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

API_END_NAMESPACE(EMS)
