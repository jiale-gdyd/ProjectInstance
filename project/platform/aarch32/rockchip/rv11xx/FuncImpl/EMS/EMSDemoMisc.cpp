#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nlohmann/json.hpp>

#include "EMSDemo.hpp"

API_BEGIN_NAMESPACE(EMS)

static size_t getFileSizeBytes(std::string filename)
{
    struct stat statbuf;
    if (stat(filename.c_str(), &statbuf) == 0) {
        return statbuf.st_size;
    }

    return 0;
}

void EMSDemoImpl::parseVideoDecodeParam(std::string confJsonFile)
{
    if (access(confJsonFile.c_str(), F_OK)) {
        printf("%s not exist\n", confJsonFile.c_str());
        return;
    }

    if (!nlohmann::json::accept(std::ifstream(confJsonFile))) {
        printf("%s not a json file\n", confJsonFile.c_str());
        return;
    }

    nlohmann::Json j;
    std::ifstream fin(confJsonFile);
    if (!fin.good()) {
        printf("%s open failed\n", confJsonFile.c_str());
        return;
    }

    fin >> j;
    bool bEnableVdec = false;
    std::vector<media_vdec_param_t> params;

    if (j.contains("enableVdec") && j.at("enableVdec").is_boolean()) {
        j.at("enableVdec").get_to(bEnableVdec);
    }

    if (bEnableVdec) {
        if (j.contains("parameter") && j.at("parameter").is_array()) {
            for (auto iter : j.at("parameter").items()) {
                int count = 0;
                media_vdec_param_t param;
                nlohmann::Json array = iter.value();

                if (array.contains("vdecChn") && array["vdecChn"].is_number_unsigned()) {
                    int vdecChn = -1;
                    array.at("vdecChn").get_to(vdecChn);

                    if ((vdecChn >= 0) && (vdecChn < DRM_VDEC_CHANNEL_BUTT)) {
                        count += 1;
                        param.vdecChn = vdecChn;
                    }
                }

                if (array.contains("bDecLoop") && array["bDecLoop"].is_boolean()) {
                    count += 1;
                    array.at("bDecLoop").get_to(param.bDecLoop);
                }

                if (array.contains("intervalMs") && array["intervalMs"].is_number_unsigned()) {
                    count += 1;
                    array.at("intervalMs").get_to(param.intervalMs);
                }

                if (array.contains("oneFramSize") && array["oneFramSize"].is_number_unsigned()) {
                    count += 1;
                    array.at("oneFramSize").get_to(param.oneFramSize);
                }

                if (array.contains("useHardwareMem") && array["useHardwareMem"].is_boolean()) {
                    array.at("useHardwareMem").get_to(param.hardwareAlloc);
                } else {
                    param.hardwareAlloc = false;
                }

                if (array.contains("memAllocFlag") && array["memAllocFlag"].is_number_unsigned()) {
                    array.at("memAllocFlag").get_to(param.allocFlag);
                } else {
                    param.allocFlag = 0;
                }

                if (array.contains("videoWidth") && array["videoWidth"].is_number_unsigned()) {
                    size_t videoWidth = 0;
                    array.at("videoWidth").get_to(videoWidth);

                    if (videoWidth > 0) {
                        count += 1;
                        param.videoWidth = videoWidth;
                    }
                }

                if (array.contains("videoHeight") && array["videoHeight"].is_number_unsigned()) {
                    size_t videoHeight = 0;
                    array.at("videoHeight").get_to(videoHeight);

                    if (videoHeight > 0) {
                        count += 1;
                        param.videoHeight = videoHeight;
                    }
                }

                if (array.contains("codecType") && array["codecType"].is_string()) {
                    std::string codecType;
                    array.at("codecType").get_to(codecType);

                    if (!codecType.empty()) {
                        if ((codecType == "h264") || (codecType == "H264")) {
                            param.codecType = DRM_CODEC_TYPE_H264;
                            count += 1;
                        } else if ((codecType == "h265") || (codecType == "H265")) {
                            param.codecType = DRM_CODEC_TYPE_H265;
                            count += 1;
                        } else if ((codecType == "jpeg") || (codecType == "JPEG")) {
                            param.codecType = DRM_CODEC_TYPE_JPEG;
                            count += 1;
                        } else if ((codecType == "mjpeg") || (codecType == "MJPEG")) {
                            param.codecType = DRM_CODEC_TYPE_MJPEG;
                            count += 1;
                        }
                    }
                }

                if (array.contains("codecFile") && array["codecFile"].is_array()) {
                    std::vector<std::string> codecFileVector;
                    nlohmann::Json codecFileArray = array["codecFile"];

                    if (codecFileArray.size() > 0) {
                        for (auto it = codecFileArray.begin(); it != codecFileArray.end(); ++it) {
                            std::string codecFile;
                            codecFile = it.value().get<std::string>();

                            if (!codecFile.empty()) {
                                if (access(codecFile.c_str(), F_OK) == 0) {
                                    size_t fileSize = getFileSizeBytes(codecFile);
                                    if ((fileSize / 1024.0) > 1.0) {
                                        codecFileVector.push_back(codecFile);
                                    }
                                }
                            }
                        }
                    }

                    if (codecFileVector.size() > 0) {
                        count += 1;
                        param.codecFile = codecFileVector;
                    }
                }

                if (count >= 8) {
                    params.emplace_back(param);
                }
            }
        }
    }

    if ((params.size() > 0) && bEnableVdec) {
        mUseVdecNotVi = true;
        mVdecParameter = params;
    } else {
        mUseVdecNotVi = false;
    }

    printf("mUseVdecNotVi:[%s], params.size:[%u]\n", mUseVdecNotVi ? "true" : "false", params.size());
}

