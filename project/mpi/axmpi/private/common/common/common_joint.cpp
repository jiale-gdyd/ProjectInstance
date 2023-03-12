#include "common_joint.hpp"

namespace axpi {
int common_axjoint_init(axjoint_models_t *handler, int image_width, int image_height)
{
    if (handler->runJoint == true) {
        int s32Ret = axjoint_init(handler->modelPath, &handler->majorModel.jointHandle, &handler->majorModel.jointAttr);
        if (0 != s32Ret) {
            axmpi_error("axjoint_init failed, return:[%d]", s32Ret);
            return -1;
        }

        axmpi_info("load model:[%s] success, input resulotion width:[%d] height:[%d]", handler->modelPath.c_str(), handler->majorModel.jointAttr.width, handler->majorModel.jointAttr.height);

        handler->algoFmt = handler->majorModel.jointAttr.format;
        handler->algoWidth = handler->majorModel.jointAttr.width;
        handler->algoHeight = handler->majorModel.jointAttr.height;

        switch (handler->modelTypeMain) {
            case MT_MLM_HUMAN_POSE_AXPPL:
            case MT_MLM_HUMAN_POSE_HRNET:
            case MT_MLM_ANIMAL_POSE_HRNET:
            case MT_MLM_HAND_POSE:
            case MT_MLM_FACE_RECOGNITION:
            case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
                s32Ret = axjoint_init(handler->modelPathL2, &handler->minorModel.jointHandle, &handler->minorModel.jointAttr);
                if (0 != s32Ret) {
                    axmpi_error("pose:axjoint_init failed, return:[%d]", s32Ret);
                    return -1;
                }

                axmpi_info("load l2 model:[%s] success, input resulotion width:[%d] height:[%d]", handler->modelPathL2.c_str(), handler->minorModel.jointAttr.width, handler->minorModel.jointAttr.height);
                break;

            default:
                handler->ivpsAlgoWidth = handler->majorModel.jointAttr.width;
                handler->ivpsAlgoHeight = handler->majorModel.jointAttr.height;
                break;
        }

        switch (handler->modelTypeMain) {
            case MT_MLM_HUMAN_POSE_AXPPL:
            case MT_MLM_HUMAN_POSE_HRNET:
            case MT_MLM_ANIMAL_POSE_HRNET:
            case MT_MLM_HAND_POSE:
            case MT_MLM_FACE_RECOGNITION:
            case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
                handler->restoreWidth = handler->ivpsAlgoWidth;
                handler->restoreHeight = handler->ivpsAlgoHeight;
                break;

            default:
                handler->restoreWidth = image_width;
                handler->restoreHeight = image_height;
                break;
        }
    } else {
        axmpi_error("Not specified model file");
    }

    return 0;
}

int common_axjoint_exit(axjoint_models_t *handler)
{
    if (handler->runJoint == true) {
        axjoint_exit(handler->majorModel.jointHandle);
        axjoint_exit(handler->minorModel.jointHandle);
    }

    return 0;
}
}
