#include <fstream>
#include <ax_sys_api.h>
#include <ax_npu_imgproc.h>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include "post_process.hpp"
#include "../utilities/log.hpp"
#include "post_process_pose.hpp"
#include "post_process_detect.hpp"
#include "../utilities/ringbuffer.hpp"
#include "../utilities/multikeymap.hpp"
#include "../utilities/matPixelAffine.hpp"

namespace axpi {
typedef struct {
    std::string        name;
    std::string        path;
    std::vector<float> feat;
} axjoint_faceid_t;

float face_recgnition_threahold = 0.4f;
std::vector<axjoint_faceid_t> face_ids;

typedef int (*inference_func_t)(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results);

static int _axjoint_inference_detetect(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    int ret = axjoint_inference(handler->majorModel.jointHandle, pstFrame, NULL);
    axjoint_post_process_detect_single_func(results, handler);
    return ret;
}

static int _axjoint_inference_pphumseg(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    int ret = axjoint_inference(handler->majorModel.jointHandle, pstFrame, NULL);
    results->bPPHumSeg = true;

    auto ptr = (float *)handler->majorModel.jointAttr.outputs[0].pVirAddr;
    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(RINGBUFFER_CACHE_COUNT);

    int seg_h = handler->majorModel.jointAttr.outInfo->pShape[2];
    int seg_w = handler->majorModel.jointAttr.outInfo->pShape[3];
    int seg_size = seg_h * seg_w;

    cv::Mat &seg_mat = mSimpleRingBuffer.next();
    if (seg_mat.empty()) {
        seg_mat = cv::Mat(seg_h, seg_w, CV_8UC1);
    }

    results->PPHumSeg.h = seg_h;
    results->PPHumSeg.w = seg_w;
    results->PPHumSeg.data = seg_mat.data;

    for (int j = 0; j < seg_h * seg_w; ++j) {
        results->PPHumSeg.data[j] = (ptr[j] < ptr[j + seg_size]) ? 255 : 0;
    }

    return ret;
}

static int _axjoint_inference_human_pose(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    int ret = axjoint_inference(handler->majorModel.jointHandle, pstFrame, NULL);
    axjoint_post_process_detect_single_func(results, handler);

    int idx = -1;
    axpi_object_t HumObj = {0};
    AX_BOOL bHasHuman = AX_FALSE;

    for (size_t i = 0; i < results->objects.size(); i++) {
        if (results->objects[i].label == handler->minorModelClassIds[0]) {
            memcpy(&HumObj, &results->objects[i], sizeof(axpi_object_t));
            bHasHuman = AX_TRUE;
            idx = i;
            break;
        }
    }

    if ((bHasHuman == AX_TRUE) && handler->minorModel.jointHandle && (HumObj.bbox.w > 0) && (HumObj.bbox.h > 0)) {
        static AX_NPU_CV_Image tmp = {0};
        if (!tmp.pVir) {
            tmp.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
            tmp.nHeight = handler->minorModel.jointAttr.height;
            tmp.nWidth = handler->minorModel.jointAttr.width;
            tmp.tStride.nW = tmp.nWidth;

            if (tmp.eDtype == AX_NPU_CV_FDT_NV12) {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3 / 2;
            } else if ((tmp.eDtype == AX_NPU_CV_FDT_RGB) || (tmp.eDtype == AX_NPU_CV_FDT_BGR)) {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3;
            } else {
                axmpi_error("just only support nv12/rgb/bgr format");
                return -1;
            }

            AX_SYS_MemAlloc(&tmp.pPhy, (void **)&tmp.pVir, tmp.nSize, 0x100, NULL);
        }

        cv::Point2f src_pts[4];

        if ((HumObj.bbox.w / HumObj.bbox.h) > (float(handler->minorModel.jointAttr.width) / float(handler->minorModel.jointAttr.height))) {
            float offset = ((HumObj.bbox.w * (float(handler->minorModel.jointAttr.height) / float(handler->minorModel.jointAttr.width))) - HumObj.bbox.h) / 2;

            src_pts[0] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y - offset);
            src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y - offset);
            src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y + HumObj.bbox.h + offset);
            src_pts[3] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y + HumObj.bbox.h + offset);
        } else {
            float offset = ((HumObj.bbox.h * (float(handler->minorModel.jointAttr.width) / float(handler->minorModel.jointAttr.height))) - HumObj.bbox.w) / 2;

            src_pts[0] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y);
            src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y);
            src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y + HumObj.bbox.h);
            src_pts[3] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y + HumObj.bbox.h);
        }

        cv::Point2f dst_pts[4];
        dst_pts[0] = cv::Point2f(0, 0);
        dst_pts[1] = cv::Point2f(handler->minorModel.jointAttr.width, 0);
        dst_pts[2] = cv::Point2f(handler->minorModel.jointAttr.width, handler->minorModel.jointAttr.height);
        dst_pts[3] = cv::Point2f(0, handler->minorModel.jointAttr.height);

        cv::Mat affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
        cv::Mat affine_trans_mat_inv;
        cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

        float mat3x3[3][3] = {
            {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
            {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
            {0, 0, 1}
        };

        ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &tmp, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);

        ret = axjoint_inference(handler->minorModel.jointHandle, &tmp, NULL);
        axjoint_post_process_pose(handler, &HumObj);

        results->objects.resize(1);
        memcpy(&results->objects[0], &HumObj, sizeof(axpi_object_t));

        std::vector<axpi_point_t> landmark;
        for (int j = 0; j < BODY_LMK_SIZE; j++) {
            axpi_point_t point;
            point.x = affine_trans_mat_inv.at<double>(0, 0) * results->objects[0].landmark[j].x + affine_trans_mat_inv.at<double>(0, 1) * results->objects[0].landmark[j].y + affine_trans_mat_inv.at<double>(0, 2);
            point.y = affine_trans_mat_inv.at<double>(1, 0) * results->objects[0].landmark[j].x + affine_trans_mat_inv.at<double>(1, 1) * results->objects[0].landmark[j].y + affine_trans_mat_inv.at<double>(1, 2);
            landmark.push_back(point);
        }

        results->objects[0].landmark = landmark;
    } else {
        results->objects.clear();
    }

    for (int i = 0; i < results->objects.size(); i++) {
        results->objects[i].bbox.x /= handler->restoreWidth;
        results->objects[i].bbox.y /= handler->restoreHeight;
        results->objects[i].bbox.w /= handler->restoreWidth;
        results->objects[i].bbox.h /= handler->restoreHeight;

        if (results->objects[i].landmark.size()) {
            for (int j = 0; j < MAX_LMK_SIZE; j++) {
                results->objects[idx].landmark[j].x /= handler->restoreWidth;
                results->objects[idx].landmark[j].y /= handler->restoreHeight;
            }
        }
    }

    return ret;
}

