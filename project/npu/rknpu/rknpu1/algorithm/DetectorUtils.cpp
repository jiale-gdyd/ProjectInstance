#include "AlgoDetector.hpp"

API_BEGIN_NAMESPACE(Ai)

void AlgoDetector::nms(std::vector<Ai::bbox> &input_boxes, float threshold)
{
    float t, l, b, r;
    float iou, dw, dh, sizeA;
    std::sort(input_boxes.begin(), input_boxes.end(), [](const bbox &box1, const bbox &box2) {
        return (box1.score > box2.score);
    });

    for (int i = 0; i < int(input_boxes.size()); ++i) {
        for (int j = i + 1; j < int(input_boxes.size()); ) {
            t = std::max(input_boxes[i].top, input_boxes[j].top);
            l = std::max(input_boxes[i].left, input_boxes[j].left);
            r = std::min(input_boxes[i].right, input_boxes[j].right);
            b = std::min(input_boxes[i].bottom, input_boxes[j].bottom);

            dw = r - l;
            dh = b - t;

            if ((dw > 0) && (dh > 0)) {
                sizeA = (input_boxes.at(i).right - input_boxes.at(i).left) * (input_boxes.at(i).bottom - input_boxes.at(i).top);
                iou = (dw * dh) / (sizeA + sizeA - (dw * dh));
            } else {
                iou = 0;
            }

            if (iou >= threshold) {
                input_boxes.erase(input_boxes.begin() + j);
            } else {
                j++;
            }
        }
    }
}

int AlgoDetector::post_process(uint8_t *input, algo_args_t *args, int index, std::vector<float> &boxes, std::vector<float> &objProbs, std::vector<int> &classId)
{
    int anchor[6];
    int validCount = 0;
    int classes = args->classCount;
    int width = args->modelInputWidth;
    int height = args->modelInputHeight;
    float threshold = args->conf_threshold;
    uint32_t zp = args->outputsAttr[index].zp;
    float scale = args->outputsAttr[index].scale;
    int stride = args->yoloAnchors[index].stride;

    int grid_w = width / stride;
    int grid_h = height / stride;
    int grid_len = grid_h * grid_w;
    float thres = unsigmoid(threshold);
    uint8_t thres_u8 = qnt_f32_to_affine(thres, zp, scale);

    for (size_t k = 0; k < 6; k += 2) {
        anchor[k + 0] = args->yoloAnchors[index].anchors.at(k / 2).width;
        anchor[k + 1] = args->yoloAnchors[index].anchors.at(k / 2).height;
    }

    for (int a = 0; a < 3; ++a) {
        for (int i = 0; i < grid_h; ++i) {
            for (int j = 0; j < grid_w; ++j) {
                uint8_t box_confidence = input[((classes + 5) * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence > thres_u8) {
                    int offset = ((classes + 5) * a) * grid_len + i * grid_w + j;
                    uint8_t *in_ptr = input + offset;
                    float box_x = sigmoid(deqnt_affine_to_f32(*in_ptr, zp, scale)) * 2.0 - 0.5;
                    float box_y = sigmoid(deqnt_affine_to_f32(in_ptr[grid_len], zp, scale)) * 2.0 - 0.5;
                    float box_w = sigmoid(deqnt_affine_to_f32(in_ptr[2 * grid_len], zp, scale)) * 2.0;
                    float box_h = sigmoid(deqnt_affine_to_f32(in_ptr[3 * grid_len], zp, scale)) * 2.0;

                    box_x = (box_x + j) * (float)stride;
                    box_y = (box_y + i) * (float)stride;
                    box_w = box_w * box_w * (float)anchor[a * 2 + 0];
                    box_h = box_h * box_h * (float)anchor[a * 2 + 1];
                    box_x -= (box_w / 2.0);
                    box_y -= (box_h / 2.0);
                    boxes.push_back(box_x);
                    boxes.push_back(box_y);
                    boxes.push_back(box_w);
                    boxes.push_back(box_h);

                    int maxClassId = 0;
                    uint8_t maxClassProbs = in_ptr[5 * grid_len];

                    for (int k = 1; k < classes; ++k) {
                        uint8_t prob = in_ptr[(5 + k) * grid_len];
                        if (prob > maxClassProbs) {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }

                    float deqnt_cls_conf = sigmoid(deqnt_affine_to_f32(maxClassProbs, zp, scale));
                    if (deqnt_cls_conf > threshold) {
                        float deqnt_box_conf = sigmoid(deqnt_affine_to_f32(box_confidence, zp, scale));

                        objProbs.push_back(deqnt_box_conf * deqnt_cls_conf);
                        classId.push_back(maxClassId);
                        validCount++;
                    }
                }
            }
        }
    }

    return validCount;
}

int AlgoDetector::quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
{
    float key;
    int key_index;
    int low = left;
    int high = right;

    if (left < right) {
        key_index = indices[left];
        key = input[left];
        while (low < high) {
            while ((low < high) && (input[high] <= key)) {
                high--;
            }

            input[low] = input[high];
            indices[low] = indices[high];
            while ((low < high) && (input[low] >= key)) {
                low++;
            }

            input[high] = input[low];
            indices[high] = indices[low];
        }

        input[low] = key;
        indices[low] = key_index;

        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }

    return low;
}

int AlgoDetector::nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order, int filterId, float nms_threshold)
{
    for (int i = 0; i < validCount; ++i) {
        if ((order[i] == -1) || (classIds[i] != filterId)) {
            continue;
        }

        int n = order[i];
        for (int j = i + 1; j < validCount; ++j) {
            int m = order[j];
            if ((m == -1) || (classIds[i] != filterId)) {
                continue;
            }

            float xmin0 = outputLocations[n * 4 + 0];
            float ymin0 = outputLocations[n * 4 + 1];
            float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
            float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

            float xmin1 = outputLocations[m * 4 + 0];
            float ymin1 = outputLocations[m * 4 + 1];
            float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
            float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);
            if (iou > nms_threshold) {
                order[j] = -1;
            }
        }
    }

    return 0;
}

