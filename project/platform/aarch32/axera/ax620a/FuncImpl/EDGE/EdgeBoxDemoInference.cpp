#include "EdgeBoxDemo.hpp"

namespace edge {
void EdgeBoxDemo::inferenceProcess(axpi::axpipe_buffer_t *buff, void *user_data, void *user_data2)
{
    static axpi::axpi_results_t results;
    int threadId = POINTER_TO_INT(user_data2);
    static EdgeBoxDemo *me = reinterpret_cast<EdgeBoxDemo *>(user_data);

    if (me->mAlgoInit) {
        axpi::axpi_image_t srcFrame = {0};
        switch (buff->dataType) {
            case axpi::AXPIPE_OUTPUT_BUFF_NV12:
                srcFrame.dtype = axpi::AXPI_COLOR_SPACE_NV12;
                break;

            case axpi::AXPIPE_OUTPUT_BUFF_BGR:
                srcFrame.dtype = axpi::AXPI_COLOR_SPACE_BGR;
                break;

            case axpi::AXPIPE_OUTPUT_BUFF_RGB:
                srcFrame.dtype = axpi::AXPI_COLOR_SPACE_RGB;
                break;

            default:
                break;
        }

        srcFrame.width = buff->width;
        srcFrame.height = buff->height;
        srcFrame.vir = buff->virAddr;
        srcFrame.phy = buff->phyAddr;
        srcFrame.strideW = buff->stride;
        srcFrame.size = buff->size;

        axpi::axpi_inference(me->mHandler, &srcFrame, &results);
        me->mJointResultsRing[threadId].insert(results);
    }
}
}
