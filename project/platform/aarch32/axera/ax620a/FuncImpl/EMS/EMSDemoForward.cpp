#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::inferenceProcess(media::axpipe_buffer_t *buff, void *user_data)
{
    Ai::axdl_frame_t frame;
    Ai::axdl_result_t lastResult;
    static EMSDemoImpl *me = reinterpret_cast<EMSDemoImpl *>(user_data);

    switch (buff->dataType) {
        case media::AXPIPE_OUTPUT_BUFF_NV12:
            frame.dtype = Ai::AXDL_COLOR_SPACE_NV12;
            break;

        case media::AXPIPE_OUTPUT_BUFF_BGR:
            frame.dtype = Ai::AXDL_COLOR_SPACE_BGR;
            break;

        case media::AXPIPE_OUTPUT_BUFF_RGB:
            frame.dtype = Ai::AXDL_COLOR_SPACE_RGB;
            break;

        default:
            break;
    }

    frame.width = buff->width;
    frame.height = buff->height;
    frame.vir = buff->virAddr;
    frame.phy = buff->phyAddr;
    frame.strideW = buff->stride;
    frame.size = buff->size;

    if (me->pDetector->forward(&frame, &lastResult) == 0) {
        me->mAlgoResultRing.insert(lastResult);
    }
}

API_END_NAMESPACE(EMS)
