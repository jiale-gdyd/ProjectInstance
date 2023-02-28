#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

static int rm_dir(std::string dir_full_path)
{
    DIR *dirp = opendir(dir_full_path.c_str());
    if (!dirp) {
        return -4;
    }

    struct stat st;
    struct dirent *dir;

    while ((dir = readdir(dirp)) != NULL) {
        if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0)) {
            continue;
        }

        std::string sub_path = dir_full_path + '/' + dir->d_name;
        if (lstat(sub_path.c_str(), &st) < 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // 如果收目录文件。递归删除
            if (rm_dir(sub_path) < 0) {
                closedir(dirp);
                return -5;
            }

            rmdir(sub_path.c_str());
        } else if (S_ISREG(st.st_mode)) {
            // 如果是文件，直接删除
            unlink(sub_path.c_str());
        } else {
            continue;
        }
    }

    if (rmdir(dir_full_path.c_str()) < 0) {
        closedir(dirp);
        return -6;
    }

    closedir(dirp);
    return 0;
}

static int rm_dir_file(std::string filename)
{
    struct stat st;
    std::string file_path = filename;

    if (lstat(file_path.c_str(), &st) < 0) {
        return -1;
    }

    if (S_ISREG(st.st_mode)) {
        if (unlink(file_path.c_str()) < 0) {
            return -2;
        }
    } else if (S_ISDIR(st.st_mode)) {
        if ((filename == ".") || (filename == "..")) {
            return -3;
        }

        int ret = rm_dir(file_path);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

static bool strIsNum(const char *str)
{
    if (!str) {
        return false;
    }

    while (*str != '\0') {
        if ((*str < '0') || (*str > '9')) {
            return false;
        }

        str++;
    }

    return true;
}

static int getDvrRecordDir(std::string rootDir, std::vector<std::string> &dirName)
{
    DIR *dir = NULL;
    struct dirent *ptr = NULL;

    if ((dir = opendir(rootDir.c_str())) == NULL) {
        return -1;
    }

    while ((ptr = readdir(dir)) != NULL) {
        if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) {
            continue;
        } else if (ptr->d_type == DT_DIR) {
            std::string ldirName = std::string(ptr->d_name);

            if ((strlen(ldirName.c_str()) == strlen("YYYYmmdd")) && strIsNum(ldirName.c_str())) {
                dirName.push_back(ldirName);
            } else {
                continue;
            }
        }
    }
    closedir(dir);

    return 0;
}

int EMSDemoImpl::getDvrRecordList(std::string rootDir)
{
   std::lock_guard<std::mutex> lock(mGetDvrVideoMutex);

    if (rootDir.empty()) {
        printf("input tfcard mount point invalid\n");
        return -1;
    }

    std::vector<std::string> dateDirName;
    int ret = getDvrRecordDir(rootDir, dateDirName);
    if ((ret < 0) || dateDirName.empty()) {
        return -1;
    }

    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    video_map_vector_t dirFileList;

    if (*rootDir.rbegin() != '/') {
        rootDir = rootDir + "/";
    }

    for (size_t i = 0; i < dateDirName.size(); ++i) {
        std::string dirName = rootDir + dateDirName.at(i);
        if ((dir = opendir(dirName.c_str())) == NULL) {
            continue;
        }

        std::vector<std::string> h26xFile;
        while ((ptr = readdir(dir)) != NULL) {
            if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, "..")) {
                continue;
            } else if (ptr->d_type == DT_REG) {
                char *p = rindex(ptr->d_name, '.');
                if ((p != NULL) && !strcmp(p, mVideoVencType == DRM_CODEC_TYPE_H264 ? ".h264" : ".h265")) {
                    h26xFile.emplace_back(ptr->d_name);
                }
            }
        }
        closedir(dir);

        if (h26xFile.size() > 0) {
            std::sort(h26xFile.begin(), h26xFile.end(), [](const std::string a, const std::string b) -> bool {
                return (a > b);
            });
            dirFileList.insert(std::make_pair(dateDirName.at(i), h26xFile));
        }
    }

    if (dirFileList.size() > 0) {
        mVideoMapVecList.clear();
        mVideoMapVecList.insert(dirFileList.begin(), dirFileList.end());
        mVideoMapVecList.insert(mVideoMapVecList.begin(), std::make_pair("back", std::vector<std::string>()));

        return (int)mVideoMapVecList.size();
    }

    return 0;
}

void EMSDemoImpl::videoRemoveProcessThread()
{
    char cmd[512] = {0};
    std::string name = "";

    pthread_setname_np(pthread_self(), "videoRmThread");

    while (!mThreadQuit) {
        if (!mRemoveVideoName.isEmpty() && mRemoveVideoName.remove(name)) {
            if (!name.empty()) {
                int ret = rm_dir_file(name);
                printf("========== delete video:[%s] return:[%2d] ==========\n", name.c_str(), ret);
            }

            mVideoRemoveStart = true;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void EMSDemoImpl::videoRemovePreProcessThread()
{
    bool bDelVid = false;
    std::string mVdecVidDirPrefix = mEMSConfig.videoEncoderParam.saveMountPoint;

    pthread_setname_np(pthread_self(), "delVideoThread");

    while (!mThreadQuit) {
        if (!mRemoveVideoRing.isEmpty() && mRemoveVideoRing.remove(bDelVid)) {
            if (bDelVid) {
                getDvrRecordList(mVdecVidDirPrefix);

                if (!mVideoMapVecList.empty()) {
                    printf("========== [LF] VideoMapVecList:[%u], first:[%s] ==========\n", mVideoMapVecList.size(), mVideoMapVecList.begin()->first.c_str());

                    // 只有一个视频目录
                    if ((mVideoMapVecList.size() == 2) && (mVideoMapVecList.begin()->first == "back")) {
                        std::string dir = mVideoMapVecList.rbegin()->first;
                        std::vector<std::string> vid = mVideoMapVecList.rbegin()->second;

                        if (vid.size() >= 5) {
                            int count = 0;
                            for (int i = 0; i < 5; ++i) {
                                std::string name = mVdecVidDirPrefix + dir + std::string("/") + std::string(vid.at(vid.size() - 1));
                                if (!name.empty()) {
                                    mRemoveVideoName.insert(name);
                                    vid.pop_back();
                                    count += 1;
                                }
                            }

                            if (count == 0) {
                                mVideoRemoveStart = true;
                            }
                        } else {
                            if (vid.size() > 0) {
                                std::string name = mVdecVidDirPrefix + dir + std::string("/") + std::string(vid.at(vid.size() - 1));
                                if (!name.empty()) {
                                    mRemoveVideoName.insert(name);
                                    vid.pop_back();
                                } else {
                                    mVideoRemoveStart = true;
                                }
                            } else {
                                mVideoRemoveStart = true;
                            }
                        }
                    } else {
                        std::string dir = mVideoMapVecList.rbegin()->first;
                        if (!dir.empty()) {
                            std::string name = mVdecVidDirPrefix + dir;
                            mRemoveVideoName.insert(name);
                        } else {
                            mVideoRemoveStart = true;
                        }
                    }

                    mVideoMapVecList.clear();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

API_END_NAMESPACE(EMS)
