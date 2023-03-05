#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

EMSDemoImpl::EMSDemoImpl() : mThreadFin(false)
{
    mPipelines.clear();
}

EMSDemoImpl::~EMSDemoImpl()
{
    mThreadFin = true;
#if defined(CONFIG_XLIB)
    x_main_loop_quit(mMainLoop);
#endif

    mediaDeinit();
}

int EMSDemoImpl::init()
{
    if (getApi()->init() != 0) {
        return -1;
    }

    if (mediaInit()) {
        return -2;
    }

#if defined(CONFIG_XLIB)
    mMainContex = x_main_context_new();
    mMainLoop = x_main_loop_new(mMainContex, FALSE);
    x_main_loop_run(mMainLoop);
    x_main_loop_unref(mMainLoop);
#else
    while (!mThreadFin) {
        sleep(10);
    }
#endif

    return 0;
}

API_END_NAMESPACE(EMS)
