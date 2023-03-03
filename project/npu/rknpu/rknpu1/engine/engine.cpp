#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include "engine.hpp"

API_BEGIN_NAMESPACE(Ai)

static void printRKNNTensor(const char *name, rknn_tensor_attr &attr)
{
    const char *qnty[] = {"NONE", "DFP", "AFFINE", "UNKNOWN"};
    const char *type[] = {"fp32", "fp16", "int8", "uint8", "int16"};

    printf("\033[1;36m[%s tensor] index:[%u], dims:[%u, %3u, %3u, %3u], n_elems:[%8u], fmt:[%s], type:[%s], qnt_type:[%s], zp:[%3u], scale:[%.6f], name:[%s]\033[0m\n",
        name, attr.index, attr.dims[3], attr.dims[2], attr.dims[1], attr.dims[0],
        attr.n_elems, attr.fmt == 0 ? "NCHW" : "NHWC", type[attr.type], qnty[attr.qnt_type], attr.zp, attr.scale, attr.name);
}

AiEngine::AiEngine() : mInitFin(false), mContext(0)
{

}

AiEngine::~AiEngine()
{
    mInitFin = false;
    if (mContext) {
        rknn_destroy(mContext);
        mContext = 0;
    }
}

int AiEngine::init(std::string modelFile)
{
    FILE *fp = fopen(modelFile.c_str(), "rb");
    if (!fp) {
        rknpu_error("open model file:[%s] failed", modelFile.c_str());
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long modelBytes = ftell(fp);
    rewind(fp);

    if (modelBytes <= 0) {
        fclose(fp);
        rknpu_error("load model file:[%s] failed, model bytes:[%ld]", modelFile.c_str(), modelBytes);
        return -2;
    }

    unsigned char *model = new unsigned char[modelBytes + 1];
    if (model == nullptr) {
        fclose(fp);
        rknpu_error("new model memory failed");
        return -3;
    }

    size_t nread = fread(model, 1, modelBytes, fp);
    if ((long)nread != modelBytes) {
        fclose(fp);
        rknpu_error("load model file:[%s] failed, read model bytes:[%u] != [%ld]", modelFile.c_str(), nread, modelBytes);
        delete[] model;
        return -4;
    }

    fclose(fp);
    int ret = rknn_init(&mContext, model, modelBytes, RKNN_FLAG_PRIOR_HIGH);
    if (ret != RKNN_SUCC) {
        delete[] model;
        rknpu_error("rknn_init failed, return:[%d]", ret);
        return -5;
    } else {
        rknpu_info("rknn init succeeded, context:[0x%X]", mContext);
    }

    delete[] model;

    int findGalcoreDevCount = 0;
    do {
        if (access("/dev/galcore", F_OK) != 0) {
            findGalcoreDevCount += 1;
            rknpu_warn("can't find npu driver, %d times", findGalcoreDevCount);
            sleep(1);
        } else {
            findGalcoreDevCount = 0;
            break;
        }
    } while (findGalcoreDevCount < 3);

    if (findGalcoreDevCount >= 3) {
        rknpu_error("can't find npu driver, please check npu driver");
        return -6;
    }

    rknn_sdk_version sdkver;
    ret = rknn_query(mContext, RKNN_QUERY_SDK_VERSION, &sdkver, sizeof(sdkver));
    if (ret == RKNN_SUCC) {
        rknpu_info("sdk version:[%s], driver version:[%s]", sdkver.api_version, sdkver.drv_version);
    } else {
        rknpu_warn("rknn_query sdk version failed, return:[%d]", ret);
    }

    ret = rknn_query(mContext, RKNN_QUERY_IN_OUT_NUM, &mIOCount, sizeof(mIOCount));
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn query input/output number failed, return:[%d]", ret);
        return -7;
    }

    for (uint32_t i = 0; i < mIOCount.n_input; ++i) {
        rknn_tensor_attr attr;

        attr.index = i;
        ret = rknn_query(mContext, RKNN_QUERY_INPUT_ATTR, &attr, sizeof(attr));
        if (ret != RKNN_SUCC) {
            rknpu_warn("rknn_query input attribute failed, return:[%d]", ret);
        }

        printRKNNTensor("input ", attr);
        mInputAttr.push_back(attr);
    }

    for (uint32_t i = 0; i < mIOCount.n_output; ++i) {
        rknn_tensor_attr attr;

        attr.index = i;
        ret = rknn_query(mContext, RKNN_QUERY_OUTPUT_ATTR, (void *)&attr, sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            rknpu_warn("rknn_query output attribute failed, return:[%d]", ret);
        }
        printRKNNTensor("output", attr);

        mOutputAttr.push_back(attr);
    }

    mInitFin = true;
    return 0;
}

