#ifndef DSL_DSL_SURFACE_TRANSFORM_H
#define DSL_DSL_SURFACE_TRANSFORM_H

#include "Dsl.h"
#include <nvbufsurftransform.h>
#include <gst-nvdssr.h>

namespace DSL {
/* 一个新的Cuda流的包装类 */
class DslCudaStream {
public:
    /**
     * @brief DslCudaStream类
     * @param gpuId GPU ID适用于多GPU系统
     */
    DslCudaStream(uint32_t gpuId) : stream(NULL)
    {
        LOG_FUNC();

        cudaError_t cudaError = cudaSetDevice(gpuId);
        if (cudaError != cudaSuccess) {
            LOG_ERROR("cudaSetDevice failed with error '" << cudaError << "'");
            throw;
        }

        cudaError = cudaStreamCreate(&stream);
        if (cudaError != cudaSuccess) {
            LOG_ERROR("cudaStreamCreate failed with error '" << cudaError << "'");
            throw;
        }
    }

    ~DslCudaStream()
    {
        LOG_FUNC();

        if (stream) {
            LOG_DEBUG("cudaStreamDestroy");
            cudaStreamDestroy(stream);
        }
    }

public:
    cudaStream_t stream;    // 如果成功创建cuda流，否则为NULL
};

/* 访问批处理surface缓冲区的信息映射 */
struct DslMappedBuffer : public GstMapInfo {
public:
    DslMappedBuffer(GstBuffer *pBuffer)
        : GstMapInfo GST_MAP_INFO_INIT, m_pBuffer(NULL)
    {
        LOG_FUNC();

        if (!gst_buffer_map(pBuffer, this, GST_MAP_READ)) {
            LOG_ERROR("Failed to map gst buffer");
            throw;
        }

        m_pBuffer = pBuffer;
        pSurface = (NvBufSurface *)data;
    }

    ~DslMappedBuffer()
    {
        LOG_FUNC();

        if (m_pBuffer) {
            LOG_DEBUG("gst_buffer_unmap");
            gst_buffer_unmap(m_pBuffer, this);
        }
    }

    /**
     * @brief 获取列表中特定surface的宽度
     */
    uint32_t GetWidth(uint32_t index)
    {
        return pSurface->surfaceList[index].width;
    }

    /**
     * @brief 获取列表中特定表面的高度
     */
    uint32_t GetHeight(uint32_t index)
    {
        return pSurface->surfaceList[index].height;
    }

    NvBufSurface *pSurface;     // 映射缓冲区表面数据的访问器

private:
    GstBuffer    *m_pBuffer;    // 映射缓冲区
};

/* 从批处理surface缓冲中的单个surface复制的新的单surface缓冲 */
struct DslMonoSurface : public NvBufSurface {
public:
    /**
     * @brief DslMonoSurface结构
     * @param mapInfo 映射缓冲区信息作为源表面缓冲区
     * @param index   索引到批处理表面列表
     */
    DslMonoSurface(NvBufSurface *pBatchedSurface, int index)
        : NvBufSurface{0}, width(0), height(0)
    {
        LOG_FUNC();

        // TODO: 更新以支持gpuId > 0

        // 复制共享surface属性
        (NvBufSurface)*this = *pBatchedSurface;

        // 将缓冲surface设置为单面
        numFilled = 1;
        batchSize = 1;

        // 将单个索引surface复制到一个的新surface列表中
        surfaceList = &(pBatchedSurface->surfaceList[index]);

        // 新的surface宽度和高度属性，因为只有一个
        width = surfaceList[0].width;
        height = surfaceList[0].height;
    }

    ~DslMonoSurface()
    {
        LOG_FUNC();
    }

public:
    int width;      // 宽度surface缓冲区的宽度(以像素为单位)
    int height;     // 高度surface缓冲区的高度(以像素为单位)
};

/* 曲面转换参数与坐标和尺寸的源和目标曲面。(目前)不支持源到目标的缩放 */
struct DslTransformParams : public NvBufSurfTransformParams {
public:
    DslTransformParams(uint32_t left, uint32_t top, uint32_t width, uint32_t height)
        : NvBufSurfTransformParams{0}, m_srcRect{top, left, width, height} , m_dstRect{0, 0, width, height} 
    {
        LOG_FUNC();

        // SRC和DST矩形是通过指针设置的
        src_rect = &m_srcRect;
        dst_rect = &m_dstRect;
        transform_flag = NVBUFSURF_TRANSFORM_CROP_SRC | NVBUFSURF_TRANSFORM_CROP_DST;
        transform_filter = NvBufSurfTransformInter_Default;
    }

    ~DslTransformParams()
    {
        LOG_FUNC();
    }

private:
    NvBufSurfTransformRect m_srcRect;   // m_srcRect要在源表面内转换的矩形的坐标和尺寸
    NvBufSurfTransformRect m_dstRect;   // m_srcRect为目标曲面变换的矩形的坐标和尺寸
};

/* surface创建参数与尺寸和内存分配大小 */
struct DslSurfaceCreateParams : public NvBufSurfaceCreateParams {
public:
    /**
     * @brief DslSurfaceCreateParams结构
     * @param gpuId  GPU ID适用于多GPU系统
     * @param width  新surface的宽度，如果只有1
     * @param height 新surface的高度，如果只有1
     * @param size   如果设置了要创建的内存，则忽略宽度和高度。当批处理大小=1时，如果不分配额外内存，则将size设置为0
     */
    DslSurfaceCreateParams(uint32_t gpuId, uint32_t width, uint32_t height, uint32_t size, NvBufSurfaceColorFormat colorFormat, NvBufSurfaceMemType memType)
        : NvBufSurfaceCreateParams{gpuId, width, height, size, false, colorFormat, NVBUF_LAYOUT_PITCH, memType}
    {
        LOG_FUNC();
    }