void EMSDemoImpl::parseApplicationConfigParam(std::string confJsonFile)
{
    if (access(confJsonFile.c_str(), F_OK)) {
        printf("%s not exist, i will generate it and use default config\n", confJsonFile.c_str());
        generateApplicationConfigParam(mEMSConfig);
        return;
    }

    if (!nlohmann::json::accept(std::ifstream(confJsonFile))) {
        printf("%s not a json file, i will generate it and use default config\n", confJsonFile.c_str());
        generateApplicationConfigParam(mEMSConfig);
        return;
    }

    nlohmann::Json j;
    std::ifstream fin(confJsonFile);
    if (!fin.good()) {
        printf("%s open failed\n", confJsonFile.c_str());
        return;
    }

    fin >> j;
    if (j.contains("enableThisChannel") && j.at("enableThisChannel").is_boolean()) {
        j.at("enableThisChannel").get_to(mEMSConfig.enableThisChannel);
    }

    if (j.contains("enableRGAFlushCache") && j.at("enableRGAFlushCache").is_boolean()) {
        j.at("enableRGAFlushCache").get_to(mEMSConfig.enableRGAFlushCache);
    }

    if (j.contains("disableVideoEncoderSave") && j.at("disableVideoEncoderSave").is_boolean()) {
        bool disableVideoEncoderSave = true;
        j.at("disableVideoEncoderSave").get_to(disableVideoEncoderSave);

        if (!disableVideoEncoderSave) {
            if (j.contains("videoEncoderParam") && j.at("videoEncoderParam").is_object()) {
                nlohmann::Json JVenc = j.at("videoEncoderParam");

                if (JVenc.contains("videpFPS") && JVenc.at("videpFPS").is_number_unsigned()) {
                    size_t videpFPS;
                    JVenc.at("videpFPS").get_to(videpFPS);

                    videpFPS = videpFPS >= 25 && videpFPS <= 60 ? videpFPS : 30;
                    mEMSConfig.videoEncoderParam.videoFPS = videpFPS;
                }

                if (JVenc.contains("encodeProfile") && JVenc.at("encodeProfile").is_number_unsigned()) {
                    size_t encodeProfile;
                    JVenc.at("encodeProfile").get_to(encodeProfile);

                    encodeProfile = encodeProfile > 0 ? encodeProfile : 66;
                    mEMSConfig.videoEncoderParam.encodeProfile = encodeProfile;
                }

                if (JVenc.contains("encodeBitRate") && JVenc.at("encodeBitRate").is_number_unsigned()) {
                    size_t encodeBitRate;
                    JVenc.at("encodeBitRate").get_to(encodeBitRate);

                    encodeBitRate = encodeBitRate > 0 ? encodeBitRate : 8000000;
                    mEMSConfig.videoEncoderParam.encodeBitRate = encodeBitRate;
                }

                if (JVenc.contains("encodeOneVideoMinute") && JVenc.at("encodeOneVideoMinute").is_number_unsigned()) {
                    size_t encodeOneVideoMinute;
                    JVenc.at("encodeOneVideoMinute").get_to(encodeOneVideoMinute);

                    encodeOneVideoMinute = encodeOneVideoMinute > 0 && encodeOneVideoMinute <= 30 ? encodeOneVideoMinute : 2;
                    mEMSConfig.videoEncoderParam.encodeOneVideoMinute = encodeOneVideoMinute;
                }

                if (JVenc.contains("encodeIFrameInterval") && JVenc.at("encodeIFrameInterval").is_number_unsigned()) {
                    size_t encodeIFrameInterval;
                    JVenc.at("encodeIFrameInterval").get_to(encodeIFrameInterval);

                    encodeIFrameInterval = encodeIFrameInterval > 0 ? encodeIFrameInterval : 15;
                    mEMSConfig.videoEncoderParam.encodeIFrameInterval = encodeIFrameInterval;
                }

                if (JVenc.contains("saveDevLimitCapGB") && JVenc.at("saveDevLimitCapGB").is_number_unsigned()) {
                    size_t saveDevLimitCapGB;
                    JVenc.at("saveDevLimitCapGB").get_to(saveDevLimitCapGB);

                    saveDevLimitCapGB = saveDevLimitCapGB <= 0 ? 2 : saveDevLimitCapGB;
                    mEMSConfig.videoEncoderParam.saveDevLimitCapGB = saveDevLimitCapGB;
                }

                if (JVenc.contains("saveMountPoint") && JVenc.at("saveMountPoint").is_string()) {
                    std::string saveMountPoint;
                    JVenc.at("saveMountPoint").get_to(saveMountPoint);

                    if (saveMountPoint.back() != '/') {
                        saveMountPoint += "/";
                    }
                    mEMSConfig.videoEncoderParam.saveMountPoint = saveMountPoint;
                }

                if (JVenc.contains("saveDeviceNode") && JVenc.at("saveDeviceNode").is_string()) {
                    std::string saveDeviceNode;
                    JVenc.at("saveDeviceNode").get_to(saveDeviceNode);

                    mEMSConfig.videoEncoderParam.saveDeviceNode = saveDeviceNode;
                }
            }
        }

        mEMSConfig.disableVideoEncoderSave = disableVideoEncoderSave;
    }
}

