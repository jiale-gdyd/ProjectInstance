#ifndef DSL_DSL_ODE_ACCUMULATOR_H
#define DSL_DSL_ODE_ACCUMULATOR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslOdeBase.h"

namespace DSL
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_ODE_ACCUMULATOR_PTR std::shared_ptr<OdeAccumulator>
    #define DSL_ODE_ACCUMULATOR_NEW(name) \
        std::shared_ptr<OdeAccumulator>(new OdeAccumulator(name))

    // *****************************************************************************

    /**
     * @class OdeTrigger
     * @brief Implements a super/abstract class for all ODE Triggers
     */
    class OdeAccumulator : public OdeBase
    {
    public: 
    
        OdeAccumulator(const char* name);

        ~OdeAccumulator();

        /**
         * @brief Handles the ODE occurrence by calling the client handler
         * @param[in] pBuffer pointer to the batched stream buffer that triggered the event
         * @param[in] pOdeTrigger shared pointer to ODE Trigger that triggered the event
         * @param[in] pFrameMeta pointer to the Frame Meta data that triggered the event
         * @param[in] pObjectMeta pointer to Object Meta if Object detection event, 
         * NULL if Frame level absence, total, min, max, etc. events.
         */
        void HandleOccurrences(DSL_BASE_PTR pOdeTrigger, 
            GstBuffer* pBuffer, std::vector<NvDsDisplayMeta*>& displayMetaData,
            NvDsFrameMeta* pFrameMeta);
        
        /**
         * @brief Adds an ODE Action as a child to this OdeAccumulator
         * @param[in] pChild pointer to ODE Action to add
         * @return true if successful, false otherwise
         */
        bool AddAction(DSL_BASE_PTR pChild);
        
        /**
         * @brief Removes a child ODE Action from this OdeAccumulator
         * @param[in] pChild pointer to ODE Action to remove
         * @return true if successful, false otherwise
         */
        bool RemoveAction(DSL_BASE_PTR pChild);
        
        /**
         * @brief Removes all child ODE Actions from this OdeAccumulator
         */
        void RemoveAllActions(); 

    private:
    
        /**
         * @brief Index variable to incremment/assign on ODE Action add.
         */
        uint m_nextActionIndex;

        /**
         * @brief Map of child ODE Actions owned by this OdeAccumulator
         */
        std::map <std::string, DSL_BASE_PTR> m_pOdeActions;
        
        /**
         * @brief Map of child ODE Actions indexed by their add-order for execution
         */
        std::map <uint, DSL_BASE_PTR> m_pOdeActionsIndexed;

    };

}

#endif // _DSL_ODE_ACCUMULATOR_H
