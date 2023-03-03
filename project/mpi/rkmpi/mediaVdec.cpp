#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/prctl.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "../private.h"
#include "mediaVdec.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

typedef struct stVdecParam {
    int         vdecChn;                        // 解码通道号
    int         enCodecType;                    // 解码文件类型
    bool        decode_loop;                    // 周期性解码
    bool        hardwareAlloc;                  // 硬件分配
    size_t      allocFlag;                      // 分配标志
    size_t      decIntervalMs;                  // 解码周期
    size_t      decOneFrameSize;                // 解码一帧大小
    std::string decodeFile;                     // 解码文件
    void        *user_data;                     // 用户参数信息
} video_codec_param_t;

typedef struct stVdecParamMulti {
    int                      vdecChn;            // 解码通道号
    int                      enCodecType;        // 解码文件类型
    bool                     decode_loop;        // 周期性解码
    bool                     hardwareAlloc;      // 硬件分配
    size_t                   allocFlag;          // 分配标志
    size_t                   decIntervalMs;      // 解码周期
    size_t                   decOneFrameSize;    // 解码一帧大小
    std::vector<std::string> decodeFile;         // 解码文件
    void                     *user_data;         // 用户参数信息
} video_codec_param_multi_t;

MediaVdec::MediaVdec()
{
    mVdecThreadQuit = false;
    for (int i = DRM_VDEC_CHANNEL_00; i < DRM_VDEC_CHANNEL_BUTT; ++i) {
        mVdecThreadId[i] = 0;
        mVdecChnState[i] = VDEC_QUIT;
        mDecodeFileCount[i] = 0;
        mCurrDecodeFileIndex[i] = 0;
    }
}

MediaVdec::~MediaVdec()
{
    mVdecThreadQuit = true;
    for (int i = DRM_VDEC_CHANNEL_00; i < DRM_VDEC_CHANNEL_BUTT; ++i) {
        destroyVdecChn(i);
        if (mVdecThreadId[i]) {
            pthread_join(mVdecThreadId[i], NULL);
            mVdecThreadId[i] = 0;
        }

        mVdecChnState[i] = VDEC_QUIT;
        mDecodeFileCount[i] = 0;
        mCurrDecodeFileIndex[i] = 0;
    }
}

int MediaVdec::destroyVdecChn(int vdecChn)
{
    if ((vdecChn < DRM_VDEC_CHANNEL_00) || (vdecChn >= DRM_VDEC_CHANNEL_BUTT)) {
        return -1;
    }

    if (mVdecChnState[vdecChn] == VDEC_QUIT) {
        return 0;
    }

    int ret = drm_mpi_vdec_destroy_channel(vdecChn);
    if (ret) {
        return ret;
    }

    return 0;
}

int MediaVdec::createVdecChn(int vdecChn, drm_codec_type_e enCodecType)
{
    if ((vdecChn < DRM_VDEC_CHANNEL_00) || (vdecChn >= DRM_VDEC_CHANNEL_BUTT)) {
        mpi_error("invalid vdec channel vdec:[%d]", vdecChn);
        return -1;
    }

    if ((enCodecType != DRM_CODEC_TYPE_H264) && (enCodecType != DRM_CODEC_TYPE_H265) && (enCodecType != DRM_CODEC_TYPE_JPEG) && (enCodecType != DRM_CODEC_TYPE_MJPEG)) {
        mpi_error("enCodecType only support DRM_CODEC_TYPE_H264/DRM_CODEC_TYPE_H265/DRM_CODEC_TYPE_JPEG/DRM_CODEC_TYPE_MJPEG");
        return -1;
    }

    if (mVdecChnState[vdecChn] == VDEC_START) {
        mpi_error("vdec channel vdec:[%d] had started", vdecChn);
        return -1;
    }

    int ret = -1;
    drm_vdec_chn_attr_t stVdecChnAttr;

    memset(&stVdecChnAttr, 0x00, sizeof(stVdecChnAttr));
    stVdecChnAttr.enCodecType = enCodecType;
    stVdecChnAttr.enMode = VIDEO_MODE_STREAM;
    stVdecChnAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
    if ((enCodecType == DRM_CODEC_TYPE_JPEG) || (enCodecType == DRM_CODEC_TYPE_MJPEG)) {
        stVdecChnAttr.enMode = VIDEO_MODE_FRAME;
    }

    ret = drm_mpi_vdec_create_channel(vdecChn, (const drm_vdec_chn_attr_t *)&stVdecChnAttr);
    if (ret) {
        mpi_error("create vdec channel vdec:[%d] failed, return:[%d]", vdecChn, ret);
        return ret;
    }

    mVdecChnState[vdecChn] = VDEC_START;
    return 0;
}

