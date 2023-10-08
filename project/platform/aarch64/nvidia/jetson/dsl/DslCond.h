#ifndef DSL_DSL_COND_H
#define DSL_DSL_COND_H

#include <glib.h>

namespace DSL
{
    /**
     * @class DslCond
     * @brief Wrapper class for the GCond type
     */
    class DslCond
    {
    public:
    
        /**
         * @brief ctor for DslCond class
         */
        DslCond() 
        {
            g_cond_init(&m_cond);
        }
        
        /**
         * @brief dtor for DslMutex class
         */
        ~DslCond()
        {
            g_cond_clear(&m_cond);
        }
        
        /**
         * @brief & operator for the DslMutex class
         * @return returns the address of the wrapped mutex.
         */
        GCond* operator& ()
        {
            return &m_cond;
        }
        
    private:
        GCond m_cond; 
    };

} // namespace 

#endif // _DSL_COND_H
