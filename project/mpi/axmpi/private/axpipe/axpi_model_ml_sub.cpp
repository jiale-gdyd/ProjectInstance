#include "../base/pose.hpp"
#include "../utilities/log.hpp"
#include "axpi_model_ml_sub.hpp"
#include "../utilities/matPixelAffine.hpp"

namespace axpi {
int AxPiModelPoseHrnetSub::preprocess(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    int ret;
    axpi_object_t &HumObj = results->objects[mCurrentIdx];

    if ((HumObj.bbox.w > 0) && (HumObj.bbox.h > 0)) {
        if (!mDstFrame.vir) {
            mDstFrame.dtype = pstFrame->dtype;
            mDstFrame.width = getAlgoWidth();
            mDstFrame.height = getAlgoHeight();
            mDstFrame.strideW = mDstFrame.width;

            if (mDstFrame.dtype == AXPI_COLOR_SPACE_NV12) {
                mDstFrame.size = mDstFrame.height * mDstFrame.width * 3 / 2;
            } else if ((mDstFrame.dtype == AXPI_COLOR_SPACE_RGB) || (mDstFrame.dtype == AXPI_COLOR_SPACE_BGR)) {
                mDstFrame.dtype = AXPI_COLOR_SPACE_BGR;
                mDstFrame.size = mDstFrame.height * mDstFrame.width * 3;
            } else {
                axmpi_error("just only support nv12/rgb/bgr format");
                return -1;
            }

            axpi_memalloc(&mDstFrame.phy, (void **)&mDstFrame.vir, mDstFrame.size, 0x100, NULL);
            mMalloc = true;
        }

        if (use_warp_preprocess) {
            cv::Point2f src_pts[4];

            if ((HumObj.bbox.w / HumObj.bbox.h) > (float(getAlgoWidth()) / float(getAlgoHeight()))) {
                float offset = ((HumObj.bbox.w * (float(getAlgoHeight()) / float(getAlgoWidth()))) - HumObj.bbox.h) / 2;

                src_pts[0] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y - offset);
                src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y - offset);
                src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y + HumObj.bbox.h + offset);
                src_pts[3] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y + HumObj.bbox.h + offset);
            } else {
                float offset = ((HumObj.bbox.h * (float(getAlgoWidth()) / float(getAlgoHeight()))) - HumObj.bbox.w) / 2;

                src_pts[0] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y);
                src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y);
                src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y + HumObj.bbox.h);
                src_pts[3] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y + HumObj.bbox.h);
            }

            cv::Point2f dst_pts[4];
            dst_pts[0] = cv::Point2f(0, 0);
            dst_pts[1] = cv::Point2f(getAlgoWidth(), 0);
            dst_pts[2] = cv::Point2f(getAlgoWidth(), getAlgoHeight());
            dst_pts[3] = cv::Point2f(0, getAlgoHeight());

            affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
            affine_trans_mat_inv;
            cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

            float mat3x3[3][3] = {
                {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
                {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
                {0, 0, 1}
            };

            ret = axpi_imgproc_warp(pstFrame, &mDstFrame, &mat3x3[0][0], 128);
            if (ret != 0) {
                return ret;
            }
        } else {
            ret = axpi_imgproc_crop_resize(pstFrame, &mDstFrame, &HumObj.bbox);
            if (ret != 0) {
                axmpi_error("crop resize failed, box:[%4.2f %4.2f %4.2f %4.2f] image:[%dx%d]", HumObj.bbox.x, HumObj.bbox.y, HumObj.bbox.w, HumObj.bbox.h, pstFrame->width, pstFrame->height);
                return ret;
            }
        }
    } else {
        return -1;
    }

    return 0;
}

int AxPiModelPoseHrnetSub::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT);
    }

    pose::ai_body_parts_s ai_point_result;
    axpi_object_t &HumObj = results->objects[mCurrentIdx];

    auto ptr = (float *)mRunner->getSpecOutput(0).pVirAddr;
    pose::hrnet_post_process(ptr, ai_point_result, BODY_LMK_SIZE, getAlgoHeight(), getAlgoWidth());

    std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
    points.resize(BODY_LMK_SIZE);
    // results->objects[mCurrentIdx].landmark = points;

    std::vector<axpi_point_t> landmark;
    if (use_warp_preprocess) {
        for (size_t i = 0; i < BODY_LMK_SIZE; i++) {
            axpi_point_t point;

            point.x = ai_point_result.keypoints[i].x;
            point.y = ai_point_result.keypoints[i].y;

            int x = affine_trans_mat_inv.at<double>(0, 0) * point.x + affine_trans_mat_inv.at<double>(0, 1) * point.y + affine_trans_mat_inv.at<double>(0, 2);
            int y = affine_trans_mat_inv.at<double>(1, 0) * point.x + affine_trans_mat_inv.at<double>(1, 1) * point.y + affine_trans_mat_inv.at<double>(1, 2);

            point.x = x;
            point.y = y;
            landmark.push_back(point);
        }
    } else {
        for (size_t i = 0; i < BODY_LMK_SIZE; i++) {
            axpi_point_t point;
            point.x = ai_point_result.keypoints[i].x / getAlgoWidth() * HumObj.bbox.w + HumObj.bbox.x;
            point.y = ai_point_result.keypoints[i].y / getAlgoHeight() * HumObj.bbox.h + HumObj.bbox.y;
            landmark.push_back(point);
        }
    }

    results->objects[mCurrentIdx].landmark = landmark;
    return 0;
}

