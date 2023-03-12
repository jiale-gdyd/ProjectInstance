#include <fstream>
#include <nlohmann/json.hpp>

#include "axpi_model_seg.hpp"
#include "axpi_common_api.hpp"
#include "axpi_model_base.hpp"
#include "axpi_model_ml_sub.hpp"
#include "axpi_model_detect.hpp"
#include "axpi_model_crowdcount.hpp"
#include "axpi_model_runner_ax620.hpp"
#include "axpi_model_multi_level_model.hpp"

namespace axpi {
#ifndef MIN
#define MIN(a, b)       ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b)       ((a) < (b) ? (b) : (a))
#endif

std::map<std::string, int> g_modelTypeTable = {
    {"MT_UNKNOWN", MT_UNKNOWN},
};

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, T *val)
{
    if (jsondata.contains(key)) {
        *val = jsondata[key];
    }
}

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, std::vector<T> *val)
{
    if (jsondata.contains(key)) {
        std::vector<T> tmp = jsondata[key];
        *val = tmp;
    }
}

int AxPiModelBase::getModelType(void *jsonData, std::string &modelType)
{
    int mModelType = MT_UNKNOWN;
    auto jsondata = *(nlohmann::json *)jsonData;

    if (jsondata.contains("model_type")) {
        if (jsondata["model_type"].is_number_integer()) {
            int mt = -1;
            mt = jsondata["model_type"];
            auto it = g_modelTypeTable.begin();
            for (size_t i = 0; i < g_modelTypeTable.size(); i++) {
                if (it->second == mt) {
                    mModelType = mt;
                }
            }
        } else if (jsondata["model_type"].is_string()) {
            modelType = jsondata["model_type"];

            auto item = g_modelTypeTable.find(modelType);
            if (item != g_modelTypeTable.end()) {
                mModelType = g_modelTypeTable[modelType];
            }
        }
    }

    return mModelType;
}

int AxPiModelBase::getRunnerType(void *jsonData, std::string &strRunnerType)
{
    int mRunnerType = RUNNER_UNKNOWN;
    auto jsondata = *(nlohmann::json *)jsonData;

    if (jsondata.contains("runner_type")) {
        if (jsondata["runner_type"].is_number_integer()) {
            int mt = -1;
            mt = jsondata["runner_type"];
            auto it = g_modelTypeTable.begin();

            for (size_t i = 0; i < g_modelTypeTable.size(); i++) {
                if (it->second == mt) {
                    mRunnerType = mt;
                }
            }
        } else if (jsondata["runner_type"].is_string()) {
            strRunnerType = jsondata["runner_type"];
            auto item = g_modelTypeTable.find(strRunnerType);
            if (item != g_modelTypeTable.end()) {
                mRunnerType = g_modelTypeTable[strRunnerType];
            }
        }
    }

    return mRunnerType;
}

void AxPiModelBase::drawBbox(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    int x, y;
    int baseLine = 0;
    cv::Size label_size;

    for (size_t i = 0; i < results->objects.size(); i++) {
        cv::Rect rect(results->objects[i].bbox.x * image.cols + offset_x, results->objects[i].bbox.y * image.rows + offset_y,
            results->objects[i].bbox.w * image.cols, results->objects[i].bbox.h * image.rows);

        label_size = cv::getTextSize(results->objects[i].objname, cv::FONT_HERSHEY_SIMPLEX, fontscale, thickness, &baseLine);
        if (results->objects[i].bHasBoxVertices) {
            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[0].x * image.cols + offset_x, results->objects[i].bbox_vertices[0].y * image.rows + offset_y),
                cv::Point(results->objects[i].bbox_vertices[1].x * image.cols + offset_x, results->objects[i].bbox_vertices[1].y * image.rows + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[1].x * image.cols + offset_x, results->objects[i].bbox_vertices[1].y * image.rows + offset_y),
                cv::Point(results->objects[i].bbox_vertices[2].x * image.cols + offset_x, results->objects[i].bbox_vertices[2].y * image.rows + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[2].x * image.cols + offset_x, results->objects[i].bbox_vertices[2].y * image.rows + offset_y),
                cv::Point(results->objects[i].bbox_vertices[3].x * image.cols + offset_x, results->objects[i].bbox_vertices[3].y * image.rows + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            cv::line(image,
                cv::Point(results->objects[i].bbox_vertices[3].x * image.cols + offset_x, results->objects[i].bbox_vertices[3].y * image.rows + offset_y),
                cv::Point(results->objects[i].bbox_vertices[0].x * image.cols + offset_x, results->objects[i].bbox_vertices[0].y * image.rows + offset_y),
                cv::Scalar(128, 0, 0, 255), thickness * 2, 8, 0);

            x = results->objects[i].bbox_vertices[0].x * image.cols + offset_x;
            y = results->objects[i].bbox_vertices[0].y * image.rows + offset_y - label_size.height - baseLine;
        } else {
            cv::rectangle(image, rect, mCocoColors[results->objects[i].label % mCocoColors.size()], thickness);
            x = rect.x;
            y = rect.y - label_size.height - baseLine;
        }

        if (y < 0) {
            y = 0;
        }

        if ((x + label_size.width) > image.cols) {
            x = image.cols - label_size.width;
        }

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)), cv::Scalar(255, 255, 255, 255), -1);
        cv::putText(image, results->objects[i].objname, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale, cv::Scalar(0, 0, 0, 255), thickness);
    }
}