static int _axjoint_inference_animal_pose(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    int ret = axjoint_inference(handler->majorModel.jointHandle, pstFrame, NULL);
    axjoint_post_process_detect_single_func(results, handler);

    int idx = -1;
    axpi_object_t HumObj = {0};
    AX_BOOL bHasHuman = AX_FALSE;

    for (size_t i = 0; i < results->objects.size(); i++) {
        for (size_t j = 0; (j < handler->minorModelClassIds.size()) && j < CLASS_ID_COUNT; j++) {
            if (results->objects[i].label == handler->minorModelClassIds[j]) {
                if ((results->objects[i].bbox.w * results->objects[i].bbox.h) > (HumObj.bbox.w * HumObj.bbox.h)) {
                    memcpy(&HumObj, &results->objects[i], sizeof(axpi_object_t));
                    bHasHuman = AX_TRUE;
                    idx = i;
                    break;
                }
            }
        }
    }

    if ((bHasHuman == AX_TRUE) && handler->minorModel.jointHandle && (HumObj.bbox.w > 0) && (HumObj.bbox.h > 0)) {
        ret = axjoint_inference(handler->minorModel.jointHandle, pstFrame, &HumObj.bbox);
        axjoint_post_process_pose(handler, &HumObj);
        results->objects.resize(1);
        memcpy(&results->objects[0], &HumObj, sizeof(axpi_object_t));
    } else {
        results->objects.clear();
    }

    for (size_t i = 0; i < results->objects.size(); i++) {
        results->objects[i].bbox.x /= handler->restoreWidth;
        results->objects[i].bbox.y /= handler->restoreHeight;
        results->objects[i].bbox.w /= handler->restoreWidth;
        results->objects[i].bbox.h /= handler->restoreHeight;

        if (results->objects[i].landmark.size() > 0) {
            for (int j = 0; j < MAX_LMK_SIZE; j++) {
                results->objects[idx].landmark[j].x /= handler->restoreWidth;
                results->objects[idx].landmark[j].y /= handler->restoreHeight;
            }
        }
    }

    return ret;
}

