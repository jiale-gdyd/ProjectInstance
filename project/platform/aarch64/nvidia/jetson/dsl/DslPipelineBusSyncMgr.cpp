#include "Dsl.h"
#include "DslPipelineBusSyncMgr.h"
#include "DslServices.h"

namespace DSL
{
    PipelineBusSyncMgr::PipelineBusSyncMgr(const GstObject* pGstPipeline)
    {
        LOG_FUNC();

        GstBus* pGstBus = gst_pipeline_get_bus(GST_PIPELINE(pGstPipeline));

        // install the sync handler for the message bus
        gst_bus_set_sync_handler(pGstBus, bus_sync_handler, this, NULL);        

        gst_object_unref(pGstBus);

        m_pSharedClientCbMutex = std::shared_ptr<DslMutex>(new DslMutex());
    }

    PipelineBusSyncMgr::~PipelineBusSyncMgr()
    {
        LOG_FUNC();
        
    }
    
    GstBusSyncReply PipelineBusSyncMgr::HandleBusSyncMessage(GstMessage* pMessage)
    {
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_busSyncMutex);

        switch (GST_MESSAGE_TYPE(pMessage))
        {
        case GST_MESSAGE_ELEMENT:
        
            if (gst_is_video_overlay_prepare_window_handle_message(pMessage))
            {
                // A Window Sink component is signaling to prepare 
                // the window handle. Call into the Window-Sink registry services
                // to get the Owner of the nveglglessink element

                DSL_WINDOW_SINK_PTR pWindowSink =
                    std::dynamic_pointer_cast<WindowSinkBintr>(
                        DSL::Services::GetServices()->_sinkWindowGet(
                            GST_MESSAGE_SRC(pMessage)));
                        
                // If the sink is found -- should always be true.
                if (pWindowSink)
                {
                    pWindowSink->PrepareWindowHandle(m_pSharedClientCbMutex);
                }
                else
                {
                    LOG_ERROR("Failed to find WindowSinkBintr in registry");
                }
                    
                UNREF_MESSAGE_ON_RETURN(pMessage);
                return GST_BUS_DROP;
            }
            break;
        default:
            break;
        }
        return GST_BUS_PASS;
    }

    static GstBusSyncReply bus_sync_handler(GstBus* bus, 
        GstMessage* pMessage, gpointer pData)
    {
        return static_cast<PipelineBusSyncMgr*>(pData)->
            HandleBusSyncMessage(pMessage);
    }

}