void AxPiModelBase::drawFps(cv::Mat &image, axpi_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    mFpsInfo = "fps:" + std::to_string(results->inFps);

    cv::Size label_size = cv::getTextSize(mFpsInfo, cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, thickness * 2, NULL);
    cv::putText(image, mFpsInfo, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontscale * 1.5, cv::Scalar(255, 0, 255, 0), thickness * 2);
}

int AxPiModelSingleBase::init(void *jsonData)
{
    auto jsondata = *(nlohmann::json *)jsonData;

    update_val(jsondata, "prob_threshold", &mProbThreshold);
    update_val(jsondata, "nms_threshold", &mNmsThreshold);
    update_val(jsondata, "class_count", &mClassCount);
    update_val(jsondata, "anchors", &mAnchors);
    update_val(jsondata, "class_names", &mClassName);
    update_val(jsondata, "model_path", &mModelPath);
    update_val(jsondata, "strides", &mStrides);

    std::string strModelType;
    mModelType = getModelType(&jsondata, strModelType);

    mRunner.reset((AxPiRunnerBase *)ObjectFactory::getInstance().getObjectByID(mRunnerType));
    if (!mRunner.get()) {
        axmpi_error("runner instantiate failed");
        return -1;
    }

    int ret = mRunner->init(mModelPath);
    if (ret) {
        axmpi_error("runner init load model failed");
        return ret;
    }

    int unknown_cls_count = MAX(0, mClassCount - mClassName.size());
    for (int i = 0; i < unknown_cls_count; i++) {
        mClassName.push_back("unknown");
    }

    return 0;
}

int AxPiModelSingleBase::exit()
{
    mRunner->exit();
    if (mMalloc) {
        axpi_memfree(mDstFrame.phy, mDstFrame.vir);
    }

    return 0;
}

int AxPiModelSingleBase::preprocess(axpi_image_t *srcFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    memcpy(&mDstFrame, srcFrame, sizeof(axpi_image_t));
    mMalloc = false;
    return 0;
}

int AxPiModelSingleBase::inference(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    int ret = preprocess(pstFrame, crop_resize_box, results);
    if (ret != 0) {
        axmpi_error("preprocess failed, return:[%d]", ret);
        return ret;
    }

    ret = mRunner->inference(&mDstFrame, crop_resize_box);
    if (ret != 0) {
        axmpi_error("inference failed, return:[%d]", ret);
        return ret;
    }

    ret = post_process(pstFrame, crop_resize_box, results);
    return ret;
}

int AxPiModelMultiBase::init(void *jsonData)
{
    auto jsondata = *(nlohmann::json *)jsonData;

    std::string strModelType;
    mModelType = getModelType(&jsondata, strModelType);

    switch (mModelType) {
        case MT_MLM_HUMAN_POSE_AXPPL:
            mModel1.reset(new AxPiModelPoseAxpplSub);
            break;

        case MT_MLM_HUMAN_POSE_HRNET:
            mModel1.reset(new AxPiModelPoseHrnetSub);
            break;

        case MT_MLM_ANIMAL_POSE_HRNET:
            mModel1.reset(new AxPiModelPoseHrnetAnimalSub);
            break;

        case MT_MLM_HAND_POSE:
            mModel1.reset(new AxPiModelPoseHandSub);
            break;

        case MT_MLM_FACE_RECOGNITION:
            mModel1.reset(new AxPiModelFaceFeatExtractorSub);
            break;

        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            mModel1.reset(new AxPiModelLicensePlateRecognitionSub);
            break;

        default:
            axmpi_error("not multi level model type:[%d]", mModelType);
            return -1;
    }

    if (jsondata.contains("model_major") && jsondata.contains("model_minor")) {
        nlohmann::json json_major = jsondata["model_major"];

        std::string strModelType;
        int mt = getModelType(&json_major, strModelType);
        mModel0.reset((AxPiModelBase *)ObjectFactory::getInstance().getObjectByID(mt));
        mModel0->init((void *)&json_major);

        nlohmann::json json_minor = jsondata["model_minor"];
        update_val(json_minor, "class_id", &mClassIds);

        if (json_minor.contains("face_database")) {
            nlohmann::json database = json_minor["face_database"];
            for (nlohmann::json::iterator it = database.begin(); it != database.end(); ++it) {
                AxPiModelFaceId faceid;
                faceid.path = it.value();
                faceid.name = it.key();
                mFaceRegisterIds.push_back(faceid);
            }
        }

        update_val(json_minor, "face_recognition_threshold", &mFaceRecThreshold);
        mModel1->init((void *)&json_minor);
    }else {
        return -1;
    }

    return 0;
}

int AxPiModelMultiBase::exit()
{
    mModel1->exit();
    mModel0->exit();
    return 0;
}
}