int MediaVdec::createVdecChn(int vdecChn, std::string fileName, drm_codec_type_e enCodecType, bool decLoop, size_t decOneFrameSize, size_t decInterMs, bool bHardware, size_t allocFlag)
{
    if ((vdecChn < DRM_VDEC_CHANNEL_00) || (vdecChn >= DRM_VDEC_CHANNEL_BUTT)) {
        mpi_error("invalid vdec channel vdec:[%d]", vdecChn);
        return -1;
    }

    if (mVdecChnState[vdecChn] == VDEC_START) {
        mpi_error("vdec channel vdec:[%d] had started", vdecChn);
        return -1;
    }

    if (access(fileName.c_str(), F_OK) != 0) {
        mpi_error("vdec channel:[%d] file:[%s] not found", vdecChn, fileName.c_str());
        return -1;
    }

    if ((enCodecType != DRM_CODEC_TYPE_H264) && (enCodecType != DRM_CODEC_TYPE_H265) && (enCodecType != DRM_CODEC_TYPE_JPEG) && (enCodecType != DRM_CODEC_TYPE_MJPEG)) {
        mpi_error("enCodecType only support DRM_CODEC_TYPE_H264/DRM_CODEC_TYPE_H265/DRM_CODEC_TYPE_JPEG/DRM_CODEC_TYPE_MJPEG");
        return -1;
    }

    static video_codec_param_t param;
    param.vdecChn = vdecChn;
    param.enCodecType = enCodecType;
    param.decode_loop = decLoop;
    param.hardwareAlloc = bHardware;
    param.allocFlag = allocFlag;
    param.decIntervalMs = decInterMs;
    param.decOneFrameSize = decOneFrameSize;
    param.decodeFile = fileName;
    param.user_data = this;
    mDecodeFileCount[vdecChn] = 1;

    if (pthread_create(&mVdecThreadId[vdecChn], NULL, videoDecoderProcessThread, (void *)&param) != 0) {
        mpi_error("vdec channel vdec:[%d] create failed", vdecChn);
        return -1;
    }

    return 0;
}

