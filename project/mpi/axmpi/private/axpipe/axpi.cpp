#include <fstream>
#include <nlohmann/json.hpp>

#include "../../axpi.hpp"
#include "axpi_model_base.hpp"
#include "../utilities/log.hpp"
#include "../utilities/objectRegister.hpp"

namespace axpi {
struct axpi_model_handle_t {
    std::shared_ptr<AxPiModelBase> model = nullptr;
    std::mutex                     locker;
};

int axpi_init(std::string confJsonFile, void **handler)
{
    std::ifstream f(confJsonFile);
    if (f.fail()) {
        axmpi_error("json file:[%s] is not exist.", confJsonFile.c_str());
        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);
    f.close();

    std::string strModelType;
    int mt = AxPiModelBase::getModelType(&jsondata, strModelType);
    if (mt == MT_UNKNOWN) {
        return -1;
    }

    *handler = new axpi_model_handle_t;
    AxPiModelBase *model = (AxPiModelBase *)ObjectFactory::getInstance().getObjectByID(mt);
    if (model == nullptr) {
        axmpi_error("create model failed, model type:[%d]", mt);
        return -1;
    }

    ((axpi_model_handle_t *)(*handler))->model.reset(model);
    return ((axpi_model_handle_t *)(*handler))->model->init(&jsondata);
}

int axpi_exit(void **handler)
{
    if (handler && (axpi_model_handle_t *)(*handler) && ((axpi_model_handle_t *)(*handler))->model.get()) {
        ((axpi_model_handle_t *)(*handler))->model->exit();
        delete (axpi_model_handle_t *)(*handler);
        *handler = nullptr;
    }
}

int axpi_get_ivps_width_height(void *handler, std::string confJsonFile, size_t &width, size_t &height)
{
    if (!(axpi_model_handle_t *)(handler) || !((axpi_model_handle_t *)(handler))->model.get()) {
        return -1;
    }

    std::ifstream f(confJsonFile);
    if (f.fail()) {
        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);
    f.close();

    if (jsondata.contains("ivps_algo_width") && jsondata.contains("ivps_algo_height")) {
        width = jsondata["ivps_algo_width"];
        height = jsondata["ivps_algo_height"];
        ((axpi_model_handle_t *)handler)->model->setDetRestoreResolution(width, height);
    } else {
        switch (((axpi_model_handle_t *)handler)->model->getModelType()) {
            case MT_MLM_HUMAN_POSE_AXPPL:
            case MT_MLM_HUMAN_POSE_HRNET:
            case MT_MLM_ANIMAL_POSE_HRNET:
            case MT_MLM_HAND_POSE:
            case MT_MLM_FACE_RECOGNITION:
            case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
                width = 960;
                height = 540;
                ((axpi_model_handle_t *)handler)->model->setDetRestoreResolution(width, height);
                break;

            default:
                width = ((axpi_model_handle_t *)handler)->model->getAlgoWidth();
                height = ((axpi_model_handle_t *)handler)->model->getAlgoHeight();
                break;
        }
    }

    return 0;
}

int axpi_get_color_space(void *handler)
{
    if (!(axpi_model_handle_t *)(handler) || !((axpi_model_handle_t *)(handler))->model.get()) {
        return AXPI_COLOR_SPACE_UNK;
    }

    return ((axpi_model_handle_t *)handler)->model->getColorSpace();
}

int axpi_get_model_type(void *handler)
{
    if (!(axpi_model_handle_t *)(handler) || !((axpi_model_handle_t *)(handler))->model.get()) {
        return -1;
    }

    return ((axpi_model_handle_t *)handler)->model->getModelType();
}

int axpi_inference(void *handler, axpi_image_t *pstFrame, axpi_results_t *lastResults)
{
    if (!(axpi_model_handle_t *)(handler) || !((axpi_model_handle_t *)(handler))->model.get()) {
        return -1;
    }

    std::lock_guard<std::mutex> locker(((axpi_model_handle_t *)handler)->locker);
    lastResults->modelType = ((axpi_model_handle_t *)handler)->model->getModelType();
    int ret = ((axpi_model_handle_t *)handler)->model->inference(pstFrame, nullptr, lastResults);
    if (ret) {
        return -1;
    }

    int width, height;
    ((axpi_model_handle_t *)handler)->model->getDetRestoreResolution(width, height);

    for (size_t i = 0; i < lastResults->objects.size(); i++) {
        lastResults->objects[i].bbox.x /= width;
        lastResults->objects[i].bbox.y /= height;
        lastResults->objects[i].bbox.w /= width;
        lastResults->objects[i].bbox.h /= height;

        for (size_t j = 0; j < lastResults->objects[i].landmark.size(); j++) {
            lastResults->objects[i].landmark[j].x /= width;
            lastResults->objects[i].landmark[j].y /= height;
        }

        if (lastResults->objects[i].bHasBoxVertices) {
            for (size_t j = 0; j < 4; j++) {
                lastResults->objects[i].bbox_vertices[j].x /= width;
                lastResults->objects[i].bbox_vertices[j].y /= height;
            }
        }
    }

    for (size_t i = 0; i < lastResults->crowdCountPts.size(); i++) {
        lastResults->crowdCountPts[i].x /= width;
        lastResults->crowdCountPts[i].y /= height;
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

        lastResults->inFps = fps;
    }

    return 0;
}

int axpi_draw_results(void *handler, axpi_canvas_t *canvas, axpi_results_t *lastResults, float fontscale, int thickness, int offsetX, int offsetY)
{
    if (!(axpi_model_handle_t *)(handler) || !((axpi_model_handle_t *)(handler))->model.get()) {
        return -1;
    }

    cv::Mat image(canvas->height, canvas->width, CV_8UC4, canvas->data);
    ((axpi_model_handle_t *)handler)->model->draw_results(image, lastResults, fontscale, thickness, offsetX, offsetY);

    return 0;
}
}
