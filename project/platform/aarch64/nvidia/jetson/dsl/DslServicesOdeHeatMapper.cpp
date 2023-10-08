#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"

namespace DSL
{

    DslReturnType Services::OdeHeatMapperNew(const char* name,
        uint cols, uint rows, uint bboxTestPoint, const char* colorPalette)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure HeatMapper name uniqueness 
            if (m_odeHeatMappers.find(name) != m_odeHeatMappers.end())
            {   
                LOG_ERROR("ODE Heat-Mapper name '" << name 
                    << "' is not unique");
                return DSL_RESULT_ODE_HEAT_MAPPER_NAME_NOT_UNIQUE;
            }
            DSL_RETURN_IF_DISPLAY_TYPE_NAME_NOT_FOUND(m_displayTypes, colorPalette);
            DSL_RETURN_IF_DISPLAY_TYPE_IS_NOT_CORRECT_TYPE(m_displayTypes, 
                colorPalette, RgbaColorPalette);

            DSL_RGBA_COLOR_PALETTE_PTR pColorPalette = 
                std::dynamic_pointer_cast<RgbaColorPalette>(
                    m_displayTypes[colorPalette]);
            
            m_odeHeatMappers[name] = DSL_ODE_HEAT_MAPPER_NEW(name,
                cols, rows, bboxTestPoint, pColorPalette);
            
            LOG_INFO("New ODE Heat-Mapper '" << name 
                << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("New ODE Heat-Mapper '" << name 
                << "' threw exception on create");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperColorPaletteGet(const char* name,
        const char** colorPalette)
    {    
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            *colorPalette = 
                m_odeHeatMappers[name]->GetColorPalette()->GetName().c_str();

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' returned RGBA Color Palette successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception getting RGBA Color Palette");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::OdeHeatMapperColorPaletteSet(const char* name,
        const char* colorPalette)
    {    
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            DSL_RETURN_IF_DISPLAY_TYPE_NAME_NOT_FOUND(m_displayTypes, colorPalette);
            DSL_RETURN_IF_DISPLAY_TYPE_IS_NOT_CORRECT_TYPE(m_displayTypes, 
                colorPalette, RgbaColorPalette);

            DSL_RGBA_COLOR_PALETTE_PTR pColorPalette = 
                std::dynamic_pointer_cast<RgbaColorPalette>(
                    m_displayTypes[colorPalette]);
            
            if (!m_odeHeatMappers[name]->SetColorPalette(pColorPalette))
            {
                LOG_ERROR("ODE HeatMapper '" << name 
                    << "' failed to set RGBA Color Palette");
                return DSL_RESULT_ODE_HEAT_MAPPER_SET_FAILED;
            }

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' returned RGBA Color Palette successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception getting RGBA Color Palette");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperLegendSettingsGet(const char* name,
        boolean* enabled, uint* location, uint* width, uint* height)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            bool bEnabled(false);
            
            m_odeHeatMappers[name]->GetLegendSettings(&bEnabled,
                location, width, height);
            *enabled = bEnabled;

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' returned Legend Settings successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception getting Legend Settings");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperLegendSettingsSet(const char* name,
        boolean enabled, uint location, uint width, uint height)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            if (!m_odeHeatMappers[name]->SetLegendSettings(enabled,
                location, width, height))
            {
                LOG_ERROR("ODE HeatMapper '" << name 
                    << "' failed to set Legend Settings");
                return DSL_RESULT_ODE_HEAT_MAPPER_SET_FAILED;
            }

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' set Legend Settings successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception getting Legend Settings");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperMetricsClear(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            m_odeHeatMappers[name]->ClearMetrics();

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' cleared its metrics successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception printing metrics");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperMetricsGet(const char* name,
        const uint64_t** buffer, uint* size)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            m_odeHeatMappers[name]->GetMetrics(buffer, size);

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' printed its metrics to the console successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception printing metrics");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperMetricsPrint(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            m_odeHeatMappers[name]->PrintMetrics();

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' printed its metrics to the console successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception printing metrics");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperMetricsLog(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            m_odeHeatMappers[name]->LogMetrics();

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' Logged its metrics at level = INFO successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception loggin metrics");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperMetricsFile(const char* name,
        const char* filePath, uint mode, uint format)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            if (!m_odeHeatMappers[name]->FileMetrics(filePath, mode, format))
            {
                LOG_ERROR("ODE HeatMapper '" << name 
                    << "' failed to log metrics");
                return DSL_RESULT_ODE_HEAT_MAPPER_SET_FAILED;
            }

            LOG_INFO("ODE Heat-Mapper '" << name 
                << "' Logged its metrics at level = INFO successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception loggin metrics");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    DslReturnType Services::OdeHeatMapperDelete(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_HEAT_MAPPER_NAME_NOT_FOUND(m_odeHeatMappers, name);
            
            if (m_odeHeatMappers[name]->IsInUse())
            {
                LOG_INFO("ODE HeatMapper '" << name << "' is in use");
                return DSL_RESULT_ODE_HEAT_MAPPER_IN_USE;
            }
            m_odeHeatMappers.erase(name);

            LOG_INFO("ODE Heat-Mapper '" << name << "' deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE HeatMapper '" << name 
                << "' threw an exception on deletion");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::OdeHeatMapperDeleteAll()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            if (m_odeHeatMappers.empty())
            {
                return DSL_RESULT_SUCCESS;
            }
            for (auto const& imap: m_odeHeatMappers)
            {
                // In the case of Delete all
                if (imap.second->IsInUse())
                {
                    LOG_ERROR("ODE Heat-Mapper '" << imap.second->GetName() 
                        << "' is currently in use");
                    return DSL_RESULT_ODE_HEAT_MAPPER_IN_USE;
                }
            }
            m_odeHeatMappers.clear();

            LOG_INFO("All ODE Heat-Mappers deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Heat-Mapper API threw an exception on delete all");
            return DSL_RESULT_ODE_HEAT_MAPPER_THREW_EXCEPTION;
        }
    }

    uint Services::OdeHeatMapperListSize()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        return m_odeHeatMappers.size();
    }
    
}
