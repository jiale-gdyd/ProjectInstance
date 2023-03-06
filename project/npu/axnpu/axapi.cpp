#include <mutex>
#include <memory>

#include "axapi.h"
#include "modelBase.hpp"

API_BEGIN_NAMESPACE(Ai)

struct model_handler {
    std::shared_ptr<modelBase> model = nullptr;
    std::mutex                 locker;
};

int axnpu_init(void **handler, std::string model)
{
    *handler = new model_handler;
    return ((struct model_handler *)(*handler))->model->init(model);
}

void axnpu_deinit(void **handler)
{
    if (handler && (struct model_handler *)(*handler) && ((struct model_handler *)(*handler))->model.get()) {
        ((struct model_handler *)(*handler))->model->deinit();
        delete (struct model_handler *)(*handler);
        *handler = nullptr;
    }
}

int axnpu_get_ivps_width_height(void *handler, int *width_ivps, int *height_ivps)
{
    if (!(struct model_handler *)(handler) || !((struct model_handler *)(handler))->model.get()) {
        return -1;
    }

    switch (((struct model_handler *)handler)->model->getModelType()) {
        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_ANIMAL_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            *width_ivps = 960;
            *height_ivps = 540;
            ((struct model_handler *)handler)->model->setDetRestoreResolution(*width_ivps, *height_ivps);
            break;

        default:
            *width_ivps = ((struct model_handler *)handler)->model->getModelWidth();
            *height_ivps = ((struct model_handler *)handler)->model->getModelHeight();
            break;
    }

    return 0;
}

int axnpu_get_model_type(void *handler)
{
    if (!(struct model_handler *)(handler) || !((struct model_handler *)(handler))->model.get()) {
        return -1;
    }

    return ((struct model_handler *)handler)->model->getModelType();
}

int axnpu_get_color_space(void *handler)
{
    if (!(struct model_handler *)(handler) || !((struct model_handler *)(handler))->model.get()) {
        return COLOR_SPACE_UNK;
    }

    return ((struct model_handler *)handler)->model->getColorSpace();
}

int axnpu_forward(void *handler, axframe_t *pstFrame, axres_t *pResults)
{
    if (!(struct model_handler *)(handler) || !((struct model_handler *)(handler))->model.get()) {
        return -1;
    }

    std::lock_guard<std::mutex> locker(((struct model_handler *)handler)->locker);
    pResults->mModelType = ((struct model_handler *)handler)->model->getModelType();
    int ret = ((struct model_handler *)handler)->model->forward(pstFrame, nullptr, pResults);
    if (ret) {
        return -1;
    }

    int width, height;
    ((struct model_handler *)handler)->model->getDetRestoreResolution(width, height);
    for (int i = 0; i < pResults->nObjSize; i++) {
        pResults->mObjects[i].bbox.x /= width;
        pResults->mObjects[i].bbox.y /= height;
        pResults->mObjects[i].bbox.w /= width;
        pResults->mObjects[i].bbox.h /= height;

        for (int j = 0; j < pResults->mObjects[i].nLandmark; j++) {
            pResults->mObjects[i].landmark[j].x /= width;
            pResults->mObjects[i].landmark[j].y /= height;
        }

        if (pResults->mObjects[i].bHasBoxVertices) {
            for (size_t j = 0; j < 4; j++) {
                pResults->mObjects[i].bbox_vertices[j].x /= width;
                pResults->mObjects[i].bbox_vertices[j].y /= height;
            }
        }
    }

    for (int i = 0; i < pResults->nCrowdCount; i++) {
        pResults->mCrowdCountPts[i].x /= width;
        pResults->mCrowdCountPts[i].y /= height;
    }

    {
        static int fcnt = 0;
        static int fps = -1;

        fcnt++;
        static struct timespec ts1, ts2;
        clock_gettime(CLOCK_MONOTONIC, &ts2);

        if ((ts2.tv_sec * 1000 + ts2.tv_nsec / 1000000) - (ts1.tv_sec * 1000 + ts1.tv_nsec / 1000000) >= 1000) {
            fps = fcnt;
            ts1 = ts2;
            fcnt = 0;
        }

        pResults->niFps = fps;
    }

    return 0;
}

int axnpu_draw_results(void *handler, axcanvas_t *canvas, axres_t *pResults, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (!(struct model_handler *)(handler) || !((struct model_handler *)(handler))->model.get()) {
        return -1;
    }

    cv::Mat image(canvas->height, canvas->width, CV_8UC4, canvas->data);
    ((struct model_handler *)handler)->model->draw_results(image, pResults, fontscale, thickness, offset_x, offset_y);

    return 0;
}

API_END_NAMESPACE(Ai)