int AiEngine::forward(cv::Mat image)
{
    if (!mInitFin || image.empty()) {
        rknpu_error("Invalid input image or engine not initialized");
        return -1;
    }

    rknn_input inputs[1];
    inputs[0].index = 0;
    inputs[0].buf = image.data;
    inputs[0].pass_through = 0;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = image.cols * image.rows * image.channels();

    int ret = rknn_inputs_set(mContext, 1, inputs);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_inputs_set failed, return:[%d]", ret);
        return -2;
    }

    ret = rknn_run(mContext, NULL);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_run failed, return:[%d]", ret);
        return -3;
    }

    return 0;
}

int AiEngine::forward(std::vector<cv::Mat> imageSet)
{
    if (!mInitFin || (imageSet.size() != mIOCount.n_input)) {
        rknpu_error("engine not init or invalid image size");
        return -1;
    }

    for (size_t i = 0; i < imageSet.size(); i++) {
        if (imageSet[i].empty()) {
            rknpu_error("image null");
            return -2;
        }
    }

    rknn_input inputs[mIOCount.n_input];
    memset(inputs, 0x00, mIOCount.n_input * sizeof(rknn_input));

    for (size_t i = 0; i < mIOCount.n_input; i++) {
        inputs[i].index = i;
        inputs[i].buf = imageSet[i].data;
        inputs[i].pass_through = 0;
        inputs[i].fmt = RKNN_TENSOR_NHWC;
        inputs[i].type = RKNN_TENSOR_UINT8;
        inputs[i].size = imageSet[i].cols * imageSet[i].rows * imageSet[i].channels();
    }

    int ret = rknn_inputs_set(mContext, mIOCount.n_input, inputs);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_inputs_set failed, return:[%d]", ret);
        return -3;
    }

    ret = rknn_run(mContext, NULL);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_run failed, return:[%d]", ret);
        return -3;
    }

    return 0;
}

int AiEngine::forward(unsigned char *data, size_t dataSize)
{
    if (!mInitFin || !data || (dataSize == 0)) {
        rknpu_error("Invalid input data or engine not initialized");
        return -1;
    }

    rknn_input inputs[1];
    inputs[0].index = 0;
    inputs[0].buf = data;
    inputs[0].size = dataSize;
    inputs[0].pass_through = 0;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].type = RKNN_TENSOR_UINT8;

    int ret = rknn_inputs_set(mContext, 1, inputs);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_inputs_set failed, return:[%d]", ret);
        return -2;
    }

    ret = rknn_run(mContext, NULL);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_run failed, return:[%d]", ret);
        return -3;
    }

    return 0;
}

int AiEngine::forward(std::vector<unsigned char *> dataSet, std::vector<size_t> dataSizeSet)
{
    if (!mInitFin || (dataSizeSet.size() != dataSet.size()) || (dataSet.size() != mIOCount.n_input)) {
        rknpu_error("engine not initialized or invalid data");
        return -1;
    }

    for (size_t i = 0; i < dataSet.size(); ++i) {
        if ((dataSet[i] == NULL) || (dataSizeSet[i] == 0)) {
            rknpu_error("image data null");
            return -2;
        }
    }

    int ret = -1;
    rknn_input inputs[mIOCount.n_input];

    memset(inputs, 0x00, mIOCount.n_input * sizeof(rknn_input));
    for (size_t i = 0; i < mIOCount.n_input; ++i) {
        inputs[i].index = i;
        inputs[i].buf = dataSet[i];
        inputs[i].size = dataSizeSet[i];
        inputs[i].pass_through = 0;
        inputs[i].fmt = RKNN_TENSOR_NHWC;
        inputs[i].type = RKNN_TENSOR_UINT8;
    }

    ret = rknn_inputs_set(mContext, mIOCount.n_input, inputs);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_inputs_set failed, return:[%d]", ret);
        return -3;
    }

    ret = rknn_run(mContext, NULL);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_run failed, return:[%d]", ret);
        return -4;
    }

    return 0;
}

