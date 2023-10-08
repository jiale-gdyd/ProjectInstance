#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"

namespace DSL
{

    DslReturnType Services::PreprocNew(const char* name, 
        const char* configFile)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure component name uniqueness 
            if (m_components.find(name) != m_components.end())
            {   
                LOG_ERROR("Preprocessor name '" << name << "' is not unique");
                return DSL_RESULT_PREPROC_NAME_NOT_UNIQUE;
            }

            std::string testPath(configFile);
            if (testPath.size())
            {
                LOG_INFO("Preprocessor config file: " << configFile);
                
                std::ifstream streamConfigFile(configFile);
                if (!streamConfigFile.good())
                {
                    LOG_ERROR("Preprocessor config file not found");
                    return DSL_RESULT_PREPROC_CONFIG_FILE_NOT_FOUND;
                }
            }
            m_components[name] = DSL_PREPROC_NEW(
                name, configFile);

            LOG_INFO("New Preprocessor '" << name << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Preprocessor '" << name 
                << "' threw exception on create");
            return DSL_RESULT_PREPROC_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PreprocConfigFileGet(const char* name, 
        const char** configFile)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PREPROC_NAME_NOT_FOUND(m_components, name);
            
            DSL_PREPROC_PTR pPreprocBintr = 
                std::dynamic_pointer_cast<PreprocBintr>(m_components[name]);

            *configFile = pPreprocBintr->GetConfigFile();

            LOG_INFO("Preprocessor '" << name << "' returned Config File = '"
                << *configFile << "' successfully");
            
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Preprocessor '" << name 
                << "' threw exception getting the Config File pathspec");
            return DSL_RESULT_PREPROC_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PreprocConfigFileSet(const char* name, 
        const char* configFile)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PREPROC_NAME_NOT_FOUND(m_components, name);

            std::string testPath(configFile);
            if (testPath.size())
            {
                LOG_INFO("Preprocessor config file: " << configFile);
                
                std::ifstream streamConfigFile(configFile);
                if (!streamConfigFile.good())
                {
                    LOG_ERROR("Preprocessor config file not found");
                    return DSL_RESULT_PREPROC_CONFIG_FILE_NOT_FOUND;
                }
            }
            
            DSL_PREPROC_PTR pPreprocBintr = 
                std::dynamic_pointer_cast<PreprocBintr>(m_components[name]);

            if (!pPreprocBintr->SetConfigFile(configFile))
            {
                LOG_ERROR("Preprocessor '" << name 
                    << "' failed to set the config file");
                return DSL_RESULT_PREPROC_SET_FAILED;
            }
            LOG_INFO("Preprocessor '" << name << "' set config file = '"
                << configFile << "' successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Preprocessor '" << name 
                << "' threw exception setting Config file");
            return DSL_RESULT_PREPROC_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PreprocEnabledGet(const char* name, 
        boolean* enabled)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PREPROC_NAME_NOT_FOUND(m_components, name);

            DSL_PREPROC_PTR pPreprocBintr = 
                std::dynamic_pointer_cast<PreprocBintr>(m_components[name]);

            *enabled = pPreprocBintr->GetEnabled();

            LOG_INFO("Preprocessor '" << name << "' returned Enabed = " 
                << *enabled  << " successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Preprocessor '" << name 
                << "' threw an exception getting enabled setting");
            return DSL_RESULT_PREPROC_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PreprocEnabledSet(const char* name, 
        boolean enabled)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        try
        {
            DSL_RETURN_IF_PREPROC_NAME_NOT_FOUND(m_components, name);

            DSL_PREPROC_PTR pPreprocBintr = 
                std::dynamic_pointer_cast<PreprocBintr>(m_components[name]);

            if (!pPreprocBintr->SetEnabled(enabled))
            {
                LOG_ERROR("Preprocessor '" << name 
                    << "' failed to set enabled setting");
                return DSL_RESULT_PREPROC_SET_FAILED;
            }
            LOG_INFO("Preprocessor '" << name 
                << "' set Enabed = " << enabled  << " successfully");
            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Preprocessor '" << name 
                << "' threw an exception setting enabled setting");
            return DSL_RESULT_PREPROC_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::PreprocUniqueIdGet(const char* name, 
        uint* uniqueId)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_PREPROC_NAME_NOT_FOUND(m_components, name);

            DSL_PREPROC_PTR pPreprocBintr = 
                std::dynamic_pointer_cast<PreprocBintr>(m_components[name]);

            *uniqueId = pPreprocBintr->GetEnabled();

            LOG_INFO("Preprocessor '" << name << "' returned unique Id = " 
                << *uniqueId  << " successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("Preprocessor '" << name 
                << "' threw an exception getting unique Id");
            return DSL_RESULT_PREPROC_THREW_EXCEPTION;
        }
    }

}