template<typename T>
std::vector<unsigned int> AlgoDetector::argsort(const std::vector<T> array)
{
    std::vector<unsigned int> indices(array.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [array](int left, int right) -> bool {
        // 根据对应的数组元素对索引进行排序
        return array[left] < array[right];
    });

    return indices;
}

void AlgoDetector::compute_iou(std::vector<std::vector<float>> boxes, std::vector<unsigned int> scores_indexes, std::vector<float> areas, int index, std::vector<float> &ious)
{
    std::vector<float> iouys1(scores_indexes.size());
    std::vector<float> iouxs1(scores_indexes.size());
    std::vector<float> iouys2(scores_indexes.size());
    std::vector<float> iouxs2(scores_indexes.size());
    std::vector<float> boxes_area(scores_indexes.size());

    float box_area = areas[index];
    std::vector<float> unions(scores_indexes.size());

    std::vector<float> intersections(scores_indexes.size());
    std::vector<float> preintersections1(scores_indexes.size(), 0);
    std::vector<float> preintersections2(scores_indexes.size(), 0);

    for (size_t j = 0; j < scores_indexes.size(); j++) {
        iouys1[j] = boxes[index][0];
        iouxs1[j] = boxes[index][1]; 
        iouys2[j] = boxes[index][2];
        iouxs2[j] = boxes[index][3];
    }
 
    for (size_t j = 0; j < scores_indexes.size(); j++) {
        if (iouys1[j] < boxes[scores_indexes[j]][0]) {
            iouys1[j] = boxes[scores_indexes[j]][0];
        }

        if (iouxs1[j] < boxes[scores_indexes[j]][1]) {
            iouxs1[j] = boxes[scores_indexes[j]][1];
        }

        if (iouys2[j] > boxes[scores_indexes[j]][2]) {
            iouys2[j] = boxes[scores_indexes[j]][2];
        }

        if (iouxs2[j] > boxes[scores_indexes[j]][3]) {
            iouxs2[j] = boxes[scores_indexes[j]][3];
        }

        boxes_area[j] = areas[scores_indexes[j]];
    }

    for (size_t j = 0; j < scores_indexes.size(); j++) {
        if (preintersections1[j] < (iouys2[j] - iouys1[j])) {
            preintersections1[j] = iouys2[j] - iouys1[j];
        }

        if (preintersections2[j] < (iouxs2[j] - iouxs1[j])) {
            preintersections2[j] = iouxs2[j] - iouxs1[j];
        }
    }

    for (size_t j = 0; j < scores_indexes.size(); j++) {
        intersections[j] = preintersections1[j] * preintersections2[j];
    }

    for (size_t j = 0; j < scores_indexes.size(); j++) {
        unions[j] = box_area + boxes_area[j] - intersections[j];
    }

    for (size_t j = 0; j < scores_indexes.size(); j++) {
        ious[j] = intersections[j] / unions[j];
    }
}

std::vector<unsigned int> AlgoDetector::non_max_suppression(std::vector<std::vector<float>> boxes, std::vector<float> scores_vector, float threshold, int size)
{
    int index = 0;
    std::vector<float> ys1(size);
    std::vector<float> xs1(size);
    std::vector<float> ys2(size);
    std::vector<float> xs2(size);
    std::vector<float> areas(size);

    for (int i = 0; i < size; i++) {
        ys1[i] = boxes[i][0];
        xs1[i] = boxes[i][1];
        ys2[i] = boxes[i][2];
        xs2[i] = boxes[i][3];
        areas[i] = (ys2[i] - ys1[i]) * (xs2[i] - xs1[i]);
    }

    std::vector<unsigned int> boxes_keep_index;
    std::vector<unsigned int> scores_indexes = argsort(scores_vector);

    while (scores_indexes.size() > 0) {
        index = scores_indexes.back();
        scores_indexes.pop_back();
        boxes_keep_index.push_back(index);
        if (scores_indexes.size() == 0) {
            break;
        }

        std::vector<float> ious(scores_indexes.size());
        compute_iou(boxes, scores_indexes, areas, index, ious);
        std::vector<float> filtered_indexes;
        for (size_t j = 0; j < scores_indexes.size(); j++){
            if (ious[j] > threshold) {
                filtered_indexes.push_back(j);
            }
        }

        std::vector<unsigned int> scores_indexes_temp;
        for (size_t j = 0; j <scores_indexes.size(); j++) {
            bool isIn = true;
            for (size_t z = 0; z < filtered_indexes.size(); z++){
                if (j == filtered_indexes[z]) {
                    isIn = false;
                }
            }

            if (isIn) {
                scores_indexes_temp.push_back(scores_indexes[j]);
            }
        }
        
        scores_indexes = scores_indexes_temp;
    }

    return boxes_keep_index;
}

API_END_NAMESPACE(Ai)
