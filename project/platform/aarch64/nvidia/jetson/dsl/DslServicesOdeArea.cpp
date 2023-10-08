#include "Dsl.h"
#include "DslApi.h"
#include "DslServices.h"
#include "DslServicesValidate.h"
#include "DslOdeArea.h"

namespace DSL
{
    DslReturnType Services::OdeAreaInclusionNew(const char* name, 
        const char* polygon, boolean show, uint bboxTestPoint)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure ODE Area name uniqueness 
            if (m_odeAreas.find(name) != m_odeAreas.end())
            {   
                LOG_ERROR("ODE Area name '" << name << "' is not unique");
                return DSL_RESULT_ODE_AREA_NAME_NOT_UNIQUE;
            }
            DSL_RETURN_IF_DISPLAY_TYPE_NAME_NOT_FOUND(m_displayTypes, polygon);
            
            DSL_RETURN_IF_DISPLAY_TYPE_IS_NOT_CORRECT_TYPE(m_displayTypes, 
                polygon, RgbaPolygon);
            
            if (bboxTestPoint > DSL_BBOX_POINT_ANY)
            {
                LOG_ERROR("Bounding box test point value of '" << bboxTestPoint << 
                    "' is invalid when creating ODE Inclusion Area '" << name << "'");
                return DSL_RESULT_ODE_AREA_PARAMETER_INVALID;
            }
            
            DSL_RGBA_POLYGON_PTR pPolygon = 
                std::dynamic_pointer_cast<RgbaPolygon>(m_displayTypes[polygon]);
            
            m_odeAreas[name] = DSL_ODE_AREA_INCLUSION_NEW(name, 
                pPolygon, show, bboxTestPoint);
         
