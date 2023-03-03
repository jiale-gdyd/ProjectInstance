#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

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
}

media::MediaInterface *MediaBase::getMedia()
{
    if (mIMedia == nullptr) {
        mIMedia = new media::MediaInterface();
        mIMedia->getSys().init();
    }

    return mIMedia;
}

API_END_NAMESPACE(media)
