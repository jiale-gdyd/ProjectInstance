#ifndef AXERA_MEDIABASE_HPP
#define AXERA_MEDIABASE_HPP

#define __AXERA_MEDIABASE_HPP_INSIDE__
#include "mediaVo.hpp"
#include "mediaVin.hpp"
#include "mediaSys.hpp"
#include "mediaIsp.hpp"
#include "mediaVdec.hpp"
#include "mediaVenc.hpp"
#include "mediaIpvs.hpp"
#include "mediaIves.hpp"
#undef __AXERA_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

class API_HIDDEN MediaApi {
public:
    MediaApi() = default;
    ~MediaApi() = default;

public:
    MediaVo &getVo() {
        return mMediaVo;
    }

    MediaVin &getVi() {
        return mMediaVin;
    }

    MediaSys &getSys() {
        return mMediaSys;
    }

    MediaIsp &getIsp() {
        return mMediaIsp;
    }

    MediaVdec &getVdec() {
        return mMediaVdec;
    }

    MediaVenc &getVenc() {
        return mMediaVenc;
    }

    MediaIpvs &getIpvs() {
        return mMediaIpvs;
    }

    MediaIves &getIves() {
        return mMediaIves;
    }

private:
    MediaVo   mMediaVo;
    MediaVin  mMediaVin;
    MediaSys  mMediaSys;
    MediaIsp  mMediaIsp;
    MediaVdec mMediaVdec;
    MediaVenc mMediaVenc;
    MediaIpvs mMediaIpvs;
    MediaIves mMediaIves;
};

class API_EXPORT MediaBase {
public:
    MediaBase();
    virtual ~MediaBase();

    // virtual int init() = 0;

public:
    media::MediaApi *getApi();

private:
    static media::MediaApi *mMediaApi;
};

API_END_NAMESPACE(media)

#endif
