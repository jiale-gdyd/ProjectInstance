#ifndef DSL_DSL_LOG_H
#define DSL_DSL_LOG_H

inline std::string methodName(const std::string& prettyFunction)
{
    size_t colons = prettyFunction.find("::");
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = prettyFunction.rfind("(") - begin;

    return prettyFunction.substr(begin,end) + "()";
}

#define __METHOD_NAME__ methodName(__PRETTY_FUNCTION__)

#if defined(DSL_LOGGER_IMP)
    #include DSL_LOGGER_IMP
#else

    /**
     * Logs the Entry and Exit of a Function with the DEBUG level.
     * Add macro as the first statement to each function of interest.
     * Consider the intrussion/penalty of this call when adding.
    */
    #define LOG_FUNC()

    /**
    Logs a message with the DEBUG level.

    @param[in] message the message string to log.
    */
    #define LOG_DEBUG(message)

    /**
    Logs a message with the INFO level.

    @param[in] message the message string to log.
    */
    #define LOG_INFO(message)

    /**
    Logs a message with the WARN level.

    @param[in] message the message string to log.
    */
    #define LOG_WARN(message)

    /**
    Logs a message with the ERROR level.

    @param[in] message the message string to log.
    */
    #define LOG_ERROR(message)
    
#endif // !DSL_LOGGER_IMP

#endif // _DSL_LOG_H
