#ifndef DSL_DSL_LOGGST_H
#define DSL_DSL_LOGGST_H

GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DSL);

#include "Dsl.h"

namespace DSL
{

/**
 * Logs the Entry and Exit of a Function with the DEBUG level.
 * Add macro as the first statement to each function of interest.
 * Consider the intrussion/penalty of this call when adding.
 */
#define LOG_FUNC() LogFunc lf(__METHOD_NAME__)

#define LOG(message, level) \
    do \
    { \
        std::stringstream logMessage; \
        logMessage  << " : " << message; \
        GST_CAT_LEVEL_LOG(GST_CAT_DSL, level, NULL, logMessage.str().c_str(), NULL); \
    } while (0)

#define LOG_DEBUG(message) LOG(message, GST_LEVEL_DEBUG)

#define LOG_INFO(message) LOG(message, GST_LEVEL_INFO)

#define LOG_WARN(message) LOG(message, GST_LEVEL_WARNING)

#define LOG_ERROR(message) LOG(message, GST_LEVEL_ERROR)
 
    /**
     * @class LogFunc
     * @brief Used to log entry and exit of a function.
     */
    class LogFunc
    {
    public:
        LogFunc(const std::string& method) 
        {
            m_logMessage  << method;
            GST_CAT_LEVEL_LOG(GST_CAT_DSL, GST_LEVEL_DEBUG, NULL, 
                m_logMessage.str().c_str(), "");
        };
        
        ~LogFunc()
        {
            GST_CAT_LEVEL_LOG(GST_CAT_DSL, GST_LEVEL_DEBUG, NULL, 
                m_logMessage.str().c_str(), "");
        };
        
    private:
        std::stringstream m_logMessage; 
    };

} // namespace 


#endif // _DSL_LOGGST_H