int MediaVdec::createVdecChn(int vdecChn, std::vector<std::string> fileName, drm_codec_type_e enCodecType, bool decLoop, size_t decOneFrameSize, size_t decInterMs, bool bHardware, size_t allocFlag)
{
    if ((vdecChn < DRM_VDEC_CHANNEL_00) || (vdecChn >= DRM_VDEC_CHANNEL_BUTT)) {
        mpi_error("invalid vdec channel vdec:[%d]", vdecChn);
        return -1;
    }

    if (mVdecChnState[vdecChn] == VDEC_START) {
        mpi_error("vdec channel vdec:[%d] had started", vdecChn);
        return -1;
    }

    if (fileName.size() == 0) {
        mpi_error("vdec channel:[%d] file count:[%u]", vdecChn, fileName.size());
        return -1;
    }

    if ((enCodecType != DRM_CODEC_TYPE_H264) && (enCodecType != DRM_CODEC_TYPE_H265) && (enCodecType != DRM_CODEC_TYPE_JPEG) && (enCodecType != DRM_CODEC_TYPE_MJPEG)) {
        mpi_error("enCodecType only support DRM_CODEC_TYPE_H264/DRM_CODEC_TYPE_H265/DRM_CODEC_TYPE_JPEG/DRM_CODEC_TYPE_MJPEG");
        return -1;
    }

    std::vector<std::string> decodeFileName;
    for (size_t i = 0; i < fileName.size(); ++i) {
        if (access(fileName[i].c_str(), F_OK) == 0) {
            decodeFileName.emplace_back(fileName[i]);
            mDecodeFileCount[vdecChn] += 1;
        }
    }

    if (mDecodeFileCount[vdecChn] == 0) {
        mpi_error("vdec channel:[%d] file count:[%d]", vdecChn, mDecodeFileCount[vdecChn]);
        return -1;
    }

    static video_codec_param_multi_t param;
    param.vdecChn = vdecChn;
    param.enCodecType = enCodecType;
    param.decode_loop = decLoop;
    param.hardwareAlloc = bHardware;
    param.allocFlag = allocFlag;
    param.decIntervalMs = decInterMs;
    param.decOneFrameSize = decOneFrameSize;
    param.decodeFile = decodeFileName;
    param.user_data = this;

    if (pthread_create(&mVdecThreadId[vdecChn], NULL, videoDecoderProcessThreadMulti, (void *)&param) != 0) {
        mpi_error("vdec channel vdec:[%d] create failed", vdecChn);
        return -1;
    }

    return 0;
}

