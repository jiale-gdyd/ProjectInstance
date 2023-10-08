#ifndef DSL_DSL_TAP_BINTR_H
#define DSL_DSL_TAP_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBintr.h"
#include "DslElementr.h"
#include "DslRecordMgr.h"

#include <gst-nvdssr.h>

namespace DSL {
#define DSL_TAP_PTR                                                     std::shared_ptr<TapBintr>
#define DSL_RECORD_TAP_PTR                                              std::shared_ptr<RecordTapBintr>
#define DSL_RECORD_TAP_NEW(name, outdir, container, clientListener)     std::shared_ptr<RecordTapBintr>(new RecordTapBintr(name, outdir, container, clientListener))

class TapBintr : public Bintr {
public:
    TapBintr(const char *name);
    ~TapBintr();

    virtual void HandleEos() = 0;

protected:
    DSL_ELEMENT_PTR m_pQueue;   // Queue元素作为所有Tap bin的sink
};

class RecordTapBintr : public TapBintr, public RecordMgr {
public:
    RecordTapBintr(const char *name, const char *outdir, uint container, dsl_record_client_listener_cb clientListener);
    ~RecordTapBintr();

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
     * @brief 由EOS上的父源调用。如果录制会话正在进行，RecordTap将尝试停止录制会话
     */
    void HandleEos();

private:
    DSL_GSTNODETR_PTR m_pRecordBin; // 节点封装NVIDIA的Record Bin
};
}

#endif
