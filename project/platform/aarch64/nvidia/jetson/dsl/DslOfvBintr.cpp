#include "Dsl.h"
#include "DslElementr.h"
#include "DslOfvBintr.h"
#include "DslPipelineBintr.h"

namespace DSL
{

    OfvBintr::OfvBintr(const char* name)
        : Bintr(name)
    {
        LOG_FUNC();
        
        m_pOptFlowQueue = DSL_ELEMENT_EXT_NEW("queue", name, "nvof");
        m_pOptFlow = DSL_ELEMENT_NEW("nvof", name);
        m_pOptFlowVisualQueue = DSL_ELEMENT_EXT_NEW("queue", name, "nvofvisual");
        m_pOptFlowVisual = DSL_ELEMENT_NEW("nvofvisual", name);
        
        AddChild(m_pOptFlowQueue);
        AddChild(m_pOptFlow);
        AddChild(m_pOptFlowVisualQueue);
        AddChild(m_pOptFlowVisual);

        m_pOptFlowQueue->AddGhostPadToParent("sink");
        m_pOptFlowVisual->AddGhostPadToParent("src");

        m_pSinkPadProbe = DSL_PAD_BUFFER_PROBE_NEW("osd-sink-pad-probe", "sink", m_pOptFlowQueue);
        m_pSrcPadProbe = DSL_PAD_BUFFER_PROBE_NEW("osd-src-pad-probe", "src", m_pOptFlowVisual);
    }    
    
    OfvBintr::~OfvBintr()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {    
            UnlinkAll();
        }
    }

    bool OfvBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("OfvBintr '" << m_name << "' is already linked");
            return false;
        }
        if (!m_pOptFlowQueue->LinkToSink(m_pOptFlow) or
            !m_pOptFlow->LinkToSink(m_pOptFlowVisualQueue) or
            !m_pOptFlowVisualQueue->LinkToSink(m_pOptFlowVisual))
        {
            return false;
        }
        m_isLinked = true;
        return true;
    }
    
    void OfvBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("OfvBintr '" << m_name << "' is not linked");
            return;
        }
        m_pOptFlowQueue->UnlinkFromSink();
        m_pOptFlow->UnlinkFromSink();
        m_pOptFlowVisualQueue->UnlinkFromSink();
        m_isLinked = false;
    }
    
    bool OfvBintr::AddToParent(DSL_BASE_PTR pBranchBintr)
    {
        LOG_FUNC();
        
        // add 'this' OSD to the Parent Pipeline 
        return std::dynamic_pointer_cast<BranchBintr>(pBranchBintr)->
            AddOfvBintr(shared_from_this());
    }
    
}
