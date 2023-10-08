#ifndef DSL_DSL_OFV_BINTR_H
#define DSL_DSL_OFV_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslElementr.h"
#include "DslBintr.h"

namespace DSL
{
    
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_OFV_PTR std::shared_ptr<OfvBintr>
    #define DSL_OFV_NEW(name) \
        std::shared_ptr<OfvBintr>(new OfvBintr(name))

    /**
     * @class OfvBintr
     * @brief Implements an Optical Flow bin container
     */
    class OfvBintr : public Bintr
    {
    public: 
    
        /**
         * @brief ctor for the OfvBintr class
         * @param[in] name name to give the new OfvBintr
         */
        OfvBintr(const char* name);

        /**
         * @brief dtor for the OfvBintr class
         */
        ~OfvBintr();

        /**
         * @brief Adds this OfvBintr to a Parent Pipline Bintr
         * @param[in] pParentBintr
         */
        bool AddToParent(DSL_BASE_PTR pParentBintr);
        
        /**
         * @brief Links all child elements of this OfvBintr
         * @return true if all elements were succesfully linked, false otherwise.
         */
        bool LinkAll();
        
        /**
         * @brief Unlinks all child elements of the OfvBintr
         */
        void UnlinkAll();

    private:
        
        /**
         @brief
         */
        
        DSL_ELEMENT_PTR m_pOptFlowQueue;
        DSL_ELEMENT_PTR m_pOptFlow;
        DSL_ELEMENT_PTR m_pOptFlowVisualQueue;
        DSL_ELEMENT_PTR m_pOptFlowVisual;
    };
}

#endif // _DSL_OFV_BINTR_H
