#include "Dsl.h"
#include "DslElementr.h"
#include "DslPreprocBintr.h"
#include "DslPipelineBintr.h"

namespace DSL
{
    // Initilize the unique id list for all PreprocBintrs 
    std::list<uint> PreprocBintr::s_uniqueIds;

    PreprocBintr::PreprocBintr(const char* name, const char* configFile)
        : Bintr(name)
        , m_uniqueId(0)
        , m_configFile(configFile)
        , m_enabled(true) // enabled by default.
    {
        LOG_FUNC();

        // Find the first available unique Id
        while(std::find(s_uniqueIds.begin(), s_uniqueIds.end(), m_uniqueId) != s_uniqueIds.end())
        {
            m_uniqueId++;
        }
        s_uniqueIds.push_back(m_uniqueId);
        
        m_pPreproc = DSL_ELEMENT_NEW("nvdspreprocess", name);
        m_pQueue = DSL_ELEMENT_NEW("queue", name);

        m_pPreproc->SetAttribute("unique-id", m_uniqueId);
        m_pPreproc->SetAttribute("gpu-id", m_gpuId);
        m_pPreproc->SetAttribute("config-file", configFile);
        m_pPreproc->SetAttribute("enable", m_enabled);

        LOG_INFO("");
        LOG_INFO("Initial property values for PreprocBintr '" << name << "'");
        LOG_INFO("  config-file        : " << m_configFile);
        LOG_INFO("  unique-id          : " << m_uniqueId);
        LOG_INFO("  enabled            : " << m_enabled);
        LOG_INFO("  gpu-id             : " << m_gpuId);
        
        AddChild(m_pPreproc);
        AddChild(m_pQueue);

        m_pQueue->AddGhostPadToParent("sink");
        m_pPreproc->AddGhostPadToParent("src");

        m_pSinkPadProbe = DSL_PAD_BUFFER_PROBE_NEW("pre-proc-sink-pad-probe", "sink", m_pQueue);
        m_pSrcPadProbe = DSL_PAD_BUFFER_PROBE_NEW("pre-proc-pad-probe", "src", m_pPreproc);
    }    
    
    PreprocBintr::~PreprocBintr()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {    
            UnlinkAll();
        }
        s_uniqueIds.remove(m_uniqueId);
    }

    bool PreprocBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("PreprocBintr '" << m_name << "' is already linked");
            return false;
        }
        
        if (!m_pQueue->LinkToSink(m_pPreproc))
        {
            return false;
        }
        m_isLinked = true;
        return true;
    }
    
    void PreprocBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("PreprocBintr '" << m_name << "' is not linked");
            return;
        }
        m_pQueue->UnlinkFromSink();
        m_isLinked = false;
    }
    
    bool PreprocBintr::AddToParent(DSL_BASE_PTR pBranchBintr)
    {
        LOG_FUNC();
        
        // add 'this' OSD to the Parent Pipeline 
        return std::dynamic_pointer_cast<BranchBintr>(pBranchBintr)->
            AddPreprocBintr(shared_from_this());
    }

    bool PreprocBintr::RemoveFromParent(DSL_BASE_PTR pBranchBintr)
    {
        LOG_FUNC();
        
        // remove 'this' OSD to the Parent Pipeline 
        return std::dynamic_pointer_cast<BranchBintr>(pBranchBintr)->
            RemovePreprocBintr(shared_from_this());
    }

    bool PreprocBintr::SetGpuId(uint gpuId)
    {
        LOG_FUNC();
        
        if (IsInUse())
        {
            LOG_ERROR("Unable to set GPU ID for PreprocBintr '" << GetName() 
                << "' as it's currently in use");
            return false;
        }

        m_gpuId = gpuId;
        LOG_DEBUG("Setting GPU ID to '" << gpuId << "' for PreprocBintr '" << m_name << "'");

        m_pPreproc->SetAttribute("gpu-id", m_gpuId);
        
        LOG_INFO("PreprocBintr '" << GetName() 
            << "' - new GPU ID = " << m_gpuId );

        return true;
    }

    const char* PreprocBintr::GetConfigFile()
    {
        LOG_FUNC();
        
        return m_configFile.c_str();
    }
    
    bool PreprocBintr::SetConfigFile(const char* configFile)
    {
        LOG_FUNC();
        
        if (IsLinked())
        {
            LOG_ERROR("Unable to set config file for PreprocBintr '" << GetName() 
                << "' as it's currently linked");
            return false;
        }
        m_pPreproc->SetAttribute("config-file", configFile);
        m_configFile.assign(configFile);

        return true;
    }
    
    bool PreprocBintr::GetEnabled()
    {
        LOG_FUNC();
        
        return m_enabled;
    }
    
    bool PreprocBintr::SetEnabled(bool enabled)
    {
        LOG_FUNC();

        if (IsLinked())
        {
            LOG_ERROR("Unable to set enabled parameter for PreprocBintr '" << GetName() 
                << "' as it's currently linked");
            return false;
        }
        m_enabled = enabled;
        m_pPreproc->SetAttribute("enable", m_enabled);

        return true;
    }
    
    uint PreprocBintr::GetUniqueId()
    {
        LOG_FUNC();
        
        return m_uniqueId;
    }
    
}