    ~DslSurfaceCreateParams()
    {
        LOG_FUNC();
    }
};

/* "转换会话"配置参数的结构，具有设置会话参数的功能 */
struct DslSurfaceTransformSessionParams : public NvBufSurfTransformConfigParams {
public:
    /**
     * @brief DslCudaStream类的
     * @param gpuId  GPU ID适用于多GPU系统
     */
    DslSurfaceTransformSessionParams(int32_t gpuId, DslCudaStream &cudaStream)
        : NvBufSurfTransformConfigParams{NvBufSurfTransformCompute_Default, gpuId, cudaStream.stream}
    {
        LOG_FUNC();
    }

    ~DslSurfaceTransformSessionParams()
    {
        LOG_FUNC();
    }

    /**
     * @brief 函数设置转换会话参数
     */
    bool Set()
    {
        LOG_FUNC();

        NvBufSurfTransform_Error error = NvBufSurfTransformSetSessionParams(this);
        if (error != NvBufSurfTransformError_Success) {
            LOG_ERROR("NvBufSurfTransformSetSessionParams failed with error '" << error << "'");
            return false;
        }

        return true;
    }
};

/* 用于新批处理表面缓冲区的包装器类 */
class DslBufferSurface {
public:
    /**
     * @brief 参数为DslBufferSurface类
     * @param[in] batchSize           用于新surface的批量大小
     * @param[in] surfaceCreateParams 创建用于新surface的参数
     */
    DslBufferSurface(uint32_t batchSize, DslSurfaceCreateParams &surfaceCreateParams, uint64_t uniqueId)
        : m_pBufSurface(NULL), m_uniqueId(uniqueId), m_isMapped(false)
    {
        LOG_FUNC();

        if (NvBufSurfaceCreate(&m_pBufSurface, batchSize, &surfaceCreateParams) != NvBufSurfTransformError_Success) {
            LOG_ERROR("NvBufSurfaceCreate failed");
            throw;
        }

        if (NvBufSurfaceMemSet(m_pBufSurface, -1, -1, 0) != NvBufSurfTransformError_Success) {
            LOG_ERROR("NvBufSurfaceMemSet failed");
            throw;
        }

        char dateTime[64] = {0};
        time_t seconds = time(NULL);

        struct tm currentTm;
        localtime_r(&seconds, &currentTm);

        std::strftime(dateTime, sizeof(dateTime), "%Y%m%d-%H%M%S", &currentTm);
        m_dateTimeStr = dateTime;
    }

    ~DslBufferSurface()
    {
        LOG_FUNC();

        if (m_isMapped) {
            LOG_DEBUG("NvBufSurfaceUnMap");
            NvBufSurfaceUnMap(m_pBufSurface, -1, -1);
        }

        if (m_pBufSurface) {
            LOG_DEBUG("NvBufSurfaceDestroy");
            NvBufSurfaceDestroy(m_pBufSurface);
        }
    }

    /**
     * @brief  取地址运算符
     * @return 返回一个指向实际GST批处理surface缓冲区的指针
     */
    NvBufSurface *operator&() {
        return m_pBufSurface;
    }

    /**
     * @brief  函数将单源surface转换为批处理中索引的surface
     * @return 变换成功为true，反之为false
     */
    bool TransformMonoSurface(DslMonoSurface &srcSurface, uint32_t index, DslTransformParams &transformParams)
    {
        return (NvBufSurfTransform(&srcSurface, &m_pBufSurface[index], &transformParams) == NvBufSurfTransformError_Success);
    }

    /**
     * @brief  函数映射新转换的批处理surface缓冲区
     * @return 映射成功时为true，否则为false
     */
    bool Map()
    {
        if (NvBufSurfaceMap(m_pBufSurface, -1, -1, NVBUF_MAP_READ) != NvBufSurfTransformError_Success) {
            return false;
        }

        m_isMapped = true;
        return true;
    }

    /**
     * @brief  函数来同步一个映射的，转换的批处理surface缓冲区，由硬件修改，供CPU访问
     * @return 映射成功时为true，否则为false
     */
    bool SyncForCpu()
    {
        return (NvBufSurfaceSyncForCpu(m_pBufSurface, -1, -1) == NvBufSurfTransformError_Success);
    }

    bool Copy(DslBufferSurface& srcSurface)
    {
        LOG_FUNC();

        return (NvBufSurfaceCopy(&srcSurface, m_pBufSurface) == NvBufSurfTransformError_Success);
    }

    uint64_t GetUniqueId()
    {
        LOG_FUNC();
        return m_uniqueId;
    }

    const char *GetDateTimeStr()
    {
        LOG_FUNC();
        return m_dateTimeStr.c_str();
    }

private:
    NvBufSurface *m_pBufSurface;    // 指向NVIDIA NvBufferSurface结构体的指针
    uint64_t     m_uniqueId;        // BufferSurface的唯一id
    bool         m_isMapped;        // 一旦映射，设置为true，以便缓冲区可以在销毁之前取消映射
    std::string  m_dateTimeStr;     // 创建DslBufferSurface的日期-时间字符串
};
}

#endif
