#include "../../../AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

int AlgoDetector::extract_yolov5s_non_tensormem_rockchip(rknn_output *output, std::vector<rknn_tensor_attr> attr, size_t output_count, nms_func_ptr nms_callback, void *args, std::map<int, std::vector<bbox>> &lastResults)
{
    algo_args_t *pArgs = (algo_args_t *)args;
    const int m_classes = int(pArgs->classCount);
    std::vector<int> filterClass = pArgs->filterClass;
    std::vector<yolo_layer_t> m_layers = pArgs->yoloAnchors;

    float nms_threshold = pArgs->nms_threshold;
    float conf_threshold = pArgs->conf_threshold;
    size_t imageWidth = pArgs->imageWidth, imageHeight = pArgs->imageHeight;
    size_t modelWidth = pArgs->modelInputWidth, modelHeight = pArgs->modelInputHeight;

    int validCount = 0;
    std::vector<int> classId;
    std::vector<float> objProbs;
    std::vector<float> filterBoxes;

    for (size_t i = 0; i < output_count; ++i) {
        validCount += post_process((uint8_t *)output[i].buf, pArgs, i, filterBoxes, objProbs, classId);
    }

    if (validCount <= 0) {
        return -1;
    }

    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i) {
        indexArray.push_back(i);
    }

    quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));
    for (auto c : class_set) {
        nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
    }

    int last_count = 0;
    int model_in_w = modelWidth;
    int model_in_h =modelHeight;
    float scale_w = (float)modelWidth / (float)imageWidth;
    float scale_h = (float)modelHeight / (float)imageHeight;

    for (int i = 0; i < validCount; ++i) {
        if ((indexArray[i] == -1) || (last_count >= OBJ_NUMB_MAX_SIZE)) {
            continue;
        }

        int n = indexArray[i];
        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        float obj_conf = objProbs[i];

        std::vector<int>::iterator iter = find(filterClass.begin(), filterClass.end(), id);
        if (iter == filterClass.end()) {
            bbox box;
            box.classify = id;
            box.score = box.prob = obj_conf;
            box.top = (float)(clamp(y1, 0, model_in_h) / scale_h);
            box.left = (float)(clamp(x1, 0, model_in_w) / scale_w);
            box.right = (float)(clamp(x2, 0, model_in_w) / scale_w);
            box.bottom = (float)(clamp(y2, 0, model_in_h) / scale_h);

            if ((int(box.right - box.left) < 2) || (int(box.bottom - box.top) < 2)) {
                continue;
            }

            auto it = lastResults.find(box.classify);
            if (it != lastResults.end()) {
                it->second.emplace_back(box);
            } else {
                lastResults.insert(std::make_pair(box.classify, std::vector<bbox>{box}));
            }
        }

        last_count++;
    }

    return 0;
}

API_END_NAMESPACE(Ai)
