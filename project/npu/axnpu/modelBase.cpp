#include <string.h>
#include <stdlib.h>

#include "modelBase.hpp"
#include "axCommApi.hpp"

API_BEGIN_NAMESPACE(Ai)

int modelSingleBase::init(std::string model)
{
    int ret = mRunner->init(model);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

void modelSingleBase::deinit()
{
    mRunner->deinit();
    if (bMalloc) {
        axsys_memfree(mDstFrame.phyaddr, mDstFrame.viraddr);
    }
}

int modelSingleBase::preprocess(axframe_t *srcFrame, axbbox_t *cropResizeBbox, axres_t *result)
{
    memcpy(&mDstFrame, srcFrame, sizeof(axframe_t));
    bMalloc = false;
    return 0;
}

int modelSingleBase::forward(axframe_t *pstFrame, axbbox_t *cropResizeBbox, axres_t *results)
{
    int ret = preprocess(pstFrame, cropResizeBbox, results);
    if (ret != 0) {
        return ret;
    }

    ret = mRunner->forward(&mDstFrame, cropResizeBbox);
    if (ret != 0) {
        return ret;
    }

    return postprocess(pstFrame, cropResizeBbox, results);
}

API_END_NAMESPACE(Ai)