int AxPiModelPoseAxpplSub::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT);
    }

    pose::ai_body_parts_s ai_point_result;
    axpi_object_t &HumObj = results->objects[mCurrentIdx];

    auto ptr = (float *)mRunner->getSpecOutput(0).pVirAddr;
    auto ptr_index = (float *)mRunner->getSpecOutput(1).pVirAddr;

    pose::ppl_pose_post_process(ptr, ptr_index, ai_point_result, BODY_LMK_SIZE);

    std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
    points.resize(BODY_LMK_SIZE);
    // results->objects[mCurrentIdx].landmark = points;

    std::vector<axpi_point_t> landmark;
    if (use_warp_preprocess) {
        for (size_t i = 0; i < BODY_LMK_SIZE; i++) {
            axpi_point_t point;

            point.x = ai_point_result.keypoints[i].x;
            point.y = ai_point_result.keypoints[i].y;
            int x = affine_trans_mat_inv.at<double>(0, 0) * point.x + affine_trans_mat_inv.at<double>(0, 1) * point.y + affine_trans_mat_inv.at<double>(0, 2);
            int y = affine_trans_mat_inv.at<double>(1, 0) * point.x + affine_trans_mat_inv.at<double>(1, 1) * point.y + affine_trans_mat_inv.at<double>(1, 2);
            results->objects[mCurrentIdx].landmark[i].x = x;
            results->objects[mCurrentIdx].landmark[i].y = y;

            landmark.push_back(point);
        }
    } else {
        for (size_t i = 0; i < BODY_LMK_SIZE; i++) {
            axpi_point_t point;
            point.x = ai_point_result.keypoints[i].x / getAlgoWidth() * HumObj.bbox.w + HumObj.bbox.x;
            point.y = ai_point_result.keypoints[i].y / getAlgoHeight() * HumObj.bbox.h + HumObj.bbox.y;
            landmark.push_back(point);
        }
    }

    results->objects[mCurrentIdx].landmark = landmark;
    return 0;
}

int AxPiModelPoseHrnetAnimalSub::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT);
    }

    pose::ai_body_parts_s ai_point_result;
    axpi_object_t &HumObj = results->objects[mCurrentIdx];

    auto ptr = (float *)mRunner->getSpecOutput(0).pVirAddr;
    pose::hrnet_post_process(ptr, ai_point_result, ANIMAL_LMK_SIZE, getAlgoHeight(), getAlgoWidth());

    std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
    points.resize(ANIMAL_LMK_SIZE);
    // results->objects[mCurrentIdx].landmark = points;

    std::vector<axpi_point_t> landmark;
    if (use_warp_preprocess) {
        for (size_t i = 0; i < ANIMAL_LMK_SIZE; i++) {
            axpi_point_t point;

            point.x = ai_point_result.keypoints[i].x;
            point.y = ai_point_result.keypoints[i].y;
            int x = affine_trans_mat_inv.at<double>(0, 0) * point.x + affine_trans_mat_inv.at<double>(0, 1) * point.y + affine_trans_mat_inv.at<double>(0, 2);
            int y = affine_trans_mat_inv.at<double>(1, 0) * point.x + affine_trans_mat_inv.at<double>(1, 1) * point.y + affine_trans_mat_inv.at<double>(1, 2);
            results->objects[mCurrentIdx].landmark[i].x = x;
            results->objects[mCurrentIdx].landmark[i].y = y;

            landmark.push_back(point);
        }
    } else {
        for (size_t i = 0; i < ANIMAL_LMK_SIZE; i++) {
            axpi_point_t point;
            point.x = ai_point_result.keypoints[i].x / getAlgoWidth() * HumObj.bbox.w + HumObj.bbox.x;
            point.y = ai_point_result.keypoints[i].y / getAlgoHeight() * HumObj.bbox.h + HumObj.bbox.y;
            landmark.push_back(point);
        }
    }

    results->objects[mCurrentIdx].landmark = landmark;
    return 0;
}

