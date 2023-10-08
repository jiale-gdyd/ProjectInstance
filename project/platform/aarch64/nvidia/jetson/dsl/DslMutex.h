#ifndef DSL_DSL_MUTEX_H
#define DSL_DSL_MUTEX_H

#include "glib.h"
 
namespace DSL
{
    /**
     * @class DslMutex
     * @brief Wrapper class for the GMutex type
     */
    class DslMutex
    {
    public:
    
        /**
         * @brief ctor for DslMutex class
         */
        DslMutex() 
        {
            g_mutex_init(&m_mutex);
        }
        
        /**
         * @brief dtor for DslMutex class
         */
        ~DslMutex()
        {
            g_mutex_clear(&m_mutex);
        }
        
        /**
         * @brief & operator for the DslMutex class
         * @return returns the address of the wrapped mutex.
         */
        GMutex* operator& ()
        {
            return &m_mutex;
        }
        
    private:
        GMutex m_mutex; 
    };
   
    #define LOCK_MUTEX_FOR_CURRENT_SCOPE(mutex) LockMutexForCurrentScope lock(mutex)
    #define LOCK_2ND_MUTEX_FOR_CURRENT_SCOPE(mutex) LockMutexForCurrentScope lock2(mutex)

    /**
     * @class LockMutexForCurrentScope
     * @brief Locks a GMutex for the current scope {}.
     */
    class LockMutexForCurrentScope
    {
    public:
        LockMutexForCurrentScope(GMutex* mutex) : m_pMutex(mutex) 
        {
            g_mutex_lock(m_pMutex);
        }
        
        ~LockMutexForCurrentScope()
        {
            g_mutex_unlock(m_pMutex);
        }
        
    private:
        GMutex* m_pMutex; 
    };

    #define UNREF_MESSAGE_ON_RETURN(message) UnrefMessageOnReturn ref(message)

    /**
     * @class UnrefMessageOnReturn
     * @brief Unreferences a GstMessage on return from current function scope.
     * i.e. the point at which an object of this class is destroyed
     */
    class UnrefMessageOnReturn
    {
    public:
        UnrefMessageOnReturn(GstMessage* message) : m_pMessage(message){};
        
        ~UnrefMessageOnReturn()
        {
            gst_message_unref(m_pMessage);
        };
        
    private:
        GstMessage* m_pMessage; 
    };

} // namespace 

#endif // _DSL_MUTEX_H
