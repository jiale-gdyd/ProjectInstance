#ifndef DSL_DSL_SOURCE_METER_H
#define DSL_DSL_SOURCE_METER_H

#include "Dsl.h"

namespace DSL {
#define DSL_SOURCE_METER_PTR        std::shared_ptr<SourceMeter>
#define DSL_SOURCE_METER_NEW(name)  std::shared_ptr<SourceMeter>(new SourceMeter(name))

/* 在两个独立的epics，一个会话，另一个间隔中实现一个测量FPS的仪表 */
class SourceMeter {
public:
    /**
     * @brief 源仪表
     * @param sourceId 被计量的源的唯一Id
     */
    SourceMeter(uint sourceId)
        : m_sourceId(sourceId), m_timeStamp{0}, m_sessionStartTime{0}, m_intervalStartTime{0}, m_sessionFrameCount(0), m_intervalFrameCount(0)
    {

    }

    /**
     * @brief 更新源表的时间戳
     */
    void Timestamp()
    {
        gettimeofday(&m_timeStamp, NULL);

        // 创建后的开始时间的一次性初始化
        if (!m_sessionStartTime.tv_sec) {
            m_sessionStartTime = m_timeStamp;
            m_intervalStartTime = m_timeStamp;
        }
    }

    /**
     * @brief 增加两个帧计数器。必须在每个缓冲区上调用
     */
    void IncrementFrameCounts()
    {
        m_sessionFrameCount++;
        m_intervalFrameCount++;
    }

    /**
     * @brief 只重置会话参数
     */
    void SessionReset()
    {
        m_sessionStartTime = m_timeStamp;
        m_sessionFrameCount = 0;
    }

    /**
     * @brief 只重置Interval参数
     */
    void IntervalReset()
    {
        m_intervalStartTime = m_timeStamp;
        m_intervalFrameCount = 0;
    }

    /**
     * @brief  计算整个会话的平均每秒帧数
     * @return 平均会话FPS
     */
    double GetSessionFpsAvg()
    {
        if (!m_sessionFrameCount) {
            return 0;
        }

        // 将秒和微秒转换为毫秒，并将其添加到单个数字中
        uint64_t sessionTime = (uint64_t)(m_timeStamp.tv_sec*1000 + m_timeStamp.tv_usec/1000) - (uint64_t)(m_sessionStartTime.tv_sec*1000 + m_sessionStartTime.tv_usec/1000);

        double sessionFpsAvg = (double)m_sessionFrameCount / ((double)sessionTime/1000);

        LOG_INFO("Source '" << m_sourceId << "' session FPS avg = " << sessionFpsAvg);
        return sessionFpsAvg;
    }

    /**
     * @brief  计算在单个间隔内每秒的平均帧数
     * @return 平均间隔FPS
     */
    double GetIntervalFpsAvg()
    {
        if (!m_intervalFrameCount) {
            return 0;
        }

        uint64_t intervalTime = (uint64_t)(m_timeStamp.tv_sec*1000 + m_timeStamp.tv_usec/1000) - (uint64_t)(m_intervalStartTime.tv_sec*1000 + m_intervalStartTime.tv_usec/1000);

        double intervalFpsAvg = (double)m_intervalFrameCount / ((double)intervalTime/1000);

        LOG_INFO("Source '" << m_sourceId << "' interval FPS avg = " << intervalFpsAvg);
        return intervalFpsAvg;
    }

private:
    int            m_sourceId;              // 被测量的源的唯一源Id
    struct timeval m_timeStamp;             // 在每个缓冲区上使用唯一源的帧元更新时间戳
    struct timeval m_sessionStartTime;      // 当前会话开始的时间戳
    struct timeval m_intervalStartTime;     // 当前间隔开始的时间戳
    uint           m_intervalFrameCount;    // 自当前会话开始以来的帧数
    uint           m_sessionFrameCount;     // 从当前间隔开始的帧数
};
}

#endif
