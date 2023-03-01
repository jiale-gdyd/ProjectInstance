#ifndef AARCH32_ROCKCHIP_RKNPU_ALGORITHM_ALGO_DETECTOR_HPP
#define AARCH32_ROCKCHIP_RKNPU_ALGORITHM_ALGO_DETECTOR_HPP

#include <mutex>
#include <numeric>
#include <algorithm>
#include <opencv2/opencv.hpp>

#include "private.hpp"
#include "../private.hpp"
#include "../engine/engine.hpp"
#include "accelerator/accelerator.hpp"

API_BEGIN_NAMESPACE(Ai)

class API_HIDDEN AlgoDetector {
public:
    AlgoDetector(std::string modelFile, size_t imageWidth, size_t imageHeight, size_t classCount, float confThreshold, float nmsThreshold, int algoType, int algoAuthor);
    ~AlgoDetector();

    virtual int init();

    virtual void setYoloAnchor(std::vector<yolo_layer_t> anchor);

    virtual int forward(cv::Mat frame);
    virtual int forward(void *mediaFrame);
    virtual int forward(unsigned char *data, size_t dataSize);

    virtual int extract(std::map<int, std::vector<bbox>> &lastResults);
    virtual int extract(std::map<int, std::vector<bbox>> &lastResults, std::vector<int> filterClass);

private:
    int extract_rockchip(std::map<int, std::vector<bbox>> &lastResults, std::vector<int> filterClass);
    static int extract_yolov5s_tensormem_rockchip(std::vector<rknn_tensor_mem> &output, void *args, std::map<int, std::vector<bbox>> &lastResults);
    static int extract_yolov5s_non_tensormem_rockchip(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults);

private:
    int extract_jialelu(std::map<int, std::vector<bbox>> &lastResults, std::vector<int> filterClass);

    static int extract_yoloxs_tensormem_jialelu(std::vector<rknn_tensor_mem> &output, void *args, std::map<int, std::vector<bbox>> &lastResults);
    static int extract_yoloxs_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults);

    static int extract_yolov5s_tensormem_jialelu(std::vector<rknn_tensor_mem> &output, void *args, std::map<int, std::vector<bbox>> &lastResults);
    static int extract_yolov5s_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults);

    static int extract_yolox_tiny_face_tensormem_jialelu(std::vector<rknn_tensor_mem> &output, void *args, std::map<int, std::vector<bbox>> &lastResults);
    static int extract_yolox_tiny_face_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults);

    static int extract_yolox_nano_face_one_anchor_tensormem_jialelu(std::vector<rknn_tensor_mem> &output, void *args, std::map<int, std::vector<bbox>> &lastResults);
    static int extract_yolox_nano_face_one_anchor_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults);

    static int extract_yolox_nano_face_one_anchor_tiny_tensormem_jialelu(std::vector<rknn_tensor_mem> &output, void *args, std::map<int, std::vector<bbox>> &lastResults);
    static int extract_yolox_nano_face_one_anchor_tiny_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults);

private:
    int processInferImage(void *mediaFrame, cv::Mat &inferMat, unsigned char *virtualCache);
    int processInferImage(cv::Mat rawRGBImage, cv::Mat &inferMat, unsigned char *virtualCache);

public:
    void getResizeRealWidthHeight(size_t &width, size_t &height);
    void getModelWHC(size_t &width, size_t &height, size_t &channels);

private:
    static void nms(std::vector<bbox> &input_boxes, float threshold);
    static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order, int filterId, float nms_threshold);

    static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices);
    static int post_process(uint8_t *input, algo_args_t *args, int index, std::vector<float> &boxes, std::vector<float> &objProbs, std::vector<int> &classId);

    template<typename T>
    static std::vector<unsigned int> argsort(const std::vector<T> array);

    static std::vector<unsigned int> non_max_suppression(std::vector<std::vector<float>> boxes, std::vector<float> scores_vector, float threshold, int size);
    static void compute_iou(std::vector<std::vector<float>> boxes, std::vector<unsigned int> scores_indexes, std::vector<float> areas, int index, std::vector<float> &ious);