            LOG_INFO("New ODE Inclusion Area '" << name 
                << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Inclusion Area '" << name 
                << "' threw exception on creation");
            return DSL_RESULT_ODE_AREA_THREW_EXCEPTION;
        }
    }                

    DslReturnType Services::OdeAreaExclusionNew(const char* name, 
        const char* polygon, boolean show, uint bboxTestPoint)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure ODE Area name uniqueness 
            if (m_odeAreas.find(name) != m_odeAreas.end())
            {   
                LOG_ERROR("ODE Area name '" << name << "' is not unique");
                return DSL_RESULT_ODE_AREA_NAME_NOT_UNIQUE;
            }
            DSL_RETURN_IF_DISPLAY_TYPE_NAME_NOT_FOUND(m_displayTypes, polygon);
            
            DSL_RETURN_IF_DISPLAY_TYPE_IS_NOT_CORRECT_TYPE(m_displayTypes, 
                polygon, RgbaPolygon);

            if (bboxTestPoint > DSL_BBOX_POINT_ANY)
            {
                LOG_ERROR("Bounding box test point value of '" << bboxTestPoint << 
                    "' is invalid when creating ODE Exclusion Area '" << name << "'");
                return DSL_RESULT_ODE_AREA_PARAMETER_INVALID;
            }

            DSL_RGBA_POLYGON_PTR pPolygon = 
                std::dynamic_pointer_cast<RgbaPolygon>(m_displayTypes[polygon]);
            
            m_odeAreas[name] = DSL_ODE_AREA_EXCLUSION_NEW(name, 
                pPolygon, show, bboxTestPoint);
         
            LOG_INFO("New ODE Exclusion Area '" << name << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Exclusion Area '" << name << "' threw exception on creation");
            return DSL_RESULT_ODE_AREA_THREW_EXCEPTION;
        }
    }                
    
    DslReturnType Services::OdeAreaLineNew(const char* name, 
        const char* line, boolean show, uint bboxTestPoint)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure ODE Area name uniqueness 
            if (m_odeAreas.find(name) != m_odeAreas.end())
            {   
                LOG_ERROR("ODE Area name '" << name << "' is not unique");
                return DSL_RESULT_ODE_AREA_NAME_NOT_UNIQUE;
            }
            DSL_RETURN_IF_DISPLAY_TYPE_NAME_NOT_FOUND(m_displayTypes, line);
            DSL_RETURN_IF_DISPLAY_TYPE_IS_NOT_CORRECT_TYPE(m_displayTypes, 
                line, RgbaLine);
            
            if (bboxTestPoint > DSL_BBOX_POINT_WEST)
            {
                LOG_ERROR("Bounding box test point value of '" << bboxTestPoint << 
                    "' is invalid when creating ODE Line Area '" << name << "'");
                return DSL_RESULT_ODE_AREA_PARAMETER_INVALID;
            }
            
            DSL_RGBA_LINE_PTR pLine = 
                std::dynamic_pointer_cast<RgbaLine>(m_displayTypes[line]);
            
            m_odeAreas[name] = DSL_ODE_AREA_LINE_NEW(name, 
                pLine, show, bboxTestPoint);
         
            LOG_INFO("New ODE Line Area '" << name 
                << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Line Area '" << name 
                << "' threw exception on creation");
            return DSL_RESULT_ODE_AREA_THREW_EXCEPTION;
        }
    }                
    
    DslReturnType Services::OdeAreaLineMultiNew(const char* name, 
        const char* multiLine, boolean show, uint bboxTestPoint)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            // ensure ODE Area name uniqueness 
            if (m_odeAreas.find(name) != m_odeAreas.end())
            {   
                LOG_ERROR("ODE Area name '" << name << "' is not unique");
                return DSL_RESULT_ODE_AREA_NAME_NOT_UNIQUE;
            }
            DSL_RETURN_IF_DISPLAY_TYPE_NAME_NOT_FOUND(m_displayTypes, multiLine);
            DSL_RETURN_IF_DISPLAY_TYPE_IS_NOT_CORRECT_TYPE(m_displayTypes, 
                multiLine, RgbaMultiLine);
            
            if (bboxTestPoint > DSL_BBOX_POINT_WEST)
            {
                LOG_ERROR("Bounding box test point value of '" << bboxTestPoint << 
                    "' is invalid when creating ODE Multi-Line Area '" << name << "'");
                return DSL_RESULT_ODE_AREA_PARAMETER_INVALID;
            }
            
            DSL_RGBA_MULTI_LINE_PTR pMultiLine = 
                std::dynamic_pointer_cast<RgbaMultiLine>(m_displayTypes[multiLine]);
            
            m_odeAreas[name] = 
                DSL_ODE_AREA_MULTI_LINE_NEW(name, pMultiLine, show, bboxTestPoint);
         
            LOG_INFO("New ODE Multi-Line Area '" << name 
                << "' created successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Multi-Line Area '" << name 
                << "' threw exception on creation");
            return DSL_RESULT_ODE_AREA_THREW_EXCEPTION;
        }
    }                
    
    DslReturnType Services::OdeAreaDelete(const char* name)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            DSL_RETURN_IF_ODE_ACTION_NAME_NOT_FOUND(m_odeAreas, name);
            
            if (m_odeAreas[name].use_count() > 1)
            {
                LOG_INFO("ODE Area'" << name << "' is in use");
                return DSL_RESULT_ODE_ACTION_IN_USE;
            }
            m_odeAreas.erase(name);

            LOG_INFO("ODE Area '" << name << "' deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Area '" << name << "' threw exception on deletion");
            return DSL_RESULT_ODE_AREA_THREW_EXCEPTION;
        }
    }
    
    DslReturnType Services::OdeAreaDeleteAll()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);

        try
        {
            if (m_odeAreas.empty())
            {
                return DSL_RESULT_SUCCESS;
            }
            for (auto const& imap: m_odeAreas)
            {
                // In the case of Delete all
                if (imap.second.use_count() > 1)
                {
                    LOG_ERROR("ODE Area '" << imap.second->GetName() 
                        << "' is currently in use");
                    return DSL_RESULT_ODE_AREA_IN_USE;
                }
            }
            m_odeAreas.clear();

            LOG_INFO("All ODE Areas deleted successfully");

            return DSL_RESULT_SUCCESS;
        }
        catch(...)
        {
            LOG_ERROR("ODE Area threw exception on delete all");
            return DSL_RESULT_ODE_AREA_THREW_EXCEPTION;
        }
    }

    uint Services::OdeAreaListSize()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_servicesMutex);
        
        return m_odeAreas.size();
    }
        
}    