void *MediaVdec::videoDecoderProcessThread(void *arg)
{
    int ret = 0;
    FILE *fp = NULL;
    size_t data_size = 0;
    size_t read_size = 0;
    drm_vdec_chn_attr_t stVdecAttr;
    video_codec_param_t *vdecParam = reinterpret_cast<video_codec_param_t *>(arg);
    MediaVdec *me = reinterpret_cast<MediaVdec *>(vdecParam->user_data);

    int vdec_chn = vdecParam->vdecChn;
    bool bDecLoop = vdecParam->decode_loop;
    size_t memAllocFlags = vdecParam->allocFlag;
    bool bUseHardwareMem = vdecParam->hardwareAlloc;
    size_t inBufSize = vdecParam->decOneFrameSize <= 0 ? 6144 : vdecParam->decOneFrameSize;
    size_t decInterMs = vdecParam->decIntervalMs <= 0 ? 10 * 1000 : vdecParam->decIntervalMs * 1000;

    std::string threadName = "xdlib-vdec" + std::to_string(vdec_chn);
    pthread_setname_np(pthread_self(), threadName.c_str());

    fp = fopen(vdecParam->decodeFile.c_str(), "rb");
    if (!fp) {
        mpi_error("vdec chn:[%d] open file:[%s] failed", vdec_chn, vdecParam->decodeFile.c_str());
        return NULL;
    }

    memset(&stVdecAttr, 0x00, sizeof(stVdecAttr));
    stVdecAttr.enCodecType = (drm_codec_type_e)vdecParam->enCodecType;
    stVdecAttr.enMode = VIDEO_MODE_STREAM;
    stVdecAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
    if ((stVdecAttr.enCodecType == DRM_CODEC_TYPE_JPEG) || (stVdecAttr.enCodecType == DRM_CODEC_TYPE_MJPEG)) {
        stVdecAttr.enMode = VIDEO_MODE_FRAME;
    } else {
        stVdecAttr.enMode = VIDEO_MODE_STREAM;
    }

    ret = drm_mpi_vdec_create_channel(vdec_chn, (const drm_vdec_chn_attr_t *)&stVdecAttr);
    if (ret) {
        mpi_error("create vdec channel:[%d] failed, return:[%d]", vdec_chn, ret);
        return NULL;
    }

    me->mVdecChnState[vdec_chn] = VDEC_START;

    if (stVdecAttr.enMode == VIDEO_MODE_STREAM) {
        data_size = inBufSize;
    } else if (stVdecAttr.enMode == VIDEO_MODE_FRAME) {
        fseek(fp, 0, SEEK_END);
        data_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    while (!me->mVdecThreadQuit) {
        if (me->mVdecChnState[vdec_chn] == VDEC_START) {
            media_buffer_t mediaFrame = me->createFrame(data_size, bUseHardwareMem, (unsigned char)memAllocFlags);
VDEC_LOOP0:
            if (mediaFrame) {
                read_size = fread(me->getFrameDataPtr(mediaFrame), 1, data_size, fp);
                if (!read_size || feof(fp)) {
                    if (bDecLoop) {
                        fseek(fp, 0, SEEK_SET);
                        goto VDEC_LOOP0;
                    } else {
                        me->releaseFrame(mediaFrame);
                        break;
                    }
                }

                me->setFrameSize(mediaFrame, read_size);
                me->sendFrame(vdec_chn, mediaFrame, MOD_ID_VDEC);
                me->releaseFrame(mediaFrame);
            } else {
                mpi_error("create media frame failed");
            }
        }

        usleep(decInterMs);
    }

    if (fp) { fclose(fp); }
    me->mVdecChnState[vdec_chn] = VDEC_STOP;
    pthread_exit(NULL);
}

void *MediaVdec::videoDecoderProcessThreadMulti(void *arg)
{
    int ret = 0;
    FILE *fp = NULL;
    size_t data_size = 0;
    size_t read_size = 0;
    drm_vdec_chn_attr_t stVdecAttr;
    video_codec_param_multi_t *vdecParam = reinterpret_cast<video_codec_param_multi_t *>(arg);
    MediaVdec *me = reinterpret_cast<MediaVdec *>(vdecParam->user_data);

    int vdec_chn = vdecParam->vdecChn;
    bool bDecLoop = vdecParam->decode_loop;
    size_t memAllocFlags = vdecParam->allocFlag;
    bool bUseHardwareMem = vdecParam->hardwareAlloc;
    size_t inBufSize = vdecParam->decOneFrameSize <= 0 ? 6144 : vdecParam->decOneFrameSize;
    size_t decInterMs = vdecParam->decIntervalMs <= 0 ? 10 * 1000 : vdecParam->decIntervalMs * 1000;

    std::string threadName = "xdlib-mvdec" + std::to_string(vdec_chn);
    pthread_setname_np(pthread_self(), threadName.c_str());

    std::vector<std::string> decodeFile(vdecParam->decodeFile);
    fp = fopen(decodeFile[0].c_str(), "rb");
    if (!fp) {
        mpi_error("vdec chn:[%d] open file:[%s] failed", vdec_chn, decodeFile[0].c_str());
        return NULL;
    }

    memset(&stVdecAttr, 0x00, sizeof(stVdecAttr));
    stVdecAttr.enCodecType = (drm_codec_type_e)(vdecParam->enCodecType);
    stVdecAttr.enMode = VIDEO_MODE_STREAM;
    stVdecAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
    if ((stVdecAttr.enCodecType == DRM_CODEC_TYPE_JPEG) || (stVdecAttr.enCodecType == DRM_CODEC_TYPE_MJPEG)) {
        stVdecAttr.enMode = VIDEO_MODE_FRAME;
    } else {
        stVdecAttr.enMode = VIDEO_MODE_STREAM;
    }

    ret = drm_mpi_vdec_create_channel(vdec_chn, (const drm_vdec_chn_attr_t *)&stVdecAttr);
    if (ret) {
        mpi_error("create vdec channel:[%d] failed, return:[%d]", vdec_chn, ret);
        return NULL;
    }

    me->mVdecChnState[vdec_chn] = VDEC_START;

    if (stVdecAttr.enMode == VIDEO_MODE_STREAM) {
        data_size = inBufSize;
    } else if (stVdecAttr.enMode == VIDEO_MODE_FRAME) {
        fseek(fp, 0, SEEK_END);
        data_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }

    while (!me->mVdecThreadQuit) {
        if (me->mVdecChnState[vdec_chn] == VDEC_START) {
            media_buffer_t mediaFrame = me->createFrame(data_size, bUseHardwareMem, (unsigned char)memAllocFlags);
VDEC_LOOP1:
            if (mediaFrame) {
                read_size = fread(me->getFrameDataPtr(mediaFrame), 1, data_size, fp);
                if (!read_size || feof(fp)) {
                    if (me->mDecodeFileCount[vdec_chn] <= 1) {
                        if (bDecLoop) {
                            fseek(fp, 0, SEEK_SET);
                            goto VDEC_LOOP1;
                        } else {
                            me->releaseFrame(mediaFrame);
                            break;
                        }
                    } else {
OPEN_FILE_FAIL:
                        me->mCurrDecodeFileIndex[vdec_chn] += 1;
                        if ((me->mCurrDecodeFileIndex[vdec_chn] >= me->mDecodeFileCount[vdec_chn]) || (me->mCurrDecodeFileIndex[vdec_chn] <= 0)) {
                            me->mCurrDecodeFileIndex[vdec_chn] = 0;
                            if (!bDecLoop) {
                                me->releaseFrame(mediaFrame);
                                break;
                            }
                        }

                        fclose(fp);

                        int index = me->mCurrDecodeFileIndex[vdec_chn];
                        std::string decFile = decodeFile[index];
                        fp = fopen(decFile.c_str(), "rb");
                        if (!fp) {
                            mpi_error("vdec chn:[%d] open file:[%s] failed", vdec_chn, decFile.c_str());
                            goto OPEN_FILE_FAIL;
                        }

                        if (stVdecAttr.enMode == VIDEO_MODE_STREAM) {
                            data_size = inBufSize;
                        } else if (stVdecAttr.enMode == VIDEO_MODE_FRAME) {
                            fseek(fp, 0, SEEK_END);
                            data_size = ftell(fp);
                            fseek(fp, 0, SEEK_SET);
                        }
                    }
                }

                me->setFrameSize(mediaFrame, read_size);
                me->sendFrame(vdec_chn, mediaFrame, MOD_ID_VDEC);
                me->releaseFrame(mediaFrame);
            } else {
                mpi_error("create media frame failed");
            }
        }

        usleep(decInterMs);
    }

    if (fp) { fclose(fp); }
    me->mVdecChnState[vdec_chn] = VDEC_STOP;
    pthread_exit(NULL);
}

int MediaVdec::sendFrame(int modeIdChn, media_buffer_t pstFrameInfo, int modeId)
{
    return drm_mpi_system_send_media_buffer((mod_id_e)modeId, modeIdChn, pstFrameInfo);
}

media_buffer_t MediaVdec::getFrame(int modeIdChn, int s32MilliSec, int modeId)
{
    return drm_mpi_system_get_media_buffer((mod_id_e)modeId, modeIdChn, s32MilliSec);
}

void *MediaVdec::getFrameDataPtr(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

void *MediaVdec::getFrameData(media_buffer_t pstFrameInfo)
{
    return drm_mpi_mb_get_ptr(pstFrameInfo);
}

int MediaVdec::setFrameSize(media_buffer_t pstFrameInfo, unsigned int size)
{
    return drm_mpi_mb_set_size(pstFrameInfo, size);
}

media_buffer_t MediaVdec::createFrame(unsigned int size, bool bHardware, unsigned char flag)
{
    return drm_mpi_mb_create_buffer(size, bHardware, flag);
}

media_buffer_t MediaVdec::createImageFrame(mb_image_info_t *pstImageInfo, bool bHardware, unsigned char flag)
{
    return drm_mpi_mb_create_image_buffer(pstImageInfo, bHardware, flag);
}

int MediaVdec::releaseFrame(media_buffer_t pstFrameInfo)
{
    if (!pstFrameInfo) {
        return -1;
    }

    return drm_mpi_mb_release_buffer(pstFrameInfo);
}

API_END_NAMESPACE(media)
