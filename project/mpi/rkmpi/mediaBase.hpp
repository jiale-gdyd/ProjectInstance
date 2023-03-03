#ifndef ROCKCHIP_MEDIABASE_HPP
#define ROCKCHIP_MEDIABASE_HPP

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "mediaVo.hpp"
#include "mediaVin.hpp"
#include "mediaRga.hpp"
#include "mediaSys.hpp"
#include "mediaVenc.hpp"
#include "mediaVdec.hpp"
#include "mediaVmix.hpp"
#include "mediaRegion.hpp"

#include <media/drm_media_api.h>
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaInterface {
public:
    MediaInterface() = default;
    ~MediaInterface() = default;

public:
    MediaVo &getVo() {
        return mMediaVo;
    }

    MediaVin &getVi() {
        return mMediaVin;
    }

    MediaRga &getRga() {
        return mMediaRga;
    }

    MediaSys &getSys() {
        return mMediaSys;
    }

    MediaVenc &getVenc() {
        return mMediaVenc;
    }

    MediaVdec &getVdec() {
        return mMediaVdec;
    }

    MediaVmix &getVmix() {
        return mMediaVmix;
    }

private:
    MediaVo   mMediaVo;
    MediaVin  mMediaVin;
    MediaRga  mMediaRga;
    MediaSys  mMediaSys;
    MediaVenc mMediaVenc;
    MediaVdec mMediaVdec;
    MediaVmix mMediaVmix;
};

class API_EXPORT MediaBase {
public:
    MediaBase();
    virtual ~MediaBase();

    virtual int init() = 0;

public:
    media::MediaRegion *getRegion();
    media::MediaInterface *getMedia();

private:
    static media::MediaRegion *mRegion;
    static media::MediaInterface *mIMedia;
};

API_END_NAMESPACE(media)

#endif