static void _normalize(float *feature, int feature_len)
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

static double _calcSimilar(float *feature1, float *feature2, int feature_len)
{
    double sim = 0.0;

    for (int i = 0; i < feature_len; i++) {
        sim += feature1[i] * feature2[i];
    }

    sim = sim < 0 ? 0 : sim > 1 ? 1 : sim;
    return sim;
}

void align_face(axpi_object_t &obj, AX_NPU_CV_Image *npu_image, AX_NPU_CV_Image &npu_image_face_align)
{
    static float target[10] = {
        38.2946, 51.6963,
        73.5318, 51.5014,
        56.0252, 71.7366,
        41.5493, 92.3655,
        70.7299, 92.2041
    };

    float _tmp[10] = {
        obj.landmark[0].x, obj.landmark[0].y,
        obj.landmark[1].x, obj.landmark[1].y,
        obj.landmark[2].x, obj.landmark[2].y,
        obj.landmark[3].x, obj.landmark[3].y,
        obj.landmark[4].x, obj.landmark[4].y
    };

    float _m[6], _m_inv[6];
    get_affine_transform(_tmp, target, 5, _m);
    invert_affine_transform(_m, _m_inv);

    float mat3x3[3][3] = {
        {_m_inv[0], _m_inv[1], _m_inv[2]},
        {_m_inv[3], _m_inv[4], _m_inv[5]},
        {0, 0, 1}
    };

    npu_image_face_align.eDtype = npu_image->eDtype;
    if ((npu_image_face_align.eDtype == AX_NPU_CV_FDT_RGB) || (npu_image_face_align.eDtype == AX_NPU_CV_FDT_BGR)) {
        npu_image_face_align.nSize = 112 * 112 * 3;
    } else if ((npu_image_face_align.eDtype == AX_NPU_CV_FDT_NV12) || (npu_image_face_align.eDtype == AX_NPU_CV_FDT_NV21)) {
        npu_image_face_align.nSize = 112 * 112 * 1.5;
    } else {
        axmpi_error("just only support BGR/RGB/NV12 format");
    }

    AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, npu_image, &npu_image_face_align, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);
}

