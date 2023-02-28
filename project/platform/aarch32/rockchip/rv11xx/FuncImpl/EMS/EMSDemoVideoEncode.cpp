#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>

#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

static std::string getDateTime()
{
    struct tm *ptm;
    struct timeb stTimeb;
    static char buf[64] = {0};

    ftime(&stTimeb);
    ptm = localtime(&stTimeb.time);

    memset(buf, 0x00, sizeof(buf));
    sprintf(buf, "%04d%02d%02d%02d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

    return std::string(buf);
}

static std::string getDateAndCreate(std::string rootDir)
{
    int ret = -1;
    std::string dateTime = getDateTime();
    std::string date = dateTime.substr(0, 8);
    std::string saveDir = rootDir + date;

    if (access(rootDir.c_str(), F_OK) == 0) {
        if (access(saveDir.c_str(), F_OK) < 0) {
            ret = mkdir(saveDir.c_str(), 0666);
            if (ret == 0) {
                return saveDir;
            }

            return "";
        }

        return saveDir;
    }

    return "";
}

int EMSDemoImpl::videoEncodeProcessCallback(media_buffer_t mediaFrame, void *user_data)
{
    static int saveFrameCount = 0;
    static bool bLockFileName = false;
    static std::string saveEncFile = "";
    static bool bFindTFCardFirst = false;
    static std::string saveBKEncFile = "";
    static EMSDemoImpl *me = reinterpret_cast<EMSDemoImpl *>(user_data);

    static size_t mEncodeFrame = mEMSConfig.videoEncoderParam.videoFPS;
    static std::string rootDir = mEMSConfig.videoEncoderParam.saveMountPoint;
    static size_t mEncodeMinute = mEMSConfig.videoEncoderParam.encodeOneVideoMinute;

    static const size_t capFrameCount = mEncodeMinute * 60 * mEncodeFrame;                  // mEncodeMinute分 * 60秒 * mEncodeFrame帧
    static const size_t tfcardTotalMinCapLimitMB = mEMSConfig.videoEncoderParam.saveDevLimitCapGB << 10;
    static const size_t tfcardTotalMinCapLimitMBDiv4 = tfcardTotalMinCapLimitMB / 4;

    if (me->mVideoEncodeQuit) {
        return -1;
    }

    bool condFirst = (me->mInsertTFCardTotalCapMB > tfcardTotalMinCapLimitMB) && (me->mInsertTFCardFreeCapMB >= tfcardTotalMinCapLimitMB);
    bool condSecond = (me->mInsertTFCardTotalCapMB > tfcardTotalMinCapLimitMB) && ((me->mInsertTFCardFreeCapMB >= tfcardTotalMinCapLimitMBDiv4) && (me->mInsertTFCardFreeCapMB < tfcardTotalMinCapLimitMB));

    if (condFirst && me->mDetectTFCardWinthin) {
        bFindTFCardFirst = true;
        if (!bLockFileName) {
            std::string dir = getDateAndCreate(rootDir);
            if (!dir.empty()) {
                saveEncFile = dir + "/" + "VD_" + getDateTime() + std::string(me->mVideoVencType == DRM_CODEC_TYPE_H264 ? ".h264" : ".h265");
                bLockFileName = true;
            }
        }

        if (!saveEncFile.empty() && (saveEncFile != saveBKEncFile)) {
            saveBKEncFile = saveEncFile;
            me->mSaveVideoFileName.insert(saveEncFile);
        }

        if (!me->mVideoVencFrame.insert(mediaFrame)) {
            return -1;
        }

        saveFrameCount += 1;
        if (saveFrameCount >= (capFrameCount - 1)) {
            saveFrameCount = 0;
            me->mVideoEncodeStart = false;
            bLockFileName = false;
        }

        return 0;
    }

    if (bFindTFCardFirst) {
        me->mVideoEncodeCloseFile = true;
        bFindTFCardFirst = false;
    }

    if (condSecond && me->mDetectTFCardWinthin) {
        if (me->mVideoRemoveStart) {
            me->mVideoRemoveStart = false;
            me->mRemoveVideoRing.insert(true);
        }
    }

    saveFrameCount = 0;
    me->mVideoEncodeStart = false;
    bLockFileName = false;

    return -1;
}

void EMSDemoImpl::saveEncVideoToTFCardProcessThread()
{
    FILE *fp = NULL;
    size_t frameSize = 0;
    void *dataPtr = NULL;
    std::string saveEncFile = "";
    media_buffer_t mediaFrame = NULL;
    std::string saveFileNameBak = "";

    pthread_setname_np(pthread_self(), "saveVideoThread");

    while (!mThreadQuit) {
        if (!mVideoVencFrame.isEmpty() && mVideoVencFrame.remove(mediaFrame)) {
            dataPtr = getMedia()->getSys().getFrameData(mediaFrame);
            frameSize = getMedia()->getSys().getFrameSize(mediaFrame);

            if (!mSaveVideoFileName.isEmpty() && mSaveVideoFileName.remove(saveEncFile)) {

            } else {
                saveEncFile = "";
            }

            if (!saveEncFile.empty()) {
                if (saveEncFile != saveFileNameBak) {
                    if (fp) {
                        fclose(fp);
                        fp = NULL;
                    }

                    fp = fopen(saveEncFile.c_str(), "w");
                    if (!fp) {
                        goto FREE_FRAMELF;
                    }

                    saveFileNameBak = saveEncFile;
                    saveEncFile = "";
                }
            }

            // 小于1K的帧可能是编码器错误帧，不保存
            if (fp && dataPtr && (frameSize > 0)) {
                mVideoEncodeStart = true;
                fwrite(dataPtr, 1, frameSize, fp);
            }
FREE_FRAMELF:
            if (dataPtr && (frameSize > 0)) {
                getMedia()->getVenc().releaseFrame(mediaFrame);
            } else {
                static uint64_t count = 0;
                printf("======= leak frame count:[%llu], frameSize:[%u]\n", ++count, frameSize);
            }

            dataPtr = NULL;
            frameSize = 0;
        } else {
            if (mVideoEncodeCloseFile) {
                mVideoEncodeCloseFile = false;
                if (fp) {
                    fclose(fp);
                    fp = NULL;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void EMSDemoImpl::videoEncodeObjectProcessThread()
{
    int saveFrameCount = 0;
    bool bLockFileName = false;
    std::string saveEncFile = "";
    media_buffer_t mediaFrame = NULL;
    bool bFindTFCardFirst = false;
    std::string saveBKEncFile = "";
    size_t mEncodeFrame = mEMSConfig.videoEncoderParam.videoFPS;
    std::string rootDir = mEMSConfig.videoEncoderParam.saveMountPoint;
    size_t mEncodeMinute = mEMSConfig.videoEncoderParam.encodeOneVideoMinute;
    size_t tfcardTotalMinCapLimitMB = mEMSConfig.videoEncoderParam.saveDevLimitCapGB << 10;

    const size_t capFrameCount = mEncodeMinute * 60 * mEncodeFrame;     // mEncodeMinute分 * 60秒 * mEncodeFrame帧
    size_t tfcardTotalMinCapLimitMBDiv4 = tfcardTotalMinCapLimitMB / 4;

    pthread_setname_np(pthread_self(), "encObjThread");

    while (!mThreadQuit) {
        mediaFrame = getMedia()->getVenc().getFrame(mVideoVencChn, -1);
        if (!mVideoEncodeQuit) {
            bool condFirst = (mInsertTFCardTotalCapMB > tfcardTotalMinCapLimitMB) && (mInsertTFCardFreeCapMB >= tfcardTotalMinCapLimitMB);
            bool condSecond = (mInsertTFCardTotalCapMB > tfcardTotalMinCapLimitMB) && ((mInsertTFCardFreeCapMB >= tfcardTotalMinCapLimitMBDiv4) && (mInsertTFCardFreeCapMB < tfcardTotalMinCapLimitMB));

            if (condFirst && mDetectTFCardWinthin) {
                bFindTFCardFirst = true;
                if (!bLockFileName) {
                    std::string dir = getDateAndCreate(rootDir);
                    if (!dir.empty()) {
                        saveEncFile = dir + "/" + "VD_" + getDateTime() + std::string(mVideoVencType == DRM_CODEC_TYPE_H264 ? ".h264" : ".h265");
                        bLockFileName = true;
                    }
                }

                if (!saveEncFile.empty() && (saveEncFile != saveBKEncFile)) {
                    saveBKEncFile = saveEncFile;
                    mSaveVideoFileName.insert(saveEncFile);
                }

                mVideoVencFrame.insert(mediaFrame);

                saveFrameCount += 1;
                if (saveFrameCount >= (capFrameCount - 1)) {
                    saveFrameCount = 0;
                    mVideoEncodeStart = false;
                    bLockFileName = false;
                }
            } else {
                if (condSecond && mDetectTFCardWinthin) {
                    if (mVideoRemoveStart) {
                        mVideoRemoveStart = false;
                        mRemoveVideoRing.insert(true);
                    }
                }

                getMedia()->getRga().releaseFrame(mediaFrame);

                saveFrameCount = 0;
                mVideoEncodeStart = false;
                bLockFileName = false;
                if (bFindTFCardFirst) {
                    mVideoEncodeCloseFile = true;
                    bFindTFCardFirst = false;
                }
            }
        } else {
            getMedia()->getRga().releaseFrame(mediaFrame);

            saveFrameCount = 0;
            mVideoEncodeStart = false;
            bLockFileName = false;
            if (bFindTFCardFirst) {
                mVideoEncodeCloseFile = true;
                bFindTFCardFirst = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(200 * 1000));
    }
}

API_END_NAMESPACE(EMS)
