#ifndef DSL_DSL_SEG_VISUAL_ELEMENTR_H
#define DSL_DSL_SEG_VISUAL_ELEMENTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslElementr.h"
#include "DslBintr.h"

namespace DSL
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
        
    #define DSL_SEGVISUAL_PTR std::shared_ptr<SegVisualBintr>
    #define DSL_SEGVISUAL_NEW(name, width, height) \
        std::shared_ptr<SegVisualBintr>(new SegVisualBintr(name, width, height))
        
    class SegVisualBintr : public Bintr
    {
    public: 
    
        SegVisualBintr(const char* name, uint width, uint height);

        ~SegVisualBintr();

        /**
         * @brief Adds the SegVisualBintr to a Parent Bintr
         * @param[in] pParentBintr Parent to add this Bintr to
         */
        bool AddToParent(DSL_BASE_PTR pParentBintr);

        /**
         * @brief Links all Child Elementrs owned by this Bintr
         * @return true if all links were succesful, false otherwise
         */
        bool LinkAll();
        
        /**
         * @brief Unlinks all Child Elemntrs owned by this Bintr
         * Calling UnlinkAll when in an unlinked state has no effect.
         */
        void UnlinkAll();
        
        /**
         * @brief Gets the current output width and height settings for this SegVisualBintr
         * @param[out] width the current width setting in pixels
         * @param[out] height the current height setting in pixels
         */ 
        void GetDimensions(uint* width, uint* height);
        
        /**
         * @brief Sets the current output width and height settings for this SegVisualBintr
         * The caller is required to provide valid width and height values
         * @param[in] width the width value to set in pixels
         * @param[in] height the height value to set in pixels
         * @return True on successful update, false otherwise
         */ 
        bool SetDimensions(uint width, uint hieght);

        /**
         * @brief sets the batch size for this SegVisualBintr
         * @param the new batchSize to use
         */
        bool SetBatchSize(uint batchSize);

        /**
         * @brief Sets the GPU ID for this SegVisualBintr
         * @param[in] gpuId new GPU ID setting.
         * @return true if successfully set, false otherwise.
         */
        bool SetGpuId(uint gpuId);

    protected:

        /**
         * @brief output frame width of the input buffer in pixels
         */
        uint m_width; 
        
        /**
         * @brief output frame height of the input buffer in pixels
         */
        uint m_height;

        /**
         * @brief Queue Elementr as Sink for this SegVisualBintr
         */
        DSL_ELEMENT_PTR  m_pQueue;

        /**
         * @brief SegVisual Elementr as Source for SegVisualBintr 
         */
        DSL_ELEMENT_PTR  m_pSegVisual;
    };

} // DSL

#endif // _DSL_SEG_VISUAL_ELEMENTR_H
