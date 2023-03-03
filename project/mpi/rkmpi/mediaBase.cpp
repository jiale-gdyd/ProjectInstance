#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

media::MediaRegion *MediaBase::mRegion = nullptr;
media::MediaInterface *MediaBase::mIMedia = nullptr;

MediaBase::MediaBase()
{

}

MediaBase::~MediaBase()
{
    if (mIMedia) {
        mIMedia->getSys().deinit();
        delete mIMedia;
        mIMedia = nullptr;
    }

    if (mRegion) {
        delete mRegion;
        mRegion = nullptr;
    }
}

media::MediaInterface *MediaBase::getMedia()
{
    if (mIMedia == nullptr) {
        mIMedia = new media::MediaInterface();
        mIMedia->getSys().init();
    }

    return mIMedia;
}

media::MediaRegion *MediaBase::getRegion()
{
    if (mRegion == nullptr) {
        mRegion = new media::MediaRegion();
    }

    return mRegion;
}

API_END_NAMESPACE(media)
