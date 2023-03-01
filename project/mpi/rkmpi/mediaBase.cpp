#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

media::MediaInterface *MediaBase::mInstance = nullptr;

MediaBase::MediaBase()
{

}

MediaBase::~MediaBase()
{
    if (mInstance) {
        mInstance->getSys().deinit();
        delete mInstance;
        mInstance = nullptr;
    }
}

media::MediaInterface *MediaBase::getMedia()
{
    if (mInstance == nullptr) {
        mInstance = new media::MediaInterface();
        mInstance->getSys().init();
    }

    return mInstance;
}

API_END_NAMESPACE(media)
