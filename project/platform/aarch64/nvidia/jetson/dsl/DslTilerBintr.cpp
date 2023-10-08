#include "Dsl.h"
#include "DslTilerBintr.h"
#include "DslBranchBintr.h"

namespace DSL
{

    TilerBintr::TilerBintr(const char* name, uint width, uint height)
        : Bintr(name)
        , m_width(width)
        , m_height(height)
        , m_rows(0)
        , m_columns(0)
        , m_frameNumberingEnabled(false)
        , m_showSourceTimeout(0)
        , m_showSourceCounter(0)
        , m_showSourceTimerId(0)
        , m_showSourceCycle(false)
    {
        LOG_FUNC();

        m_pQueue = DSL_ELEMENT_NEW("queue", name);
        m_pTiler = DSL_ELEMENT_NEW("nvmultistreamtiler", name);

        // Don't overwrite the default "best-fit" columns and rows on construction
        m_pTiler->SetAttribute("width", m_width);
        m_pTiler->SetAttribute("height", m_height);
        
        m_pTiler->SetAttribute("gpu-id", m_gpuId);
        m_pTiler->SetAttribute("nvbuf-memory-type", m_nvbufMemType);

        // Get property defaults that aren't specifically set
        m_pTiler->GetAttribute("show-source", &m_showSourceId);
        m_pTiler->GetAttribute("gpu-id", &m_gpuId);
        m_pTiler->GetAttribute("compute-hw", &m_computeHw);

        LOG_INFO("");
        LOG_INFO("Initial property values for TilerBintr '" << name << "'");
        LOG_INFO("  rows              : " << m_rows);
        LOG_INFO("  columns           : " << m_columns);
        LOG_INFO("  width             : " << m_width);
        LOG_INFO("  height            : " << m_height);
        LOG_INFO("  show-source       : " << m_showSourceId);
        LOG_INFO("  gpu-id            : " << m_gpuId);
        LOG_INFO("  nvbuf-memory-type : " << m_nvbufMemType);
        LOG_INFO("  compute-hw        : " << m_computeHw);

        AddChild(m_pQueue);
        AddChild(m_pTiler);

        m_pQueue->AddGhostPadToParent("sink");
        m_pTiler->AddGhostPadToParent("src");
    
        m_pSinkPadProbe = DSL_PAD_BUFFER_PROBE_NEW("tiler-sink-pad-probe", "sink", m_pQueue);
        m_pSrcPadProbe = DSL_PAD_BUFFER_PROBE_NEW("tiler-src-pad-probe", "src", m_pTiler);
        
        // temp solution to ensure PPH is always executed first - alphabetically
        std::string adderName("___"); 
        adderName += name;
        m_pFrameNumberAdder = DSL_PPEH_FRAME_NUMBER_ADDER_NEW(adderName.c_str());
    }

    TilerBintr::~TilerBintr()
    {
        LOG_FUNC();

        if (m_isLinked)
        {    
            UnlinkAll();
        }
        if (m_showSourceTimerId)
        {
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
            
            g_source_remove(m_showSourceTimerId);
        }
    }

    bool TilerBintr::AddToParent(DSL_BASE_PTR pParentBintr)
    {
        LOG_FUNC();
        
        // add 'this' tiler to the Parent Pipeline 
        return std::dynamic_pointer_cast<BranchBintr>(pParentBintr)->
            AddTilerBintr(shared_from_this());
    }
    
