#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"
#include "DslInferBintr.h"

namespace DSL
{
    DslReturnType Services::BranchNew(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        if (m_components[name])
        {   
            LOG_ERROR("Branch name '" << name << "' is not unique");
            return DSL_RESULT_BRANCH_NAME_NOT_UNIQUE;
        }
        try
        {
            m_components[name] = std::shared_ptr<Bintr>(new BranchBintr(name));
        }
        catch(...)
        {
            LOG_ERROR("New Branch '" << name << "' threw exception on create");
            return DSL_RESULT_BRANCH_THREW_EXCEPTION;
        }
        LOG_INFO("New BRANCH '" << name << "' created successfully");

        return DSL_RESULT_SUCCESS;
    }

    DslReturnType Services::BranchComponentAdd(const char* branch, 
        const char* component)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, branch);
        DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, component);

        try
        {
            // Can't add components if "is-in-use" by another Pipeline
            if (m_components[component]->IsInUse())
            {
                LOG_ERROR("Unable to add component '" << component 
                    << "' as it's currently in use");
                return DSL_RESULT_COMPONENT_IN_USE;
            }

            if (!m_components[component]->AddToParent(m_components[branch]))
            {
                LOG_ERROR("Branch '" << branch
                    << "' failed to add component '" << component << "'");
                return DSL_RESULT_BRANCH_COMPONENT_ADD_FAILED;
            }
            LOG_INFO("Component '" << component 
                << "' was added to Branch '" << branch << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Branch '" << branch
                << "' threw exception adding component '" << component << "'");
            return DSL_RESULT_BRANCH_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::BranchComponentRemove(const char* branch, 
        const char* component)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        try
        {
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, branch);
            DSL_RETURN_IF_COMPONENT_NAME_NOT_FOUND(m_components, component);

            if (!m_components[component]->IsParent(m_components[branch]))
            {
                LOG_ERROR("Component '" << component << 
                    "' is not in use by Branch '" << branch << "'");
                return DSL_RESULT_COMPONENT_NOT_USED_BY_BRANCH;
            }
            if (!m_components[component]->RemoveFromParent(m_components[branch]))
            {
                LOG_ERROR("Branch '" << branch
                    << "' failed to remove component '" << component << "'");
                return DSL_RESULT_BRANCH_COMPONENT_REMOVE_FAILED;
            }
            LOG_INFO("Component '" << component 
                << "' was removed from Branch '" << branch << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Branch '" << branch 
                << "' threw an exception removing component");
            return DSL_RESULT_BRANCH_COMPONENT_REMOVE_FAILED;
        }
    }

}
