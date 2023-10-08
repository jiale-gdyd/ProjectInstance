#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"

namespace DSL
{
    DslReturnType Services::ComponentDelete(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
        
        if (m_components[name]->IsInUse())
        {
            LOG_INFO("Component '" << name << "' is in use");
            return DSL_RESULT_COMPONENT_IN_USE;
        }
        m_components.erase(name);

        LOG_INFO("Component '" << name << "' deleted successfully");

        return DSL_RESULT_SUCCESS;
    }
    
    DslReturnType Services::ComponentDeleteAll()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            if (m_components.empty())
            {
                return DSL_RESULT_SUCCESS;
            }
            // Only if there are Pipelines do we check if the component is in use.
            if (m_pipelines.size())
            {
                for (auto const& imap: m_components)
                {
                    // In the case of Delete all
                    if (imap.second->IsInUse())
                    {
                        LOG_ERROR("Component '" << imap.second->GetName() << "' is currently in use");
                        return DSL_RESULT_COMPONENT_IN_USE;
                    }
                }
            }

            m_components.clear();
            LOG_INFO("All Components deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("DSL threw exception on Delete All Components");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }

    uint Services::ComponentListSize()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        return m_components.size();
    }

    DslReturnType Services::ComponentGpuIdGet(const char* name, uint* gpuid)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            
            *gpuid = m_components[name]->GetGpuId();

            LOG_INFO("Current GPU ID = " << *gpuid 
                << " for component '" << name << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Component '" << name 
                << "' threw exception getting GPU Id");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::ComponentGpuIdSet(const char* name, uint gpuid)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            
            if (!m_components[name]->SetGpuId(gpuid))
            {
                LOG_INFO("Component '" << name 
                    << "' faild to set GPU Id = " << gpuid);
                return DSL_RESULT_COMPONENT_SET_GPUID_FAILED;
            }

            LOG_INFO("New GPU ID = " << gpuid 
                << " for component '" << name << "'");

            return DSL_RESULT_SUCCESS;
            }
        catch(...)
        {
            LOG_ERROR("Component '" << name 
                << "' threw exception setting GPU Id");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
}

DslReturnType Services::ComponentNvbufMemTypeGet(const char* name, uint* type)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);
            
            *type = m_components[name]->GetNvbufMemType();

            LOG_INFO("Current NVIDIA buffer memory type = " << *type 
                << " for component '" << name << "'");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Component '" << name 
                << "' threw exception getting NVIDIA buffer memory type");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::ComponentNvbufMemTypeSet(const char* name, uint type)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, name);

            if (type > DSL_NVBUF_MEM_TYPE_SURFACE_ARRAY)
            {
                LOG_ERROR("Invalid NVIDIA buffer memory type = " << type 
                    << " for component '"  << name << "'");
                return DSL_RESULT_COMPONENT_SET_NVBUF_MEM_TYPE_FAILED;
            }
            
            if (!m_components[name]->SetNvbufMemType(type))
            {
                LOG_INFO("Component '" << name 
                    << "' faild to set NVIDIA buffer memory type = " << type);
                return DSL_RESULT_COMPONENT_SET_NVBUF_MEM_TYPE_FAILED;
            }

            LOG_INFO("NVIDIA buffer memorytype = " << type 
                << " set for component '" << name << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Component '" << name 
                << "' threw exception setting NVIDIA buffer memory type");
            return DSL_RESULT_COMPONENT_THREW_EXCEPTION;
        }
    }
}
