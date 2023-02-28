#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

void EMSDemoImpl::videoEncodeInit()
{
    mVideoEncodeQuit = false;
    mVideoEncodeStart = false;
    mVideoRemoveStart = false;
    mVideoEncodeCloseFile = false;

    mInsertTFCardFreeCapMB = 0;
    mInsertTFCardFreeCapGB = 0;
    mDetectTFCardWinthin = false;
    mInsertTFCardTotalCapMB = 0;
    mInsertTFCardTotalCapGB = 0;

    mDetectTFCardWinthin = access(mEMSConfig.videoEncoderParam.saveDeviceNode.c_str(), F_OK) == 0;
    mVideoDetectTFCardThreadId = std::thread(std::bind(&EMSDemoImpl::detectTFCardProcessThread, this));

    if (mVideoVencEnOK) {
        mSaveVideo2TFCardThreadId = std::thread(std::bind(&EMSDemoImpl::saveEncVideoToTFCardProcessThread, this));
        mVideoRemoveThreadId = std::thread(std::bind(&EMSDemoImpl::videoRemoveProcessThread, this));
        mVideoRemovePreprocThreadId = std::thread(std::bind(&EMSDemoImpl::videoRemovePreProcessThread, this));
    }
}

void EMSDemoImpl::videoEncodeExit()
{
    mVideoEncodeQuit = false;
    mVideoEncodeStart = false;
    mVideoRemoveStart = false;
    mInsertTFCardFreeCapMB = 0;
    mInsertTFCardFreeCapGB = 0;
    mDetectTFCardWinthin = false;
    mInsertTFCardTotalCapMB = 0;
    mInsertTFCardTotalCapGB = 0;

    if (mVideoRemoveThreadId.joinable()) {
        mVideoRemoveThreadId.join();
    }

    if (mVideoRemovePreprocThreadId.joinable()) {
        mVideoRemovePreprocThreadId.join();
    }

    if (mVideoDetectTFCardThreadId.joinable()) {
        mVideoDetectTFCardThreadId.join();
    }

    if (mSaveVideo2TFCardThreadId.joinable()) {
        mSaveVideo2TFCardThreadId.join();
    }
}

static size_t getDiskTotalAndFreeCapMb(std::string mountPoint, size_t &totalCapMb, bool &bExit)
{
    int ret = -1;
    struct statfs diskInfo;
    size_t mbFreeDiskSize = 0;
    unsigned long long freeDisk = 0;
    unsigned long long totalSize = 0;
    unsigned long long totalBlocks = 0;

    if (mountPoint.empty()) {
        bExit = false;
        return 0;
    }

    ret = statfs(mountPoint.c_str(), &diskInfo);
    if (ret < 0) {
        bExit = false;
        return 0;
    }

    totalBlocks = diskInfo.f_bsize;
    totalSize = totalBlocks * diskInfo.f_blocks;
    totalCapMb = totalSize >> 20;
    freeDisk = diskInfo.f_bfree * totalBlocks;
    mbFreeDiskSize = freeDisk >> 20;

    if ((totalSize >> 20) > 1024) {
        bExit = true;
    }

    return mbFreeDiskSize;
}

void EMSDemoImpl::detectTFCardProcessThread()
{
    std::string mp = mEMSConfig.videoEncoderParam.saveMountPoint;
    std::string dn = mEMSConfig.videoEncoderParam.saveDeviceNode;

    pthread_setname_np(pthread_self(), "detectTFCardThr");

    while (!mThreadQuit) {
        bool bDetectCard = false;
        bool bTFCardInsert = false;
        size_t tfcardFreeCapMB = 0;
        size_t tfcardTotalCapMB = 0;

        bTFCardInsert = access(dn.c_str(), F_OK) == 0;
        tfcardFreeCapMB = getDiskTotalAndFreeCapMb(dn, tfcardTotalCapMB, bDetectCard);

        mDetectTFCardWinthin = bTFCardInsert && bDetectCard;
        mInsertTFCardFreeCapMB = tfcardFreeCapMB;
        mInsertTFCardTotalCapMB = tfcardTotalCapMB;
        mInsertTFCardFreeCapGB = tfcardFreeCapMB >> 10;
        mInsertTFCardTotalCapGB = tfcardTotalCapMB >> 10;

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

API_END_NAMESPACE(EMS)
