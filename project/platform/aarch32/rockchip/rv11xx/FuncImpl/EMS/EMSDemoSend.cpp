#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::sendRawFrame(media_buffer_t &mediaFrame, bool bDisplay, bool bFreeFrame, int voChn, int zPos, drm_plane_type_e enDispLayer, size_t dispWidth, size_t dispHeight, size_t dispXoffset, size_t dispYoffset, bool bDispSwap, drm_image_type_e enDispType)
{
    if (bDisplay) {
        if (getMedia()->getVo().voChnStart(voChn)) {
            if (mediaFrame) {
                getMedia()->getVo().sendFrame(voChn, mediaFrame);
            }
        } else {
            getMedia()->getVo().createVoChn(voChn, zPos, enDispLayer, dispWidth, dispHeight, dispXoffset, dispYoffset, bDispSwap, enDispType, mDisplayDeviceCard.c_str());
            if (getMedia()->getVo().voChnStart(voChn)) {
                if (mediaFrame) {
                    getMedia()->getVo().sendFrame(voChn, mediaFrame);
                }
            }
        }
    }

    if (bFreeFrame) {
        if (mediaFrame) {
            getMedia()->getVo().releaseFrame(mediaFrame);
        }
        mediaFrame = NULL;
    }

    if (!bDisplay) {
        if (getMedia()->getVo().voChnStart(voChn)) {
            getMedia()->getVo().destroyVoChn(voChn);
        }
    }
}

void EMSDemoImpl::sendZoomFrame(media_buffer_t &mediaFrame, int voChn, bool bDisplay, int rgaCropChn, int rgaZoomChn)
{
    if (mediaFrame == NULL) {
        return;
    }

    getMedia()->getRga().sendFrame(mRtspVencRgaChn, mediaFrame);

    if (bDisplay) {
        if (getMedia()->getVo().voChnStart(voChn)) {
            if (getMedia()->getRga().rgaChnStart(rgaCropChn) && getMedia()->getRga().rgaChnStart(rgaZoomChn)) {
                getMedia()->getVo().sendZoomVoChnWithBind(mediaFrame, voChn, rgaCropChn, rgaZoomChn);
            }
        }
    }

    getMedia()->getVo().releaseFrame(mediaFrame);
    mediaFrame = NULL;
}

API_END_NAMESPACE(EMS)
