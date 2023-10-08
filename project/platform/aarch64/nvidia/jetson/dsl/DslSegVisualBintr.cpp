#include "Dsl.h"
#include "DslSegVisualBintr.h"
#include "DslBranchBintr.h"

namespace DSL
{
    SegVisualBintr::SegVisualBintr(const char* name, 
        uint width, uint height)
        : Bintr(name)
        , m_width(width)
        , m_height(height)
    {
        LOG_FUNC();
        
        m_pQueue = DSL_ELEMENT_NEW("queue", name);
        m_pSegVisual = DSL_ELEMENT_NEW("nvsegvisual", name);
        
        m_pSegVisual->SetAttribute("width", m_width);
        m_pSegVisual->SetAttribute("height", m_height);
        m_pSegVisual->SetAttribute("gpu-id", m_gpuId);

        LOG_INFO("");
        LOG_INFO("Initial property values for SegVisualBintr '" << name << "'");
        LOG_INFO("  width             : " << m_width);
        LOG_INFO("  height            : " << m_height);
        LOG_INFO("  gpu-id            : " << m_gpuId);

        AddChild(m_pQueue);
        AddChild(m_pSegVisual);
        
        m_pQueue->AddGhostPadToParent("sink");
        m_pSegVisual->AddGhostPadToParent("src");
        
        m_pSrcPadProbe = DSL_PAD_BUFFER_PROBE_NEW("segvisual-src-pad-probe", 
            "src", m_pSegVisual);
    }

    SegVisualBintr::~SegVisualBintr()
    {
        LOG_FUNC();
    }

    bool SegVisualBintr::LinkAll()
    {
        LOG_FUNC();

        if (!m_batchSize)
        {
            LOG_ERROR("SegVisualBintr '" << GetName() 
                << "' can not be linked: batch size = 0");
            return false;
        }
        if (m_isLinked)
        {
            LOG_ERROR("SegVisualBintr '" << GetName() << "' is already linked");
            return false;
        }
        if (!m_pQueue->LinkToSink(m_pSegVisual))
        {
            return false;
        }
        
        m_isLinked = true;
        
        return true;
    }
    
    void SegVisualBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("SegVisualBintr '" << GetName() << "' is not linked");
            return;
        }
        m_pQueue->UnlinkFromSink();

        m_isLinked = false;
    }

    bool SegVisualBintr::AddToParent(DSL_BASE_PTR pParentBintr)
    {
        LOG_FUNC();
        
        // add 'this' SegVisualBintr to the Parent Pipeline 
        return std::dynamic_pointer_cast<BranchBintr>(pParentBintr)->
            AddSegVisualBintr(shared_from_this());
    }

    void SegVisualBintr::GetDimensions(uint* width, uint* height)
    {
        LOG_FUNC();
        
        *width = m_width;
        *height = m_height;
    }

    bool SegVisualBintr::SetDimensions(uint width, uint height)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set dimensions for SegVisualBintr '" << GetName() 
                << "' as it's currently in use");
            return false;
        }

        m_width = width;
        m_height = height;

        m_pSegVisual->SetAttribute("width", m_width);
        m_pSegVisual->SetAttribute("height", m_height);
        
        return true;
    }

    bool SegVisualBintr::SetBatchSize(uint batchSize)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set batch-size for SegVisualBintr '" << GetName() 
                << "' as it's currently linked");
            return false;
        }
        
        LOG_INFO("Setting batch size to '" << batchSize 
            << "' for SegVisualBintr '" << GetName() << "'");
        
        m_batchSize = batchSize;
        
        m_pSegVisual->SetAttribute("batch-size", m_batchSize);
        return true;
    };


    bool SegVisualBintr::SetGpuId(uint gpuId)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set GPU ID for SegVisualBintr '" << GetName() 
                << "' as it's currently in use");
            return false;
        }
        m_gpuId = gpuId;
        m_pSegVisual->SetAttribute("gpu-id", m_batchSize);
        
        LOG_INFO("SegVisualBintr '" << GetName() 
            << "' - new GPU ID = " << m_gpuId );

        return true;
    }
    
}
