#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaBase.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaBase::MediaBase(MediaInterface *IMedia) : mIMedia(IMedia)
{

}

MediaBase::~MediaBase()
{

}

API_END_NAMESPACE(media)