int AxPiModelPoseHandSub::preprocess(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (!mDstFrame.vir) {
        mDstFrame.dtype = pstFrame->dtype;
        mDstFrame.height = getAlgoHeight();
        mDstFrame.width = getAlgoWidth();
        mDstFrame.strideW = mDstFrame.width;
    
        if (mDstFrame.dtype == AXPI_COLOR_SPACE_NV12) {
            mDstFrame.size = mDstFrame.height * mDstFrame.width * 3 / 2;
        } else if ((mDstFrame.dtype == AXPI_COLOR_SPACE_RGB) || (mDstFrame.dtype == AXPI_COLOR_SPACE_BGR)) {
            mDstFrame.dtype = AXPI_COLOR_SPACE_BGR;
            mDstFrame.size = mDstFrame.height * mDstFrame.width * 3;
        } else {
            axmpi_error("just only support nv12/rgb/bgr format");
            return -1;
        }

        axpi_memalloc(&mDstFrame.phy, (void **)&mDstFrame.vir, mDstFrame.size, 0x100, NULL);
        mMalloc = true;
    }

    axpi_object_t &object = results->objects[mCurrentIdx];

    cv::Point2f src_pts[4];
    src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
    src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
    src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
    src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

    cv::Point2f dst_pts[4];
    dst_pts[0] = cv::Point2f(0, 0);
    dst_pts[1] = cv::Point2f(getAlgoWidth(), 0);
    dst_pts[2] = cv::Point2f(getAlgoWidth(), getAlgoHeight());
    dst_pts[3] = cv::Point2f(0, getAlgoHeight());

    affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
    affine_trans_mat_inv;
    cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

    float mat3x3[3][3] = {
        {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
        {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
        {0, 0, 1}
    };

    int ret = axpi_imgproc_warp(pstFrame, &mDstFrame, &mat3x3[0][0], 128);
    if (ret) {
        return ret;
    }

    return 0;
}

int AxPiModelPoseHandSub::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0) {
        mSimpleRingBuffer.resize(RINGBUFFER_CACHE_COUNT * MAX_HAND_BBOX_COUNT);
    }

    auto &info_point = mRunner->getSpecOutput(0);
    auto &info_score = mRunner->getSpecOutput(1);
    auto point_ptr = (float *)info_point.pVirAddr;
    auto score_ptr = (float *)info_score.pVirAddr;

    pose::ai_hand_parts_s ai_hand_point_result;
    pose::post_process_hand(point_ptr, score_ptr, ai_hand_point_result, HAND_LMK_SIZE, getAlgoHeight(), getAlgoWidth());

    std::vector<axpi_point_t> &points = mSimpleRingBuffer.next();
    points.resize(HAND_LMK_SIZE);
    // results->objects[mCurrentIdx].landmark = points;

    std::vector<axpi_point_t> landmark;
    for (size_t i = 0; i < HAND_LMK_SIZE; i++) {
        axpi_point_t point;

        point.x = ai_hand_point_result.keypoints[i].x;
        point.y = ai_hand_point_result.keypoints[i].y;
        int x = affine_trans_mat_inv.at<double>(0, 0) * point.x + affine_trans_mat_inv.at<double>(0, 1) * point.y + affine_trans_mat_inv.at<double>(0, 2);
        int y = affine_trans_mat_inv.at<double>(1, 0) * point.x + affine_trans_mat_inv.at<double>(1, 1) * point.y + affine_trans_mat_inv.at<double>(1, 2);
        point.x = x;
        point.y = y;

        landmark.push_back(point);
    }
    results->objects[mCurrentIdx].landmark = landmark;

    return 0;
}

void AxPiModelFaceFeatExtractorSub::_normalize(float *feature, int feature_len)
{
    float sum = 0;

    for (int it = 0; it < feature_len; it++) {
        sum += feature[it] * feature[it];
    }

    sum = sqrt(sum);
    for (int it = 0; it < feature_len; it++) {
        feature[it] /= sum;
    }
}

int AxPiModelFaceFeatExtractorSub::preprocess(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (!mDstFrame.vir) {
        mDstFrame.width = mDstFrame.height = mDstFrame.strideW = 112;
        axpi_memalloc(&mDstFrame.phy, (void **)&mDstFrame.vir, 112 * 112 * 3, 0x100, "SAMPLE-CV");
        mMalloc = true;
    }

    axpi_object_t &obj = results->objects[mCurrentIdx];
    axpi_imgproc_align_face(&obj, pstFrame, &mDstFrame);

    return 0;
}