void EMSDemoImpl::generateApplicationConfigParam(ems_config_t configParam)
{
    std::string confJsonFile = APPLF_CONFIG_JSONFILE;

    nlohmann::Json j;

    /*********************************************************************************************************************/
    j["enableThisChannel"] = configParam.enableThisChannel;
    j["enableRGAFlushCache"] = configParam.enableRGAFlushCache;
    /*********************************************************************************************************************/

    /*********************************************************************************************************************/
    nlohmann::Json jVenc;

    j["disableVideoEncoderSave"] = configParam.disableVideoEncoderSave;
    jVenc["videpFPS"] = configParam.videoEncoderParam.videoFPS;
    jVenc["encodeProfile"] = configParam.videoEncoderParam.encodeProfile;
    jVenc["encodeBitRate"] = configParam.videoEncoderParam.encodeBitRate;
    jVenc["encodeOneVideoMinute"] = configParam.videoEncoderParam.encodeOneVideoMinute;
    jVenc["encodeIFrameInterval"] = configParam.videoEncoderParam.encodeIFrameInterval;
    jVenc["saveDevLimitCapGB"] = configParam.videoEncoderParam.saveDevLimitCapGB;
    jVenc["saveMountPoint"] = configParam.videoEncoderParam.saveMountPoint;
    jVenc["saveDeviceNode"] = configParam.videoEncoderParam.saveDeviceNode;
    j["videoEncoderParam"] = jVenc;
    /*********************************************************************************************************************/

    std::ofstream fout(confJsonFile);
    if (fout.good()) {
        fout << std::setw(4) << j << std::endl;
        system("sync");
    }
}

API_END_NAMESPACE(EMS)