static int _axjoint_inference_face_recognition(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    static bool b_face_database_init = false;
    static AX_NPU_CV_Image npu_image_face_align;

    if (!b_face_database_init) {
        npu_image_face_align.nWidth = npu_image_face_align.nHeight = npu_image_face_align.tStride.nW = 112;
        AX_SYS_MemAlloc((AX_U64 *)&npu_image_face_align.pPhy, (void **)&npu_image_face_align.pVir, 112 * 112 * 3, 0x100, (const AX_S8 *)"SAMPLE-CV");

        for (size_t i = 0; i < face_ids.size(); i++) {
            auto &faceid = face_ids[i];
            cv::Mat image = cv::imread(faceid.path);
            if (image.empty()) {
                axmpi_error("image:[%s] cannot open, name:[%s] register failed", faceid.path.c_str(), faceid.name.c_str());
                continue;
            }

            AX_NPU_CV_Image npu_image;
            npu_image.eDtype = AX_NPU_CV_FDT_RGB;
            npu_image.nHeight = image.rows;
            npu_image.nWidth = image.cols;
            npu_image.tStride.nW = npu_image.nWidth;
            npu_image.nSize = npu_image.nWidth * npu_image.nHeight * 3;
            AX_SYS_MemAlloc((AX_U64 *)&npu_image.pPhy, (void **)&npu_image.pVir, npu_image.nSize, 0x100, (AX_S8 *)"SAMPLE-CV");
            memcpy(npu_image.pVir, image.data, npu_image.nSize);

            int ret = axjoint_inference(handler->majorModel.jointHandle, &npu_image, NULL);
            axpi_results_t Results = {0};

            int tmp_width = handler->restoreWidth;
            int tmp_height = handler->restoreHeight;
            handler->restoreWidth = npu_image.nWidth;
            handler->restoreHeight = npu_image.nHeight;
            axjoint_post_process_detect_single_func(&Results, handler);
            handler->restoreWidth = tmp_width;
            handler->restoreHeight = tmp_height;

            if (Results.objects.size() > 0) {
                axpi_object_t &obj = Results.objects[0];

                align_face(obj, &npu_image, npu_image_face_align);

                ret = axjoint_inference(handler->minorModel.jointHandle, &npu_image_face_align, nullptr);

                faceid.feat.resize(FACE_FEAT_LEN);
                memcpy(faceid.feat.data(), handler->minorModel.jointAttr.outputs[0].pVirAddr, FACE_FEAT_LEN * sizeof(float));
                _normalize(faceid.feat.data(), FACE_FEAT_LEN);
            }

            AX_SYS_MemFree(npu_image.pPhy, npu_image.pVir);
        }

        b_face_database_init = true;
    }

    int ret = axjoint_inference(handler->majorModel.jointHandle, pstFrame, NULL);
    axjoint_post_process_detect_single_func(results, handler);

    float feat[FACE_FEAT_LEN];
    for (size_t i = 0; i < results->objects.size(); i++) {
        axpi_object_t &obj = results->objects[i];
        align_face(obj, (AX_NPU_CV_Image *)pstFrame, npu_image_face_align);

        ret = axjoint_inference(handler->minorModel.jointHandle, &npu_image_face_align, nullptr);

        memcpy(&feat[0], handler->minorModel.jointAttr.outputs[0].pVirAddr, FACE_FEAT_LEN * 4);
        _normalize(feat, FACE_FEAT_LEN);

        int maxidx = -1;
        float max_score = 0;

        for (size_t j = 0; j < face_ids.size(); j++) {
            if (face_ids[j].feat.size() != FACE_FEAT_LEN) {
                continue;
            }

            float sim = _calcSimilar(feat, face_ids[j].feat.data(), FACE_FEAT_LEN);
            if ((sim > max_score) && (sim > face_recgnition_threahold)) {
                maxidx = j;
                max_score = sim;
            }
        }

        if (maxidx >= 0) {
            if (max_score >= face_recgnition_threahold) {
                strcpy(obj.objname, face_ids[maxidx].name.c_str());
            } else {
                strcpy(obj.objname, "unknow");
            }
        } else {
            strcpy(obj.objname, "unknow");
        }
    }

    for (size_t i = 0; i < results->objects.size(); i++) {
        results->objects[i].bbox.x /= handler->restoreWidth;
        results->objects[i].bbox.y /= handler->restoreHeight;
        results->objects[i].bbox.w /= handler->restoreWidth;
        results->objects[i].bbox.h /= handler->restoreHeight;

        if (results->objects[i].bHasBoxVertices) {
            for (size_t j = 0; j < 4; j++) {
                results->objects[i].bbox_vertices[j].x /= handler->restoreWidth;
                results->objects[i].bbox_vertices[j].y /= handler->restoreHeight;
            }
        }
    }

    return ret;
}

