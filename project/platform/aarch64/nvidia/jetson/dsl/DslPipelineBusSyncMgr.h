#ifndef DSL_DSL_PIPELINE_XWIN_MGR_H
#define DSL_DSL_PIPELINE_XWIN_MGR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBintr.h"
#include "DslSourceBintr.h"
#include "DslSinkBintr.h"

namespace DSL
{

     class PipelineBusSyncMgr
    {
    public: 
    
        PipelineBusSyncMgr(const GstObject* pGstPipeline);

        ~PipelineBusSyncMgr();
        
        /**
         * @brief handles incoming sync messages
         * @param[in] message incoming message to process
         * @return [GST_BUS_PASS|GST_BUS_FAIL]
         */
        GstBusSyncReply HandleBusSyncMessage(GstMessage* pMessage);
        
    protected:
    
        /**
         * @brief Shared client cb mutex - owned by the the PipelineBusSyncMgr
         * but shared amoungst all child Window Sinks. Mutex will clear
         * on last unreference.
         */
        std::shared_ptr<DslMutex> m_pSharedClientCbMutex;

    private:

        /**
         * @brief mutex to prevent callback reentry
         */
        DslMutex m_busSyncMutex;
    };
    
    /**
     * @brief 
     * @param[in] bus instance pointer
     * @param[in] message incoming message packet to process
     * @param[in] pData pipeline instance pointer
     * @return [GST_BUS_PASS|GST_BUS_FAIL]
     */
    static GstBusSyncReply bus_sync_handler(
        GstBus* bus, GstMessage* pMessage, gpointer pData);
}

#endif //  DSL_PIPELINE_XWIN_MGR_H
