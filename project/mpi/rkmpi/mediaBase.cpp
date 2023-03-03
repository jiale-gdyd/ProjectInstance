#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

media::MediaApi *MediaBase::mMediaApi = nullptr;

MediaBase::MediaBase()
{

}

MediaBase::~MediaBase()
{
    if (mMediaApi) {
        mMediaApi->getSys().deinit();
        delete mMediaApi;
        mMediaApi = nullptr;
    }
}

media::MediaApi *MediaBase::getApi()
{
    if (mMediaApi == nullptr) {
        mMediaApi = new media::MediaApi();
        mMediaApi->getSys().init();
    }

    return mMediaApi;
}

API_END_NAMESPACE(media)