int _axjoint_inference_license_plate_recognition(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    int ret = axjoint_inference(handler->majorModel.jointHandle, pstFrame, NULL);
    axjoint_post_process_detect_single_func(results, handler);

    for (size_t i = 0; i < results->objects.size(); i++) {
        static AX_NPU_CV_Image tmp = {0};
        if (!tmp.pVir) {
            tmp.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
            tmp.nHeight = handler->minorModel.jointAttr.height;
            tmp.nWidth = handler->minorModel.jointAttr.width;
            tmp.tStride.nW = tmp.nWidth;

            if (tmp.eDtype == AX_NPU_CV_FDT_NV12) {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3 / 2;
            } else if ((tmp.eDtype == AX_NPU_CV_FDT_RGB) || (tmp.eDtype == AX_NPU_CV_FDT_BGR)) {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3;
            } else {
                axmpi_error("just only support nv12/rgb/bgr format");
                return -1;
            }

            AX_SYS_MemAlloc(&tmp.pPhy, (void **)&tmp.pVir, tmp.nSize, 0x100, NULL);
        }

        axpi_object_t &object = results->objects[i];

        cv::Point2f src_pts[4];
        src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
        src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
        src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
        src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

        cv::Point2f dst_pts[4];
        dst_pts[0] = cv::Point2f(0, 0);
        dst_pts[1] = cv::Point2f(handler->minorModel.jointAttr.width, 0);
        dst_pts[2] = cv::Point2f(handler->minorModel.jointAttr.width, handler->minorModel.jointAttr.height);
        dst_pts[3] = cv::Point2f(0, handler->minorModel.jointAttr.height);

        cv::Mat affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
        cv::Mat affine_trans_mat_inv;
        cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

        float mat3x3[3][3] = {
            {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
            {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
            {0, 0, 1}
        };

        ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &tmp, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);

        static const std::vector<std::string> plate_string = {
            "#", "beijing", "shanghai", "tianjin", "chongqing", "hebei", "shan1xi", "neimenggu", "liaoning", "jilin", "heilongjiang", "jiangsu", "zhejiang", "anhui",
            "fujian", "jiangxi", "shandong", "henan", "hubei", "hunan", "guangdong", "guangxi", "hainan", "sichuan", "guizhou", "yunnan", "xizang", "shan3xi",
            "gansu", "qinghai", "ningxia", "xinjiang", "jiaolian", "jingcha", "xianggang", "aomen", "gua", "shi", "ling", "ming", "hang", "shen",
            "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
            "A", "B", "C", "D", "E", "F", "G", "H",
            "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
        };

        ret = axjoint_inference(handler->minorModel.jointHandle, &tmp, nullptr);

        float argmax_idx[21];
        float argmax_data[21];
        // 1x1x21x78
        float *outputdata = (float *)handler->minorModel.jointAttr.outputs[0].pVirAddr;

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

        if (plate.length() < OBJ_NAME_MAX_LEN) {
            char name[OBJ_NAME_MAX_LEN] = {0};
            sprintf(name, "%.*s", plate.length(), plate.c_str());
            strcpy(results->objects[i].objname, name);
        }
    }

    for (size_t i = 0; i < results->objects.size(); i++) {
        results->objects[i].bbox.x /= handler->restoreWidth;
        results->objects[i].bbox.y /= handler->restoreHeight;
        results->objects[i].bbox.w /= handler->restoreWidth;
        results->objects[i].bbox.h /= handler->restoreHeight;

        if (results->objects[i].bHasBoxVertices) {
            for (size_t j = 0; j < 4; j++) {
                results->objects[i].bbox_vertices[j].x /= handler->restoreWidth;
                results->objects[i].bbox_vertices[j].y /= handler->restoreHeight;
            }
        }
    }

    return ret;
}

int _axjoint_inference_handpose(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    static AX_NPU_CV_Image _pstFrame = {0};

    if (!_pstFrame.pVir) {
        memcpy(&_pstFrame, pstFrame, sizeof(AX_NPU_CV_Image));
        _pstFrame.eDtype = AX_NPU_CV_FDT_BGR;
        AX_SYS_MemAlloc(&_pstFrame.pPhy, (void **)&_pstFrame.pVir, _pstFrame.nSize, 0x100, NULL);
    }

    _pstFrame.eDtype = AX_NPU_CV_FDT_BGR;
    AX_NPU_CV_CSC(AX_NPU_MODEL_TYPE_1_1_1, (AX_NPU_CV_Image *)pstFrame, &_pstFrame);
    _pstFrame.eDtype = AX_NPU_CV_FDT_RGB;

    int ret = axjoint_inference(handler->majorModel.jointHandle, &_pstFrame, NULL);
    axjoint_post_process_detect_single_func(results, handler);

    for (size_t i = 0; i < results->objects.size(); i++) {
        static AX_NPU_CV_Image tmp = {0};

        if (!tmp.pVir) {
            tmp.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
            tmp.nHeight = handler->minorModel.jointAttr.height;
            tmp.nWidth = handler->minorModel.jointAttr.width;
            tmp.tStride.nW = tmp.nWidth;

            if (tmp.eDtype == AX_NPU_CV_FDT_NV12) {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3 / 2;
            } else if ((tmp.eDtype == AX_NPU_CV_FDT_RGB) || (tmp.eDtype == AX_NPU_CV_FDT_BGR)) {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3;
            } else {
                axmpi_error("just only support nv12/rgb/bgr format");
                return -1;
            }

            AX_SYS_MemAlloc(&tmp.pPhy, (void **)&tmp.pVir, tmp.nSize, 0x100, NULL);
        }

        axpi_object_t &object = results->objects[i];

        cv::Point2f src_pts[4];
        src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
        src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
        src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
        src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

        cv::Point2f dst_pts[4];
        dst_pts[0] = cv::Point2f(0, 0);
        dst_pts[1] = cv::Point2f(handler->minorModel.jointAttr.width, 0);
        dst_pts[2] = cv::Point2f(handler->minorModel.jointAttr.width, handler->minorModel.jointAttr.height);
        dst_pts[3] = cv::Point2f(0, handler->minorModel.jointAttr.height);

        cv::Mat affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
        cv::Mat affine_trans_mat_inv;
        cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

        float mat3x3[3][3] = {
            {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
            {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
            {0, 0, 1}
        };

        ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &tmp, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);

        ret = axjoint_inference(handler->minorModel.jointHandle, &tmp, nullptr);
        axjoint_post_process_pose(handler, &object);

        for (size_t j = 0; j < HAND_LMK_SIZE; j++) {
            int x = affine_trans_mat_inv.at<double>(0, 0) * object.landmark[j].x + affine_trans_mat_inv.at<double>(0, 1) * object.landmark[j].y + affine_trans_mat_inv.at<double>(0, 2);
            int y = affine_trans_mat_inv.at<double>(1, 0) * object.landmark[j].x + affine_trans_mat_inv.at<double>(1, 1) * object.landmark[j].y + affine_trans_mat_inv.at<double>(1, 2);
            object.landmark[j].x = x;
            object.landmark[j].y = y;
        }
    }

    for (size_t i = 0; i < results->objects.size(); i++) {
        results->objects[i].bbox.x /= handler->restoreWidth;
        results->objects[i].bbox.y /= handler->restoreHeight;
        results->objects[i].bbox.w /= handler->restoreWidth;
        results->objects[i].bbox.h /= handler->restoreHeight;

        if (results->objects[i].bHasBoxVertices) {
            for (size_t j = 0; j < 4; j++) {
                results->objects[i].bbox_vertices[j].x /= handler->restoreWidth;
                results->objects[i].bbox_vertices[j].y /= handler->restoreHeight;
            }
        }

        if (results->objects[i].landmark.size() > 0) {
            for (size_t j = 0; j < MAX_LMK_SIZE; j++) {
                results->objects[i].landmark[j].x /= handler->restoreWidth;
                results->objects[i].landmark[j].y /= handler->restoreHeight;
            }
        }
    }

    return ret;
}

static MultikeyMap<std::string, int, inference_func_t> modelTypeTable = {
    {"MT_UNKNOWN",                         MT_UNKNOWN,                         nullptr},
    {"MT_DET_YOLOV5",                      MT_DET_YOLOV5,                      _axjoint_inference_detetect},
    {"MT_DET_YOLOV5_FACE",                 MT_DET_YOLOV5_FACE,                 _axjoint_inference_detetect},
    {"MT_DET_YOLOV7",                      MT_DET_YOLOV7,                      _axjoint_inference_detetect},
    {"MT_DET_YOLOV6",                      MT_DET_YOLOV6,                      _axjoint_inference_detetect},
    {"MT_DET_YOLOX",                       MT_DET_YOLOX,                       _axjoint_inference_detetect},
    {"MT_DET_NANODET",                     MT_DET_NANODET,                     _axjoint_inference_detetect},
    {"MT_INSEG_YOLOV5_MASK",               MT_INSEG_YOLOV5_MASK,               _axjoint_inference_detetect},
    {"MT_DET_YOLOX_PPL",                   MT_DET_YOLOX_PPL,                   _axjoint_inference_detetect},
    {"MT_DET_PALM_HAND",                   MT_DET_PALM_HAND,                   _axjoint_inference_detetect},
    {"MT_DET_YOLOPV2",                     MT_DET_YOLOPV2,                     _axjoint_inference_detetect},
    {"MT_DET_YOLO_FASTBODY",               MT_DET_YOLO_FASTBODY,               _axjoint_inference_detetect},
    {"MT_DET_YOLOV5_LICENSE_PLATE",        MT_DET_YOLOV5_LICENSE_PLATE,        _axjoint_inference_detetect},
    {"MT_DET_YOLOV7_FACE",                 MT_DET_YOLOV7_FACE,                 _axjoint_inference_detetect},
    {"MT_DET_YOLOV7_PALM_HAND",            MT_DET_YOLOV7_PALM_HAND,            _axjoint_inference_detetect},
    {"MT_SEG_PPHUMSEG",                    MT_SEG_PPHUMSEG,                    _axjoint_inference_pphumseg},
    {"MT_MLM_HUMAN_POSE_HRNET",            MT_MLM_HUMAN_POSE_HRNET,            _axjoint_inference_human_pose},
    {"MT_MLM_ANIMAL_POSE_HRNET",           MT_MLM_ANIMAL_POSE_HRNET,           _axjoint_inference_animal_pose},
    {"MT_MLM_HUMAN_POSE_AXPPL",            MT_MLM_HUMAN_POSE_AXPPL,            _axjoint_inference_human_pose},
    {"MT_MLM_HAND_POSE",                   MT_MLM_HAND_POSE,                   _axjoint_inference_handpose},
    {"MT_MLM_VEHICLE_LICENSE_RECOGNITION", MT_MLM_VEHICLE_LICENSE_RECOGNITION, _axjoint_inference_license_plate_recognition},
    {"MT_MLM_FACE_RECOGNITION",            MT_MLM_FACE_RECOGNITION,            _axjoint_inference_face_recognition}
};

int axjoint_parse_param(std::string confiJsonFile, axjoint_models_t *handler)
{
    std::ifstream f(confiJsonFile);
    if (f.fail()) {
        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);
    f.close();

    if (jsondata.contains("model_type")) {
        if (jsondata["model_type"].is_number_integer()) {
            int mt = -1;
            mt = jsondata["model_type"];
            if (modelTypeTable.contain(mt)) {
                handler->modelTypeMain = mt;
            } else {
                handler->modelTypeMain = MT_UNKNOWN;
            }
        } else if (jsondata["model_type"].is_string()) {
            std::string strModelType = jsondata["model_type"];

            if (modelTypeTable.contain(strModelType)) {
                auto match_vec = modelTypeTable.get1(strModelType);
                if (match_vec.size() > 1) {
                    axmpi_error("[%s] multi define in modelTypeTable, please check modelTypeTable", strModelType.c_str());
                    return -1;
                }

                handler->modelTypeMain = match_vec[0]->key2;
            } else {
                handler->modelTypeMain = MT_UNKNOWN;
            }
        }

        handler->majorModel.modelType = handler->modelTypeMain;
    }

    if (jsondata.contains("model_path")) {
        std::string path = jsondata["model_path"];
        if (!path.empty()) {
            handler->modelPath = path;
            handler->runJoint = true;
        }
    }

    if (jsondata.contains("ivps_algo_width")) {
        handler->ivpsAlgoWidth = jsondata["ivps_algo_width"];
    }

    if (jsondata.contains("ivps_algo_height")) {
        handler->ivpsAlgoHeight = jsondata["ivps_algo_height"];
    }

    switch (handler->modelTypeMain) {
        case MT_DET_YOLOV5:
        case MT_DET_YOLOPV2:
        case MT_DET_YOLOV5_FACE:
        case MT_DET_YOLOV6:
        case MT_DET_YOLOV7:
        case MT_DET_YOLOX:
        case MT_DET_NANODET:
        case MT_DET_YOLOX_PPL:
        case MT_INSEG_YOLOV5_MASK:
        case MT_DET_PALM_HAND:
        case MT_DET_YOLOV7_FACE:
        case MT_DET_YOLOV7_PALM_HAND:
        case MT_DET_YOLOV5_LICENSE_PLATE:
            axjoint_parse_param(confiJsonFile);
            handler->majorModel.modelType = handler->modelTypeMain;
            break;

        case MT_SEG_PPHUMSEG:
            handler->majorModel.modelType = handler->modelTypeMain;
            break;

        case MT_MLM_HUMAN_POSE_AXPPL:
        case MT_MLM_HUMAN_POSE_HRNET:
        case MT_MLM_ANIMAL_POSE_HRNET:
        case MT_MLM_HAND_POSE:
        case MT_MLM_FACE_RECOGNITION:
        case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
            if (jsondata.contains("model_major")) {
                nlohmann::json json_major = jsondata["model_major"];
                if (json_major.contains("model_type")) {
                    if (json_major["model_type"].is_number_integer()) {
                        int mt = -1;

                        mt = json_major["model_type"];
                        if (modelTypeTable.contain(mt)) {
                            handler->majorModel.modelType = mt;
                        } else {
                            handler->majorModel.modelType = MT_UNKNOWN;
                        }
                    } else if (json_major["model_type"].is_string()) {
                        std::string strModelType = json_major["model_type"];

                        if (modelTypeTable.contain(strModelType)) {
                            auto match_vec = modelTypeTable.get1(strModelType);
                            if (match_vec.size() > 1) {
                                axmpi_error("[%s] multi define in modelTypeTable, please check modelTypeTable", strModelType.c_str());
                                return -1;
                            }

                            handler->majorModel.modelType = match_vec[0]->key2;
                        } else {
                            handler->majorModel.modelType = MT_UNKNOWN;
                        }
                    }
                }

                if (json_major.contains("model_path")) {
                    std::string path = json_major["model_path"];
                    if (!path.empty()) {
                        handler->modelPath = path;
                        handler->runJoint = true;
                    }
                }

                axjoint_set_param(&json_major);
            }
    
            if (jsondata.contains("model_minor")) {
                nlohmann::json json_minor = jsondata["model_minor"];
                if (json_minor.contains("model_path")) {
                    std::string hrnet_path = json_minor["model_path"];
                    handler->modelPathL2 = hrnet_path;
                }

                if (json_minor.contains("class_id")) {
                    std::vector<int> clsids = json_minor["class_id"];

                    int classIds = MIN(clsids.size(), CLASS_ID_COUNT);
                    handler->minorModelClassIds.resize(classIds);
                    for (int i = 0; i < classIds; i++) {
                        handler->minorModelClassIds[i] = clsids[i];
                    }
                }

                if (json_minor.contains("face_database")) {
                    nlohmann::json database = json_minor["face_database"];
                    for (nlohmann::json::iterator it = database.begin(); it != database.end(); ++it) {
                        axjoint_faceid_t faceid;
                        faceid.path = it.value();
                        faceid.name = it.key();
                        face_ids.push_back(faceid);
                    }
                }

                if (json_minor.contains("face_recgnition_threahold")) {
                    face_recgnition_threahold = json_minor["face_recgnition_threahold"];
                }
            }
            break;

        default:
            axmpi_error("unkown model type:[%d]", handler->modelTypeMain);
            break;
    }

    if (handler->modelTypeMain == MT_UNKNOWN) {
        handler->runJoint = false;
    }

    return 0;
}

int axjoint_inference_single_func(axjoint_models_t *handler, const void *pstFrame, axpi_results_t *results)
{
    int ret = 0;

    memset(results, 0, sizeof(axpi_results_t));
    results->modelType = handler->modelTypeMain;

    if (modelTypeTable.contain(handler->modelTypeMain)) {
        int mt = handler->modelTypeMain;
        auto func = modelTypeTable.get2(mt);
        if (func[0]->val != nullptr) {
            ret = func[0]->val(handler, pstFrame, results);
        } else {
            axmpi_error("[%s] func pointer is null", func[0]->key1.c_str());
        }
    } else {
        axmpi_error("cannot find inference func for model type:[%d]", handler->modelTypeMain);
        ret = -1;
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

        results->inFps = fps;
    }

    return ret;
}
}
