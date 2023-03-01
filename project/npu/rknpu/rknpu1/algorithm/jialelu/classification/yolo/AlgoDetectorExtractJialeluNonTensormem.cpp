#include "../../../AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::extract_yolov5s_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults)
{
    algo_args_t *pArgs = (algo_args_t *)args;
    const int m_classes = int(pArgs->classCount);
    std::vector<int> filterClass = pArgs->filterClass;
    std::vector<yolo_layer_t> m_layers = pArgs->yoloAnchors;

    if (m_layers.size() == 0) {
        printf("anchors parameter must be non-empty\n");
        return -1;
    }

    float nms_threshold = pArgs->nms_threshold;
    float conf_threshold = pArgs->conf_threshold;
    rknn_output *pRawBuf = (rknn_output *)output;
    size_t imageWidth = pArgs->imageWidth, imageHeight = pArgs->imageHeight;
    size_t modelWidth = pArgs->modelInputWidth, modelHeight = pArgs->modelInputHeight;

    for (size_t i = 0; i < output_count; ++i) {
        auto anchors = m_layers[i].anchors;
        auto *pOrig = static_cast<float *>(pRawBuf[i].buf);
        float *pRawData = pOrig;
        int grideSizeW = modelWidth / m_layers[i].stride;
        int grideSizeH = modelHeight / m_layers[i].stride;
        int page = grideSizeW * grideSizeH;

        for (size_t prior = 0; prior < anchors.size(); ++prior) {
            pRawData = prior * page * (4 + 1 + m_classes) + pOrig;              // 指向每一个prior box的起始地址
            for (int shift_y = 0; shift_y < grideSizeH; ++shift_y) {
                for (int shift_x = 0; shift_x < grideSizeW; ++shift_x) { 
                    int classify = -1;
                    float maxScore = 0.0f;
                    float confidence = 0.0f;
                    float *record = pRawData + shift_x;                         // 按行取数据

                    confidence = record[4 * page];
                    for (int k = 0; k < m_classes; ++k) {
                        float score = confidence * record[(5 + k) * page];
                        if (score > maxScore) {
                            classify = k;
                            maxScore = score;
                        }
                    }

                    if (maxScore > conf_threshold) {
                        std::vector<int>::iterator iter = find(filterClass.begin(), filterClass.end(), classify);
                        if (iter == filterClass.end()) {
                            float w = pow(record[2 * page] * 2.0f, 2) * anchors[prior].width;
                            float h = pow(record[3 * page] * 2.0f, 2) * anchors[prior].height;
                            float cx = (record[0 * page] * 2.0f - 0.5f + shift_x) * (m_layers[i].stride);
                            float cy = (record[1 * page] * 2.0f - 0.5f + shift_y) * (m_layers[i].stride);

                            bbox box;
                            box.score = maxScore;
                            box.classify = classify;
                            box.top = std::max(0, std::min((int)modelHeight, int(cy - h / 2.0f)));
                            box.left = std::max(0, std::min((int)modelWidth, int(cx - w / 2.0f)));
                            box.right = std::max(0, std::min((int)modelWidth, int(cx + w / 2.0f)));
                            box.bottom = std::max(0, std::min((int)modelHeight, int(cy + h / 2.0f)));

                            auto it = lastResults.find(classify);
                            if (it != lastResults.end()) {
                                it->second.emplace_back(box);
                            } else {
                                lastResults.insert(std::make_pair(classify, std::vector<bbox>{box}));
                            }
                        }
                    }
                }

                pRawData += grideSizeW;                                     // 取下一页
            }
        }
    }

    if (nms_callback) {
        for (auto &last : lastResults) {
            nms_callback(last.second, nms_threshold);
        }
    }

    return 0;
}

int AlgoDetector::extract_yoloxs_non_tensormem_jialelu(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults)
{
    algo_args_t *pArgs = (algo_args_t *)args;
    const int m_classes = int(pArgs->classCount);
    std::vector<int> filterClass = pArgs->filterClass;
    std::vector<yolo_layer_t> m_layers = pArgs->yoloAnchors;

    if (m_layers.size() == 0) {
        printf("anchors parameter must be non-empty\n");
        return -1;
    }

    float nms_threshold = pArgs->nms_threshold;
    float conf_threshold = pArgs->conf_threshold;
    rknn_output *pRawBuf = (rknn_output *)output;
    size_t imageWidth = pArgs->imageWidth, imageHeight = pArgs->imageHeight;
    size_t modelWidth = pArgs->modelInputWidth, modelHeight = pArgs->modelInputHeight;

    for (size_t idx = 0; idx < output_count; ++idx) {
        int stride = m_layers[idx].stride;
        int grideSizeW = modelWidth / stride;
        int grideSizeH = modelHeight / stride;
        int stepPages = grideSizeW * grideSizeH;
        float *pRawData = static_cast<float *>(pRawBuf[idx].buf);

        for (int y = 0; y < grideSizeH; ++y) {
            for (int x = 0; x < grideSizeW; ++x) {
                const int bboxIndex = y * grideSizeW + x;               // x   y   w   h
                const float objectNess = pRawData[bboxIndex + stepPages * (1 + 1 + 1 + 1)];
                if (objectNess < conf_threshold) {
                    continue;
                }

                int classify = -1;
                float maxScore = 0.0f;
                for (int i = 0; i < m_classes; ++i) {               // x   y   w   h   s
                    float prob = pRawData[bboxIndex + stepPages * (1 + 1 + 1 + 1 + 1 + i)];
                    if (prob > maxScore) {
                        maxScore = prob;
                        classify = i;
                    }
                }

                float prob = objectNess * maxScore;
                if (prob > conf_threshold) {
                    std::vector<int>::iterator iter = find(filterClass.begin(), filterClass.end(), classify);
                    if (iter == filterClass.end()) {
                        const float cx = (pRawData[bboxIndex + stepPages * 0] + (float)x) * stride;
                        const float cy = (pRawData[bboxIndex + stepPages * 1] + (float)y) * stride;
                        const float bw = exp(pRawData[bboxIndex + stepPages * 2]) * stride;
                        const float bh = exp(pRawData[bboxIndex + stepPages * 3]) * stride;
                        const float bx = cx - bw * 0.5f;
                        const float by = cy - bh * 0.5f;

                        bbox box;
                        box.score = prob;
                        box.classify = classify;
                        box.top = std::max(0, std::min((int)modelHeight, int(by)));
                        box.left = std::max(0, std::min((int)modelWidth, int(bx)));
                        box.right = std::max(0, std::min((int)modelWidth, int(box.left + bw)));
                        box.bottom = std::max(0, std::min((int)modelHeight, int(box.top + bh)));

                        auto it = lastResults.find(classify);
                        if (it != lastResults.end()) {
                            it->second.emplace_back(box);
                        } else {
                            lastResults.insert(std::make_pair(classify, std::vector<bbox>{box}));
                        }
                    }
                }
            }
        }
    }

    if (nms_callback) {
        for (auto &last : lastResults) {
            nms_callback(last.second, nms_threshold);
        }
    }

    return 0;
}

API_END_NAMESPACE(Ai)
