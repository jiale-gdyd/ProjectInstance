#include "Dsl.h"
#include "DslTrackerBintr.h"
#include "DslBranchBintr.h"

namespace DSL
{
    TrackerBintr::TrackerBintr(const char* name,
        const char* configFile, guint width, guint height)
        : Bintr(name)
        , m_llLibFile(NVDS_MOT_LIB)
        , m_llConfigFile(configFile)
        , m_width(width)
        , m_height(height)
    {
        LOG_FUNC();

        // New Tracker element for this TrackerBintr
        m_pTracker = DSL_ELEMENT_NEW("nvtracker", name);

        m_pTracker->SetAttribute("tracker-width", m_width);
        m_pTracker->SetAttribute("tracker-height", m_height);
        m_pTracker->SetAttribute("gpu-id", m_gpuId);
        m_pTracker->SetAttribute("ll-lib-file", m_llLibFile.c_str());

        // set the low-level configuration file property if provided.
        if (m_llConfigFile.size())
        {
            m_pTracker->SetAttribute("ll-config-file", configFile);
        }

        // Get property defaults that aren't specifically set
        m_pTracker->GetAttribute("gpu-id", &m_gpuId);
        m_pTracker->GetAttribute("enable-batch-process", &m_batchProcessingEnabled);
        m_pTracker->GetAttribute("enable-past-frame", &m_pastFrameReporting);

        LOG_INFO("");
        LOG_INFO("Initial property values for TrackerBintr '" << name << "'");
        LOG_INFO("  tracker-width        : " << m_width);
        LOG_INFO("  tracker-height       : " << m_height);
        LOG_INFO("  ll-lib-file          : " << m_llLibFile);
        LOG_INFO("  ll-config-file       : " << m_llConfigFile);
        LOG_INFO("  gpu-id               : " << m_gpuId);
        LOG_INFO("  enable-batch-process : " << m_batchProcessingEnabled);
        LOG_INFO("  enable-past-frame    : " << m_pastFrameReporting);

        AddChild(m_pTracker);

        m_pTracker->AddGhostPadToParent("sink");
        m_pTracker->AddGhostPadToParent("src");
        
        m_pSinkPadProbe = DSL_PAD_BUFFER_PROBE_NEW("tracker-sink-pad-probe", "sink", m_pTracker);
        m_pSrcPadProbe = DSL_PAD_BUFFER_PROBE_NEW("tracker-src-pad-probe", "src", m_pTracker);
    }

    TrackerBintr::~TrackerBintr()
    {
        LOG_FUNC();

        if (IsLinked())
        {
            UnlinkAll();
        }
    }

    bool TrackerBintr::AddToParent(DSL_BASE_PTR pParentBintr)
    {
        LOG_FUNC();
        
        // add 'this' Tracker to the Parent Branch 
        return std::dynamic_pointer_cast<BranchBintr>(pParentBintr)->
            AddTrackerBintr(shared_from_this());
    }

    bool TrackerBintr::RemoveFromParent(DSL_BASE_PTR pParentBintr)
    {
        LOG_FUNC();
        
        // remove 'this' Tracker from the Parent Branch
        return std::dynamic_pointer_cast<BranchBintr>(pParentBintr)->
            RemoveTrackerBintr(shared_from_this());
    }
    
    bool TrackerBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("TrackerBintr '" << m_name << "' is already linked");
            return false;
        }
        // Nothing to link with single Elementr
        m_isLinked = true;
        
        return true;
    }
    
    void TrackerBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("TrackerBintr '" << m_name << "' is not linked");
            return;
        }
        // Nothing to unlink with single Elementr
        m_isLinked = false;
    }

    const char* TrackerBintr::GetLibFile()
    {
        LOG_FUNC();
        
        return m_llLibFile.c_str();
    }
    
    bool TrackerBintr::SetLibFile(const char* libFile)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set library file for TrackerBintr '" << GetName() 
                << "' as it's currently linked");
            return false;
        }
        m_llLibFile.assign(libFile);
        m_pTracker->SetAttribute("ll-lib-file", libFile);
        return true;
    }
    
    const char* TrackerBintr::GetConfigFile()
    {
        LOG_FUNC();
        
        return m_llConfigFile.c_str();
    }
    
    bool TrackerBintr::SetConfigFile(const char* configFile)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set config file for TrackerBintr '" << GetName() 
                << "' as it's currently linked");
            return false;
        }
        m_llConfigFile.assign(configFile);
        m_pTracker->SetAttribute("ll-config-file", configFile);
        return true;
    }
    
    void TrackerBintr::GetDimensions(uint* width, uint* height)
    {
        LOG_FUNC();
        
        m_pTracker->GetAttribute("tracker-width", &m_width);
        m_pTracker->GetAttribute("tracker-height", &m_height);
        
        *width = m_width;
        *height = m_height;
    }

    bool TrackerBintr::SetDimensions(uint width, uint height)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set Dimensions for TrackerBintr '" << GetName() 
                << "' as it's currently linked");
            return false;
        }

        m_width = width;
        m_height = height;

        m_pTracker->SetAttribute("tracker-width", m_width);
        m_pTracker->SetAttribute("tracker-height", m_height);
        
        return true;
    }

    bool TrackerBintr::SetGpuId(uint gpuId)
    {
        LOG_FUNC();
        
        if (IsInUse())
        {
            LOG_ERROR("Unable to set GPU ID for FileSinkBintr '" << GetName() 
                << "' as it's currently in use");
            return false;
        }

        m_gpuId = gpuId;
        m_pTracker->SetAttribute("gpu-id", m_gpuId);
        
        LOG_INFO("TrackerBintr '" << GetName() 
            << "' - new GPU ID = " << m_gpuId );

        return true;
    }

    boolean TrackerBintr::GetBatchProcessingEnabled()
    {
        LOG_FUNC();

        return m_batchProcessingEnabled;
    }
    
    bool TrackerBintr::SetBatchProcessingEnabled(boolean enabled)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set the enable-batch-processing setting for TrackerBintr '" 
                << GetName() << "' as it's currently in use");
            return false;
        }
        
        m_batchProcessingEnabled = enabled;
        m_pTracker->SetAttribute("enable-batch-process", m_batchProcessingEnabled);
        return true;
    }
    
    boolean TrackerBintr::GetPastFrameReportingEnabled()
    {
        LOG_FUNC();

        return m_pastFrameReporting;
    }
    
    bool TrackerBintr::SetPastFrameReportingEnabled(boolean enabled)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set the enable-past-frame setting for TrackerBintr '" 
                << GetName() << "' as it's currently in use");
            return false;
        }
        m_pastFrameReporting = enabled;
        m_pTracker->SetAttribute("enable-past-frame", m_pastFrameReporting);
        return true;
    }

    bool TrackerBintr::SetBatchSize(uint batchSize)
    {
        LOG_FUNC();
        
        if (batchSize > 1 and !m_batchProcessingEnabled)
        {
            LOG_WARN("The Pipeline's batch-size is set to " << batchSize 
                << " while the Tracker's batch processing is disable!");
        }
        return true;
    }
    
}