int AxPiModelFaceFeatExtractorSub::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (mSimpleRingBuffer_FaceFeat.size() == 0) {
        mSimpleRingBuffer_FaceFeat.resize(RINGBUFFER_CACHE_COUNT * MAX_BBOX_COUNT);
    }

    auto &feat = mSimpleRingBuffer_FaceFeat.next();
    feat.resize(FACE_FEAT_LEN);
    memcpy(feat.data(), mRunner->getSpecOutput(0).pVirAddr, FACE_FEAT_LEN * sizeof(float));
    _normalize(feat.data(), FACE_FEAT_LEN);

    results->objects[mCurrentIdx].faceFeat.w = FACE_FEAT_LEN * 4;
    results->objects[mCurrentIdx].faceFeat.h = 1;
    results->objects[mCurrentIdx].faceFeat.data = (unsigned char *)feat.data();

    return 0;
}

int AxPiModelLicensePlateRecognitionSub::preprocess(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    if (!mDstFrame.vir) {
        mDstFrame.dtype = pstFrame->dtype;
        mDstFrame.height = getAlgoHeight();
        mDstFrame.width = getAlgoWidth();
        mDstFrame.strideW = mDstFrame.width;

        if (mDstFrame.dtype == AXPI_COLOR_SPACE_NV12) {
            mDstFrame.size = mDstFrame.height * mDstFrame.width * 3 / 2;
        } else if ((mDstFrame.dtype == AXPI_COLOR_SPACE_RGB) || (mDstFrame.dtype == AXPI_COLOR_SPACE_BGR)) {
            mDstFrame.dtype = AXPI_COLOR_SPACE_BGR;
            mDstFrame.size = mDstFrame.height * mDstFrame.width * 3;
        } else {
            axmpi_error("just only support nv12/rgb/bgr format");
            return -1;
        }

        axpi_memalloc(&mDstFrame.phy, (void **)&mDstFrame.vir, mDstFrame.size, 0x100, NULL);
        mMalloc = true;
    }

    cv::Point2f src_pts[4];
    axpi_object_t &object = results->objects[mCurrentIdx];

    src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
    src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
    src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
    src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

    cv::Point2f dst_pts[4];
    dst_pts[0] = cv::Point2f(0, 0);
    dst_pts[1] = cv::Point2f(getAlgoWidth(), 0);
    dst_pts[2] = cv::Point2f(getAlgoWidth(), getAlgoHeight());
    dst_pts[3] = cv::Point2f(0, getAlgoHeight());

    affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
    affine_trans_mat_inv;
    cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

    float mat3x3[3][3] = {
        {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
        {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
        {0, 0, 1}
    };

    int ret = axpi_imgproc_warp(pstFrame, &mDstFrame, &mat3x3[0][0], 128);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

int AxPiModelLicensePlateRecognitionSub::post_process(axpi_image_t *pstFrame, axpi_bbox_t *crop_resize_box, axpi_results_t *results)
{
    static const std::vector<std::string> plate_string = {
        // "#", "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑", "苏", "浙", "皖",
        // "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂", "琼", "川", "贵", "云", "藏", "陕",
        // "甘", "青", "宁", "新", "学", "警", "港", "澳", "挂", "使", "领", "民", "航", "深",
        "#", "beijing", "shanghai", "tianjin", "chongqing", "hebei", "shan1xi", "neimenggu", "liaoning", "jilin", "heilongjiang", "jiangsu", "zhejiang", "anhui",
        "fujian", "jiangxi", "shandong", "henan", "hubei", "hunan", "guangdong", "guangxi", "hainan", "sichuan", "guizhou", "yunnan", "xizang", "shan3xi",
        "gansu", "qinghai", "ningxia", "xinjiang", "jiaolian", "jingcha", "xianggang", "aomen", "gua", "shi", "ling", "ming", "hang", "shen",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "A", "B", "C", "D", "E", "F", "G", "H",
        "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

    float *outputdata = (float *)mRunner->getSpecOutput(0).pVirAddr;

    for (int row = 0; row < 21; row++) {
        argmax_data[row] = outputdata[0];
        argmax_idx[row] = 0;
        for (int col = 0; col < 78; col++) {
            if (outputdata[0] > argmax_data[row]) {
                argmax_data[row] = outputdata[0];
                argmax_idx[row] = col;
            }

            outputdata += 1;
        }
    }

    std::string plate = "";
    std::string pre_str = "#";

    for (int i = 0; i < 21; i++) {
        int index = argmax_idx[i];
        if ((plate_string[index] != "#") && (plate_string[index] != pre_str)) {
            plate += plate_string[index];
        }

        pre_str = plate_string[index];
    }

    results->objects[mCurrentIdx].objname = plate;
    return 0;
}
}
