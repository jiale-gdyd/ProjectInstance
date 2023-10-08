#ifndef DSL_DSL_TILER_BINTR_H
#define DSL_DSL_TILER_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBintr.h"
#include "DslElementr.h"

namespace DSL {
#define DSL_TILER_PTR                       std::shared_ptr<TilerBintr>
#define DSL_TILER_NEW(name, width, height)  std::shared_ptr<TilerBintr>(new TilerBintr(name, width, height))

class TilerBintr : public Bintr {
public:
    TilerBintr(const char *name, uint width, uint height);
    ~TilerBintr();

    /**
     * @brief 将TilerBintr添加到父管道Bintr中
     * @param[in] pParentBintr 父管道，将此Bintr添加到其中
     */
    bool AddToParent(DSL_BASE_PTR pParentBintr);

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
     * @brief 获取TilerBintr的当前行数和列数
     * @param[out] rows    当前行数
     * @param[out] columns 当前列数
     */
    void GetTiles(uint *columns, uint *rows);

    /**
     * @brief 设置TilerBintr的行数和列数。调用者需要提供有效的行和列值
     * @param[in] rows    要设置的行数
     * @param[in] columns 要设置的列数
     * @return 如果TilerBintr当前处于使用状态，则为false。否则为true
     */
    bool SetTiles(uint columns, uint rows);

    /**
     * @brief 获取此TilerBintr的当前宽度和高度设置
     * @param[out] width  以像素为单位的当前宽度设置
     * @param[out] height 当前高度设置(以像素为单位)
     */ 
    void GetDimensions(uint *width, uint *height);

    /**
     * @brief 为TilerBintr设置当前的宽度和高度。调用者需要提供有效的宽度和高度值
     * @param[in] width  要以像素为单位设置的宽度值
     * @param[in] height 要以像素为单位设置的高度值
     * @return 如果TilerBintr当前处于使用状态，则为false。否则为true
     */ 
    bool SetDimensions(uint width, uint hieght);

    /**
     * @brief  获取帧数加法器的当前启用状态
     * @return 当前启用状态
     */
    bool GetFrameNumberingEnabled();

    /**
     * @brief 设置帧数加法器的使能状态
     * @param enabled 设置为true为启用，false为禁用
     * @return 如果状态被正确更新，则为true
     */
    bool SetFrameNumberingEnabled(bool enabled);

    /**
     * @brief  获取TilerBintr的当前显示源设置
     * @return 当前显示源设置，-1等于显示的所有源/磁贴
     */
    void GetShowSource(int *sourceId, uint *timeout);

    /**
     * @brief 将TilerBintr的当前显示源设置设置为单个源
     * @param[in] sourceId      要使用的新显示源设置
     * Note:                    sourceId必须小于当前的批处理大小，在管线被链接/播放之前为0
     * @param[in] timeout       以秒为单位显示当前源的时间
     * @param[in] hasPrecedence 如果为true，将优先于当前显示的单个源
     * @return 如果设置值成功，则为true，否则为false
     */
    bool SetShowSource(int sourceId, uint timeout, bool hasPrecedence);

    /**
     * @brief 用于显示源定时器操作的处理程序
     */
    int HandleShowSourceTimer();

    /**
     * @brief 将show source设置为-1，显示所有源。show-source超时(如果正在运行)将在调用时取消
     */
    void ShowAllSources();

    /**
     * @brief 循环遍历所有源，直到超时为止
     * @param[in] timeout 以秒为单位显示当前源的时间
     */
    bool CycleAllSources(uint timeout);

    /**
     * @brief  设置所有元素的GPU ID
     * @return 成功设置为true，否则为false
     */
    bool SetGpuId(uint gpuId);

    /**
     * @brief 设置NVIDIA缓冲存储器类型
     * @param[in] nvbufMemType 要使用的新内存类型，DSL_NVBUF_MEM_TYPE常量之一
     * @return 成功设置为true，否则为false
     */
    bool SetNvbufMemType(uint nvbufMemType);

private:
    uint                            m_rows;                 // TilerBintr的行数
    uint                            m_columns;              // TilerBintr的列数
    uint                            m_width;                // TilerBintr的宽度，以像素为单位
    uint                            m_height;               // TilerBintr的高度，以像素为单位
    uint                            m_computeHw;            // 计算缩放要使用的硬件。只适用于Jetson。0(默认):默认，GPU为Tesla，VIC为Jetson; 1 (GPU): GPU; 2 (VIC): VIC
    bool                            m_frameNumberingEnabled;// 如果帧数加法器已经启用，可以为每个帧添加帧数，穿过TilerBintr的Source pad

    DSL_PPEH_FRAME_NUMBER_ADDER_PTR m_pFrameNumberAdder;    // frame - number Adder Pad Probe Handler，当启用时，将frame_number添加到穿过TilerBintr的Source Pad的每一帧
    DSL_ELEMENT_PTR                 m_pQueue;               // 队列元素作为此tilerbin的Sink
    DSL_ELEMENT_PTR                 m_pTiler;               // tile元素作为这个tilerbin的源
    DslMutex                        m_showSourceMutex;      // 防止回调重入的互斥锁
    int                             m_showSourceId;         // 当前show-source id， -1 == show-a
    uint                            m_showSourceTimeout;    // 客户端提供的超时(以秒为单位)
    uint                            m_showSourceCounter;    // 显示源计数计数器值。0 == 时间过期
    uint                            m_showSourceTimerId;    // Show-source timer-id，非零==当前正在运行
    bool                            m_showSourceCycle;      // 如果启用源循环，则为true，否则为false
};

static int ShowSourceTimerHandler(void *user_data);
}

#endif