    bool TilerBintr::LinkAll()
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("TilerBintr '" << m_name << "' is already linked");
            return false;
        }
        m_pQueue->LinkToSink(m_pTiler);
        m_isLinked = true;
        
        m_pFrameNumberAdder->ResetFrameNumber();
        
        return true;
    }
    
    void TilerBintr::UnlinkAll()
    {
        LOG_FUNC();
        
        if (!m_isLinked)
        {
            LOG_ERROR("TilerBintr '" << m_name << "' is not linked");
            return;
        }
        m_pQueue->UnlinkFromSink();
        m_isLinked = false;
    }
    
    void TilerBintr::GetTiles(uint* columns, uint* rows)
    {
        LOG_FUNC();
        
        *columns = m_columns;
        *rows = m_rows;
    }
    
    bool TilerBintr::SetTiles(uint columns, uint rows)
    {
        LOG_FUNC();
        
        m_columns = columns;
        m_rows = rows;
    
        m_pTiler->SetAttribute("columns", columns);
        m_pTiler->SetAttribute("rows", m_rows);
        
        return true;
    }
    
    void TilerBintr::GetDimensions(uint* width, uint* height)
    {
        LOG_FUNC();
        
        m_pTiler->GetAttribute("width", &m_width);
        m_pTiler->GetAttribute("height", &m_height);
        
        *width = m_width;
        *height = m_height;
    }

    bool TilerBintr::SetDimensions(uint width, uint height)
    {
        LOG_FUNC();

        m_width = width;
        m_height = height;

        m_pTiler->SetAttribute("width", m_width);
        m_pTiler->SetAttribute("height", m_height);
        
        return true;
    }

    bool TilerBintr::GetFrameNumberingEnabled()
    {
        LOG_FUNC();
        
        return m_frameNumberingEnabled;
    }

    bool TilerBintr::SetFrameNumberingEnabled(bool enabled)
    {
        LOG_FUNC();
        
        if (m_frameNumberingEnabled and enabled)
        {
            LOG_ERROR("Can't enable frame-numbering for Tiler '" <<
                GetName() << "' as it's already enabled ");
            return false;
        }
        if (!m_frameNumberingEnabled and !enabled)
        {
            LOG_ERROR("Can't disabled frame-numbering for Tiler '" <<
                GetName() << "' as it's already disabled ");
            return false; 
        }
        m_frameNumberingEnabled = enabled;
        
        if(m_frameNumberingEnabled)
        {
            return AddPadProbeHandler(m_pFrameNumberAdder, DSL_PAD_SRC);
        }
        return RemovePadProbeHandler(m_pFrameNumberAdder, DSL_PAD_SRC);
    }
    

    void TilerBintr::GetShowSource(int* sourceId, uint* timeout)
    {
        LOG_FUNC();
        
        *sourceId = m_showSourceId;
        *timeout = m_showSourceTimeout;
    }

    bool TilerBintr::SetShowSource(int sourceId, uint timeout, bool hasPrecedence)
    {
        // Not logging function entry/exit as this serice can be called by actions for
        // every object in every frame.
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);

        if (sourceId < 0)
        {
            LOG_ERROR("Invalid source Id '" << sourceId << "' for TilerBintr '" << GetName());
            return false;
        }

        // call has Precendence over source cycling 
        m_showSourceCycle = false;
        if (sourceId != m_showSourceId)
        {
            if (m_showSourceTimerId and !hasPrecedence)
            {
                // don't log error as this may be common with ODE Triggers and Actions calling
                LOG_DEBUG("Show source Timer is running for Source '" << m_showSourceId << 
                   "' New Source '" << sourceId << "' without precedence can not be shown");
                return false;
            }

            m_showSourceId = sourceId;
            m_showSourceTimeout = timeout;
            m_pTiler->SetAttribute("show-source", m_showSourceId);

            m_showSourceCounter = m_showSourceTimeout*10;
            if (m_showSourceCounter)
            {
                LOG_INFO("Adding show-source timer with timeout = " << timeout << "' for TilerBintr '" << GetName());
                m_showSourceTimerId = g_timeout_add(100, ShowSourceTimerHandler, this);
            }
            return true;
        }
            
        // otherwise it's the same source.
        
        m_showSourceTimeout = timeout;
        m_showSourceCounter = timeout*10;
        if (!m_showSourceTimerId and m_showSourceCounter)
        {
            LOG_INFO("Adding show-source timer with timeout = " << timeout << "' for TilerBintr '" << GetName());
            m_showSourceTimerId = g_timeout_add(100, ShowSourceTimerHandler, this);
        }
        return true;
    }
    
    bool TilerBintr::CycleAllSources(uint timeout)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
        
        if (!timeout)
        {
            LOG_ERROR("Timeout value can not be 0 when enabling cycle-all-sources for TilerBintr '" << GetName());
            return false;
        }
        // if the timer is currently running, stop and remove first.
        if (m_showSourceTimerId)
        {
            g_source_remove(m_showSourceTimerId);
            m_showSourceTimerId = 0;
        }

        m_showSourceCycle = true;
        m_showSourceId = 0;
        m_showSourceTimeout = timeout;
        m_pTiler->SetAttribute("show-source", m_showSourceId);

        m_showSourceCounter = m_showSourceTimeout*10;
            
        if (!m_showSourceTimerId and m_showSourceCounter)
        {
            LOG_INFO("Adding show-source timer with timeout = " << timeout << "' for TilerBintr '" << GetName());
            m_showSourceTimerId = g_timeout_add(100, ShowSourceTimerHandler, this);
        }
        return true;
    }
    
    
    void TilerBintr::ShowAllSources()
    {
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
        
        if (m_showSourceTimerId)
        {
            g_source_remove(m_showSourceTimerId);
            m_showSourceTimerId = 0;
            // call has Precendence over source cycling 
            m_showSourceCycle = false;
        }
        if (m_showSourceId != -1)
        {
            m_showSourceId = -1;
            m_pTiler->SetAttribute("show-source", m_showSourceId);
        }
    }

    int TilerBintr::HandleShowSourceTimer()
    {
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_showSourceMutex);
        
        // Tiler is no longer linked but the main_loop and timer are still running
        if (!IsLinked())
        {
            // do nothing, but keep the timer running to support relink and play
            // The Timer's Cycle Source setting should remain as is.
            return true;
        }
        if (--m_showSourceCounter == 0)
        {
            // if we are cycling through sources
            if (m_showSourceCycle)
            {
                // reset the timeout counter, cycle to the next source, and return true to continue
                m_showSourceCounter = m_showSourceTimeout*10;
                m_showSourceId = (m_showSourceId+1)%m_batchSize;
                m_pTiler->SetAttribute("show-source", m_showSourceId);
                return true;
            }
            // otherwise, reset the timer Id, show all sources, and return false to destroy the timer
            m_showSourceTimerId = 0;
            m_showSourceId = -1;
            m_pTiler->SetAttribute("show-source", m_showSourceId);
            return false;
        }
        return true;
    }

    bool TilerBintr::SetGpuId(uint gpuId)
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("Unable to set GPU ID for TilerBintr '" << GetName() 
                << "' as it's currently in use");
            return false;
        }

        m_gpuId = gpuId;
        m_pTiler->SetAttribute("gpu-id", m_gpuId);
        
        LOG_INFO("TilerBintr '" << GetName() 
            << "' - new GPU ID = " << m_gpuId );
        
        return true;
    }

    bool TilerBintr::SetNvbufMemType(uint nvbufMemType)
    {
        LOG_FUNC();
        
        if (m_isLinked)
        {
            LOG_ERROR("Unable to set NVIDIA buffer memory type for TilerBintr '" 
                << GetName() << "' as it's currently linked");
            return false;
        }
        m_nvbufMemType = nvbufMemType;
        m_pTiler->SetAttribute("nvbuf-memory-type", m_nvbufMemType);

        return true;
    }

    //----------------------------------------------------------------------------------------------
    
    static int ShowSourceTimerHandler(void* user_data)
    {
        return static_cast<TilerBintr*>(user_data)->
            HandleShowSourceTimer();
    }
}
