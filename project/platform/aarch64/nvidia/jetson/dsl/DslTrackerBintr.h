#ifndef DSL_DSL_TRACKER_BINTR_H
#define DSL_DSL_TRACKER_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBintr.h"
#include "DslElementr.h"
#include "DslPadProbeHandler.h"

namespace DSL {
#define DSL_TRACKER_PTR                                     std::shared_ptr<TrackerBintr>
#define DSL_TRACKER_NEW(name, configFile, width, height)    std::shared_ptr<TrackerBintr>(new TrackerBintr(name, configFile, width, height))

class TrackerBintr : public Bintr {
public:
    TrackerBintr(const char *name, const char *configFile, guint width, guint height);
    ~TrackerBintr();

    /**
     * @brief  获取该GieBintr使用的跟踪器配置文件的名称
     * @return 用于创建此文件的完全限定的patspec
     */
    const char *GetLlConfigFile();

    /**
     * @brief  获取这个PrimaryGieBintr使用的跟踪器库文件的名称
     * @return 用于创建此文件的完全限定的patspec
     */
    const char *GetLlLibFile();

    /**
     * @brief 将TrackerBintr添加到父分支Bintr
     * @param[in] pParentBintr 父管道，将此Bintr添加到其中
     */
    bool AddToParent(DSL_BASE_PTR pParentBintr);

    /**
     * @brief 将这个TrackerBintr从其父分支Bintr中移除
     * @param[in] pParentBintr parent要从中删除的管道
     * @return 添加成功时为true，否则为false
     */
    bool RemoveFromParent(DSL_BASE_PTR pParentBintr);

    /**
     * @brief  链接此目录下的所有子元素
     * @return 如果所有链接都成功，则为true，否则为false
     */
    bool LinkAll();

    /**
     * @brief 解除链接该目录下所有的子元素。在未链接状态下调用UnlinkAll无效
     */
    void UnlinkAll();

    /**
     * @brief  获取此TrackerBintr正在使用的低级库的路径
     * @return 正在使用的库文件的绝对路径
     */
    const char *GetLibFile();

    /**
     * @brief 设置为这个TrackerBintr使用的低级库文件
     * @param[in] libFile 要使用的Lib文件的完全绝对或相对路径
     * @return 更新成功时为true，否则为false
     */
    bool SetLibFile(const char *libFile);

    /**
     * @brief  获取此TrackerBintr正在使用的配置文件的路径
     * @return 使用的配置文件的绝对路径
     */
    const char *GetConfigFile();

    /**
     * @brief 设置这个TrackerBintr使用的配置文件
     * @param[in] configFile 要使用的Lib文件的完全绝对或相对路径
     * @return 更新成功时为true，否则为false
     */
    bool SetConfigFile(const char *configFile);

    /**
     * @brief 获取此跟踪器的当前宽度和高度设置
     * @param[out] width  以像素为单位的当前宽度设置
     * @param[out] height 当前高度设置(以像素为单位)
     */ 
    void GetDimensions(uint *width, uint *height);

    /**
     * @brief 设置此跟踪器的当前宽度和高度设置。调用者需要提供有效的宽度和高度值
     * @param[in] width  要以像素为单位设置的宽度值
     * @param[in] height 要以像素为单位设置的高度值
     * @return 如果跟踪器当前正在使用，则为false。否则为true
     */ 
    bool SetDimensions(uint width, uint hieght);

    /**
     * @brief  获取此跟踪器当前启用批处理的设置
     * @return 启用时为true，否则为false
     */
    boolean GetBatchProcessingEnabled();

    /**
     * @brief  为此跟踪器设置启用批处理设置
     * @return 设置为true表示启用，否则设置为false
     * Note:   这个调用只有在底层库同时支持批处理和逐流处理时才有效
     */
    bool SetBatchProcessingEnabled(boolean enabled);

    /**
     * @brief  获取此跟踪器的"过去帧启用"设置
     * @return 启用时为true，否则为false
     */
    boolean GetPastFrameReportingEnabled();

    /**
     * @brief  设置此跟踪器的"过去帧"设置
     * @return 如果启用则设置为true，否则设置为false
     * Note:   这个调用只有在底层库支持过去的框架报告时才有效
     */
    bool SetPastFrameReportingEnabled(boolean enabled);

    /**
     * @brief 这个Bintr使用常见的SetBatchSize Bintr方法来检查批处理是否被禁用，以及管道的batchSize是否>1。如果发现这种情况为真，该函数将记录一条警告消息
     * @param batchSize 管线batchSize检查
     * @return true
     */
    bool SetBatchSize(uint batchSize);

    /**
     * @brief  设置所有元素的GPU ID
     * @return 成功设置为true，否则为false
     */
    bool SetGpuId(uint gpuId);

private:
    std::string     m_llConfigFile;             // 路径规范到这个TrackerBintr使用的跟踪器配置文件
    std::string     m_llLibFile;                // 路径规范到这个TrackerBintr使用的跟踪器库文件
    uint            m_width;                    // 输入缓冲区的最大帧宽度(以像素为单位)
    uint            m_height;                   // 输入缓冲区的最大帧高度(以像素为单位)
    DSL_ELEMENT_PTR m_pTracker;                 // TrackerBintr的跟踪器元素
    boolean         m_batchProcessingEnabled;   // 如果设置了enable-batch-processing设置，则为True，否则为false
    boolean         m_pastFrameReporting;       // 如果设置了enable-past-frame设置，则为True，否则为false
};
}

#endif