int AiEngine::forward(std::vector<rknn_tensor_mem> &inputs_mem, size_t index, bool &bLockInMem)
{
    if (!mInitFin) {
        rknpu_error("engine not initialized");
        return -1;
    }

    if (index > inputs_mem.size()) {
        index = 0;
    }

    int ret = -1;
    if (!bLockInMem) {
        ret = rknn_set_io_mem(mContext, &inputs_mem[index], &mInputAttr[index]);
        if (ret != RKNN_SUCC) {
            rknpu_error("rknn_set_io_mem failed, return:[%d]", ret);
            return -2;
        }

        bLockInMem = true;
    }

    ret = rknn_run(mContext, NULL);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_run failed, return:[%d]", ret);
        return -3;
    }

    return 0;
}

int AiEngine::extract(extract_func_ptr extract_cb, nms_func_ptr nms_cb, void *args, std::map<int, std::vector<bbox>> &lastResult)
{
    if (!mInitFin) {
        rknpu_error("engine not initialized");
        return -1;
    }

    for (uint32_t i = 0; i < mIOCount.n_output; ++i) {
        mOutputs[i].index = i;
        mOutputs[i].want_float = 1;
        mOutputs[i].is_prealloc = 0;
    }

    int ret = rknn_outputs_get(mContext, mIOCount.n_output, mOutputs, NULL);
    if (ret != RKNN_SUCC) {
        rknpu_error("rknn_outputs_get failed, return:[%d]", ret);
        return -2;
    }

    if (extract_cb) {
        ret = extract_cb(mOutputs, mOutputAttr, mIOCount.n_output, nms_cb, args, lastResult);
    }

    rknn_outputs_release(mContext, mIOCount.n_output, mOutputs);
    return ret;
}

int AiEngine::setOutputMemory(std::vector<rknn_tensor_mem> &outputs_mem)
{
    int ret = -1;

    for (size_t i = 0; i < outputs_mem.size(); ++i) {
        ret = rknn_set_io_mem(mContext, &outputs_mem[i], &mOutputAttr[i]);
        if (ret != RKNN_SUCC) {
            rknpu_error("rknn_set_io_mem failed, return:[%d]", ret);
            return -1;
        }
    }

    return ret;
}

uint32_t AiEngine::getInputCount()
{
    return mInitFin ? mIOCount.n_input : 0;
}

uint32_t AiEngine::getOutputCount()
{
    return mInitFin ? mIOCount.n_output : 0;
}

std::vector<rknn_tensor_attr> AiEngine::getInputAttr()
{
    return mInputAttr;
}

std::vector<rknn_tensor_attr> AiEngine::getOutputAttr()
{
    return mOutputAttr;
}

void AiEngine::nms(std::vector<bbox> &inputBbox, float threshold)
{
    float t, l, b, r;
    float iou, dw, dh, sizeA;
    std::sort(inputBbox.begin(), inputBbox.end(), [](const bbox &box1, const bbox &box2) {
        return (box1.score > box2.score);
    });

    for (int i = 0; i < int(inputBbox.size()); ++i) {
        for (int j = i + 1; j < int(inputBbox.size()); ) {
            t = std::max(inputBbox[i].top, inputBbox[j].top);
            l = std::max(inputBbox[i].left, inputBbox[j].left);
            r = std::min(inputBbox[i].right, inputBbox[j].right);
            b = std::min(inputBbox[i].bottom, inputBbox[j].bottom);

            dw = r - l;
            dh = b - t;

            if ((dw > 0) && (dh > 0)) {
                sizeA = (inputBbox.at(i).right - inputBbox.at(i).left) * (inputBbox.at(i).bottom - inputBbox.at(i).top);
                iou = (dw * dh) / (sizeA + sizeA - (dw * dh));
            } else {
                iou = 0;
            }

            if (iou >= threshold) {
                inputBbox.erase(inputBbox.begin() + j);
            } else {
                j++;
            }
        }
    }
}

API_END_NAMESPACE(Ai)