private:
    static inline float fast_exp(float x) {
        union {unsigned int i; float f;} v{};
        v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);

        return v.f;
    }

    static inline int clamp(float val, int min, int max) {
        return val > min ? (val < max ? val : max) : min;
    }

    static inline float sigmoid(float x) {
        return 1.0 / (1.0 + expf(-x));
    }

    static inline float unsigmoid(float y) {
        return -1.0 * logf((1.0 / y) - 1.0);
    }

    static inline int32_t __clip(float val, float min, float max) {
        float f = val <= min ? min : (val >= max ? max : val);
        return f;
    }

    static inline uint8_t qnt_f32_to_affine(float f32, uint32_t zp, float scale) {
        float dst_val = (f32 / scale) + zp;
        uint8_t res = (uint8_t)__clip(dst_val, 0, 255);
        return res;
    }

    static inline float deqnt_affine_to_f32(uint8_t qnt, uint32_t zp, float scale) {
        return ((float)qnt - (float)zp) * scale;
    }

    static inline float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
        float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
        float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
        float i = w * h;
        float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;

        return u <= 0.f ? 0.f : (i / u);
    }

private:
    std::vector<yolo_layer_t> getYolov4DefaultAnchors(void) {
        static std::vector<yolo_layer_t> m_layers = {
            {"output1", 32, {{64, 50}, {96, 84}, {142, 139}}},
            {"output2", 16, {{25, 19}, {21, 53}, {41,  30}}},
            {"output3", 8,  {{6,  7},  {14, 11}, {9,   22}}},
        };

        return m_layers;
    }

    std::vector<yolo_layer_t> getYolov5DefaultAnchors(void) {
        static std::vector<yolo_layer_t> m_layers = {
            {"output3", 8,  {{10,  13}, {16,   30}, {33,  23}}},
            {"output2", 16, {{30,  61}, {62,   45}, {59,  119}}},
            {"output1", 32, {{116, 90}, {156, 198}, {373, 326}}}
        };

        return m_layers;
    }

private:
    bool                          mInitFin;               // 初始化完成
    bool                          mZeroCopy;              // 使用零拷贝
    bool                          mTensorMem;             // 使用映射内存地址
    bool                          mLockInputMem;          // 锁定输入内存信息

    std::string                   mModelFile;             // 算法模型文件
    size_t                        mImageWidth;            // 输入图像宽度(原图)
    size_t                        mImageHeight;           // 输入图像高度(原图)
    size_t                        mClassCount;            // 分类检测的分类数(例如80个类)
    float                         mConfThreshold;         // 置信度过滤阈值
    float                         mNmsThreshold;          // 非极大抑制过滤阈值
    int                           mAlgoType;              // 算法类型(例如Yolov5, Yolov3等)
    int                           mAlgoAuthor;            // 算法提供作者(例如rockchip，不同作者可能在处理上有区别)

    size_t                        mModelChns;             // 模型通道数
    size_t                        mRealWidth;             // 有效图像宽度
    size_t                        mRealHeight;            // 有效图像高度
    size_t                        mInputWidth;            // 模型输入图像宽
    size_t                        mInputHeight;           // 模型输入图像高

private:
    AiEngine                      *pAiEngine;             // 算法推理引擎句柄
    hardware::Accelerator         *pAccelerator;          // 加速前处理器件实例句柄

    std::vector<rknn_tensor_mem>  mInputsTensorMem;       // 输入张量内存信息
    std::vector<rknn_tensor_mem>  mOutputsTensorMem;      // 输出张量内存信息

    uint32_t                      mModelInputCount;       // 模型输入数量
    uint32_t                      mModelOutputCount;      // 模型输出数量
    std::vector<rknn_tensor_attr> mModelInputChnAttr;     // 模型每个输入通道的属性
    std::vector<rknn_tensor_attr> mModelOutputChnAttr;    // 模型每个输出通道的属性

    std::mutex                    mCallbackMutex;         // 互斥锁
    std::vector<yolo_layer_t>     mYoloAnchors;           // YOLO输出层

private:
    enum {
        CACHE_PING = 0,
        CACHE_PONG = 1,
        CACHE_BUTT
    };

    size_t                        mPingPongFlag;          // 取缓存标志
    size_t                        mCacheDataSize;         // 数据字节数
    cv::Mat                       mInferMat[CACHE_BUTT];  // 推理图像矩阵
    unsigned char                 *mCacheData[CACHE_BUTT];// 缓存输入到NPU推理的数据
};

API_END_NAMESPACE(Ai)

#endif
