#ifndef DSL_DSL_ODE_BASE_H
#define DSL_DSL_ODE_BASE_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBase.h"

namespace DSL
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_ODE_BASE_PTR std::shared_ptr<OdeBase>
    
    // ********************************************************************

    class OdeBase : public Base
    {
    public: 
    
        /**
         * @brief ctor for the ODE base class
         * @param[in] name unique name for the ODE Action
         */
        OdeBase(const char* name)
            : Base(name)
            , m_enabled(true)
        {
            LOG_FUNC();
        };

        /**
         * @brief ctor for the ODE base class
         */
        ~OdeBase()
        {
            LOG_FUNC();
        };

        /**
         * @brief Gets the current Enabled setting, default = true
         * @return the current Enabled setting
         */
        bool GetEnabled()
        {
            LOG_FUNC();
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_propertyMutex);
            
            return m_enabled;
        };
        
        /**
         * @brief Sets the Enabled setting for ODE Action
         * @param[in] the new value to use
         */
        void SetEnabled(bool enabled)
        {
            LOG_FUNC();
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_propertyMutex);
            
            m_enabled = enabled;
            
            // iterate through the map of limit-state-change-listeners calling each
            for(auto const& imap: m_enabledStateChangeListeners)
            {
                try
                {
                    imap.first(m_enabled, imap.second);
                }
                catch(...)
                {
                    LOG_ERROR("Exception calling Client Limit-State-Change-Lister");
                }
            }
        };
        
        /**
         * @brief Adds a "limit state change listener" function to be notified
         * on Trigger limit reached and count reset.
         * @return ture if the listener function was successfully added, false otherwise.
         */
        bool AddEnabledStateChangeListener(
            dsl_ode_enabled_state_change_listener_cb listener, void* clientData)
        {
            LOG_FUNC();
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_propertyMutex);

            if (m_enabledStateChangeListeners.find(listener) != 
                m_enabledStateChangeListeners.end())
            {   
                LOG_ERROR("Limit state change listener is not unique");
                return false;
            }
            m_enabledStateChangeListeners[listener] = clientData;

            return true;
        };

        /**
         * @brief Removes a "limit state change listener" function previously added
         * with a call to AddLimitStateChangeListener.
         * @return ture if the listener function was successfully removed, false otherwise.
         */
        bool RemoveEnabledStateChangeListener(
            dsl_ode_enabled_state_change_listener_cb listener)
        {
            LOG_FUNC();
            LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_propertyMutex);

            if (m_enabledStateChangeListeners.find(listener) == 
                m_enabledStateChangeListeners.end())
            {   
                LOG_ERROR("Limit state change listener was not found");
                return false;
            }
            m_enabledStateChangeListeners.erase(listener);

            return true;
        };
        
    protected:

        /**
         * @brief Mutex to ensure mutual exlusion for propery get/sets
         */
        DslMutex m_propertyMutex;
    
        /**
         * @brief enabled flag.
         */
        bool m_enabled;

    private:
    
        /**
         * @brief map of all currently registered enabled-state-change-listeners
         * callback functions mapped with the user provided data
         */
        std::map<dsl_ode_enabled_state_change_listener_cb, 
            void*>m_enabledStateChangeListeners;
        
    };            

}

#endif // _DSL_ODE_ACTION_H
