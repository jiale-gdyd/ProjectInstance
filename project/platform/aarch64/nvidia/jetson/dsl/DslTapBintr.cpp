#include "Dsl.h"
#include "DslTapBintr.h"
#include "DslBranchBintr.h"

namespace DSL
{

    TapBintr::TapBintr(const char* name)
        : Bintr(name)
    {
        LOG_FUNC();

        m_pQueue = DSL_ELEMENT_NEW("queue", name);
        AddChild(m_pQueue);
        m_pQueue->AddGhostPadToParent("sink");
    }

    TapBintr::~TapBintr()
    {
        LOG_FUNC();
    }

    //-------------------------------------------------------------------------
    
    RecordTapBintr::RecordTapBintr(const char* name, const char* outdir, 
        uint container, dsl_record_client_listener_cb clientListener)
        : TapBintr(name)
        , RecordMgr(name, outdir, m_gpuId, container, clientListener)
    {
        LOG_FUNC();
        
        LOG_INFO("");
        LOG_INFO("Initial property values for RecordTapBintr '" << name << "'");
        LOG_INFO("  outdir             : " << outdir);
        LOG_INFO("  container          : " << container);
    }
    
    RecordTapBintr::~RecordTapBintr()
    {
        LOG_FUNC();
    
        if (IsLinked())
        {    
            UnlinkAll();
        }
    }

    bool RecordTapBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("RecordTapBintr '" << GetName() << "' is already linked");
            return false;
        }

        if (!CreateContext())
        {
            return false;
        }
        
        // Create a new GstNodetr to wrap the record-bin
        m_pRecordBin = DSL_GSTNODETR_NEW("record-bin");
        m_pRecordBin->SetGstObject(GST_OBJECT(m_pContext->recordbin));
            
        AddChild(m_pRecordBin);
        
        if (!m_pQueue->LinkToSink(m_pRecordBin))
        {
            LOG_ERROR("RecordTapBintr '" << GetName() << "' failed to link");
            return false;
        }
        m_isLinked = true;
        return true;
    }
    
    void RecordTapBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("RecordTapBintr '" << GetName() << "' is not linked");
            return;
        }
        m_pQueue->UnlinkFromSink();
        
        RemoveChild(m_pRecordBin);

        // Destroy the RecordBin GSTNODETR and context.
        m_pRecordBin = nullptr;
        DestroyContext();
        
        m_isLinked = false;
    }
        
    void RecordTapBintr::HandleEos()
    {
        LOG_FUNC();
        
        if (IsOn())
        {
            LOG_INFO("RecordTapBintr '" << GetName() 
                << "' is in session, stopping to handle the EOS");
            StopSession(true);
        }
    }

}
