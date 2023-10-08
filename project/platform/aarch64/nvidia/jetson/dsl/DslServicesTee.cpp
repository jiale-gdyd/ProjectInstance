#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"
#include "DslMultiBranchesBintr.h"

namespace DSL
{
    DslReturnType Services::TeeDemuxerNew(const char* name, 
        uint maxBranches)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure component name uniqueness 
            if (m_components.find(name) != m_components.end())
            {   
                LOG_ERROR("Demuxer Tee name '" << name << "' is not unique");
                return DSL_RESULT_TEE_NAME_NOT_UNIQUE;
            }
            m_components[name] = std::shared_ptr<Bintr>(new DemuxerBintr(name,
                maxBranches));
            
            LOG_INFO("New Demuxer Tee '" << name << "' created successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("New Demuxer Tee '" << name << "' threw exception on create");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::TeeDemuxerMaxBranchesGet(const char* name, 
        uint* maxBranches)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, name, 
                DemuxerBintr);
            
            DSL_DEMUXER_PTR pDemuxerBintr 
                = std::dynamic_pointer_cast<DemuxerBintr>(m_components[name]);
            
            *maxBranches = pDemuxerBintr->GetMaxBranches();
                
            LOG_INFO("Demuxer '" << name 
                << "' returned max-brances = " << *maxBranches << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Demuxer '" << name 
                << "' threw an exception getting max-branches");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    
    
    DslReturnType Services::TeeDemuxerMaxBranchesSet(const char* name, 
        uint maxBranches)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, name, 
                DemuxerBintr);
            
            DSL_DEMUXER_PTR pDemuxerBintr 
                = std::dynamic_pointer_cast<DemuxerBintr>(m_components[name]);
            
            if (!pDemuxerBintr->SetMaxBranches(maxBranches))
            {
                LOG_ERROR("Demuxer '" << name << 
                    "' failed set max-branches = " << maxBranches);
                return DSL_RESULT_TEE_SET_FAILED;
            }
                
            LOG_INFO("Demuxer '" << name 
                << "' set max-brances = " << maxBranches << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Demuxer '" << name 
                << "' threw an exception setting max-branches");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    

    DslReturnType Services::TeeSplitterNew(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure component name uniqueness 
            if (m_components.find(name) != m_components.end())
            {   
                LOG_ERROR("Splitter Tee name '" << name << "' is not unique");
                return DSL_RESULT_TILER_NAME_NOT_UNIQUE;
            }
            m_components[name] = std::shared_ptr<Bintr>(new SplitterBintr(name));
            
            LOG_INFO("New Splitter Tee '" << name << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("New Splitter Tee '" << name << "' threw exception on create");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::TeeDemuxerBranchAddTo(const char* name, 
        const char* branch, uint streamId)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, branch);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, 
                name, DemuxerBintr);
            DSL_RETURN_IF_COMPONENT_IS_NOT_BRANCH(m_components, branch);
            
            // Can't add components if they're In use by another Branch
            if (m_components[branch]->IsInUse())
            {
                LOG_ERROR("Unable to add branch '" << branch 
                    << "' as it's currently in use");
                return DSL_RESULT_COMPONENT_IN_USE;
            }
            // Cast the Branch to a Bintr to call the correct AddChild method.
            DSL_BINTR_PTR pBranchBintr = 
                std::dynamic_pointer_cast<Bintr>(m_components[branch]);

            if (!std::dynamic_pointer_cast<DemuxerBintr>(
                    m_components[name])->AddChildTo(pBranchBintr, streamId))
            {
                LOG_ERROR("Demuxer '" << name << 
                    "' failed to add branch '" << branch 
                    << "' at stream-id = " << streamId);
                return DSL_RESULT_TEE_BRANCH_ADD_FAILED;
            }
                
            LOG_INFO("Branch '" << branch 
                << "' was added to Demuxer Tee '" << name << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Demuxer Tee '" << name 
                << "' threw an exception adding branch '" << branch << "'");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    

    DslReturnType Services::TeeDemuxerBranchMoveTo(const char* name, 
        const char* branch, uint streamId)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, branch);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, 
                name, DemuxerBintr);
            DSL_RETURN_IF_COMPONENT_IS_NOT_BRANCH(m_components, branch);
            
            DSL_BINTR_PTR pBranchBintr = 
                std::dynamic_pointer_cast<Bintr>(m_components[branch]);

            if (!std::dynamic_pointer_cast<DemuxerBintr>(
                    m_components[name])->MoveChildTo(pBranchBintr, streamId))
            {
                LOG_ERROR("Demuxer '" << name << 
                    "' failed to move branch '" << branch 
                    << "' to stream-id = " << streamId);
                return DSL_RESULT_TEE_BRANCH_MOVE_FAILED;
            }
                
            LOG_INFO("Branch '" << branch 
                << "' was moved to stream-id = " << streamId 
                <<  " for Demuxer Tee '" << name << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Demuxer Tee '" << name 
                << "' threw an exception moving branch '" << branch << "'");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    

   
    DslReturnType Services::TeeBranchAdd(const char* name, 
        const char* branch)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, branch);
            DSL_RETURN_IF_COMPONENT_IS_NOT_TEE(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_BRANCH(m_components, branch);
            
            // Can't add components if they're In use by another Branch
            if (m_components[branch]->IsInUse())
            {
                LOG_ERROR("Unable to add branch '" << branch 
                    << "' as it's currently in use");
                return DSL_RESULT_COMPONENT_IN_USE;
            }
            // Cast the Branch to a Bintr to call the correct AddChild method.
            DSL_BINTR_PTR pBranchBintr = 
                std::dynamic_pointer_cast<Bintr>(m_components[branch]);

            bool retval;
            
            // We need to check which of the two types the Tee is and cast accordingly
            if (m_components[name]->IsType(typeid(SplitterBintr)))
            {
                retval = std::dynamic_pointer_cast<SplitterBintr>(
                    m_components[name])->AddChild(pBranchBintr);
            }
            else
            {
                retval = std::dynamic_pointer_cast<DemuxerBintr>(
                    m_components[name])->AddChild(pBranchBintr);
            }
            if (!retval)
            {
                LOG_ERROR("Tee '" << name << 
                    "' failed to add branch '" << branch << "'");
                return DSL_RESULT_TEE_BRANCH_ADD_FAILED;
            }
                
            LOG_INFO("Branch '" << branch 
                << "' was added to Tee '" << name << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" << name 
                << "' threw an exception adding branch '" << branch << "'");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    
    
    DslReturnType Services::TeeBranchRemove(const char* name, 
        const char* branch)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_TEE(m_components, name);
            DSL_RETURN_IF_BRANCH_NAME_NOT_FOUND(m_components, branch);

            DSL_MULTI_BRANCHES_PTR pTeeBintr = 
                std::dynamic_pointer_cast<MultiBranchesBintr>(m_components[name]);

            if (!pTeeBintr->IsChild(m_components[branch]))
            {
                LOG_ERROR("Branch '" << branch << 
                    "' is not in use by Tee '" << name << "'");
                return DSL_RESULT_TEE_BRANCH_IS_NOT_CHILD;
            }

            // Cast the Branch to a Bintr to call the correct AddChile method.
            DSL_BINTR_PTR pBranchBintr = 
                std::dynamic_pointer_cast<Bintr>(m_components[branch]);

            if (!pTeeBintr->RemoveChild(pBranchBintr))
            {
                LOG_ERROR("Tee '" << name << 
                    "' failed to remove branch '" << branch << "'");
                return DSL_RESULT_TEE_BRANCH_REMOVE_FAILED;
            }
            LOG_INFO("Branch '" << branch 
                << "' was removed from Tee '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" << name 
                << "' threw an exception removing branch '" << branch << "'");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::TeeBranchRemoveAll(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_TEE(m_components, name);

            DSL_MULTI_BRANCHES_PTR pTeeBintr = 
                std::dynamic_pointer_cast<MultiBranchesBintr>(m_components[name]);
                
            // pTeeBintr->RemoveAll();
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" <<  name
                << "' threw an exception removing all branches");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::TeeBranchCountGet(const char* name, uint* count)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_TEE(m_components, name);

            DSL_MULTI_BRANCHES_PTR pTeeBintr = 
                std::dynamic_pointer_cast<MultiBranchesBintr>(m_components[name]);

            *count = pTeeBintr->GetNumChildren();
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" <<  name
                << "' threw an exception getting branch count");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::TeeBlockingTimeoutGet(const char* name, 
        uint* timeout)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, name, 
                DemuxerBintr);
            
            DSL_MULTI_BRANCHES_PTR pTeeBintr = 
                std::dynamic_pointer_cast<MultiBranchesBintr>(m_components[name]);
            
            *timeout = pTeeBintr->GetBlockingTimeout();
                
            LOG_INFO("Tee '" << name 
                << "' returned blocking-timeout = " << *timeout 
                << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" << name 
                << "' threw an exception getting blocking-timeout");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    
    
    DslReturnType Services::TeeBlockingTimeoutSet(const char* name, 
        uint timeout)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_CORRECT_TYPE(m_components, name, 
                DemuxerBintr);
            
            DSL_MULTI_BRANCHES_PTR pTeeBintr = 
                std::dynamic_pointer_cast<MultiBranchesBintr>(m_components[name]);
            
            if (!pTeeBintr->SetBlockingTimeout(timeout))
            {
                LOG_ERROR("Tee '" << name << 
                    "' failed to set blocking-timeout = " << timeout);
                return DSL_RESULT_TEE_SET_FAILED;
            }
                
            LOG_INFO("Tee '" << name 
                << "' set blocking-timeout = " << timeout << "' successfully");
                
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" << name 
                << "' threw an exception setting blocking-timeout");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }    

    DslReturnType Services::TeePphAdd(const char* name, const char* handler)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_TEE(m_components, name);
            DSL_RETURN_IF_PPH_NAME_NOT_FOUND(m_padProbeHandlers, handler);

            // call on the Handler to add itself to the Tee as a PadProbeHandler
            if (!m_padProbeHandlers[handler]->AddToParent(m_components[name], 
                DSL_PAD_SINK))
            {
                LOG_ERROR("Tee '" << name << "' failed to add Pad Probe Handler");
                return DSL_RESULT_TEE_HANDLER_ADD_FAILED;
            }
            LOG_INFO("Pad Probe Handler '" << handler 
                << "' added to Tee '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tiler '" << name 
                << "' threw an exception adding Pad Probe Handler");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }
   
    DslReturnType Services::TeePphRemove(const char* name, const char* handler) 
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            DSL_RETURN_IF_COMPONENT_IS_NOT_TEE(m_components, name);
            DSL_RETURN_IF_PPH_NAME_NOT_FOUND(m_padProbeHandlers, handler);

            // call on the Handler to remove itself from the Tee
            if (!m_padProbeHandlers[handler]->RemoveFromParent(m_components[name], 
                DSL_PAD_SINK))
            {
                LOG_ERROR("Pad Probe Handler '" << handler 
                    << "' is not a child of Tee '" << name << "'");
                return DSL_RESULT_TEE_HANDLER_REMOVE_FAILED;
            }
            LOG_INFO("Pad Probe Handler '" << handler 
                << "' removed from Tee '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Tee '" << name 
                << "' threw an exception removing Pad Probe Handler");
            return DSL_RESULT_TEE_THREW_EXCEPTION;
        }
    }

}
