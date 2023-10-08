#ifndef DSL_DSL_BINTR_H
#define DSL_DSL_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslNodetr.h"
#include "DslPadProbeHandler.h"

namespace DSL
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_BINTR_PTR std::shared_ptr<Bintr>
    #define DSL_BINTR_NEW(name) \
        std::shared_ptr<Bintr>(new Bintr(name))    

    /**
     * @class Bintr
     * @brief Implements a base container class for a GST Bin
     */
    class Bintr : public GstNodetr
    {
    public:

        /**
         * @brief named container ctor with new Bin 
         */
        Bintr(const char* name, bool isPipeline = false)
            : GstNodetr(name)
            , m_isPipeline(isPipeline)
            , m_requestPadId(-1)
            , m_isLinked(false)
            , m_batchSize(0)
            , m_gpuId(0)
            , m_nvbufMemType(DSL_NVBUF_MEM_TYPE_DEFAULT)
        { 
            LOG_FUNC(); 

            if (m_isPipeline)
            {
                m_pGstObj = GST_OBJECT(gst_pipeline_new(name));
            }
            else
            {
                m_pGstObj = GST_OBJECT(gst_bin_new(name));
            }
            if (!m_pGstObj)
            {
                LOG_ERROR("Failed to create a new GST bin for Bintr '" << name << "'");
                throw;  
            }
        }
        
        /**
         * @brief Bintr dtor to release all GST references
         */
        ~Bintr()
        {
            LOG_FUNC();
        }

        /**
         * @brief returns the current sink or src request pad-id -- as managed by the 
         * multi-component Parent Bintr -- for this bintr if used (i.e connected a 
         * streammuxer, demuxer, or splitter).
         * @return -1 when id is not assigned, i.e. bintr is not currently in use
         */
        int GetRequestPadId()
        {
            LOG_FUNC();
            
            return m_requestPadId;
        }
        
        /**
         * @brief Sets the the sink or src request pad-id -- as managed by the 
         * multi-component Parent Bintr -- for this bintr if used (i.e connected a 
         * streammuxer, demuxer, or splitter).
         * @param request pad-id value to assign. use -1 for unassigned. 
         */
        void SetRequestPadId(int id)
        {
            LOG_FUNC();

            m_requestPadId = id;
        }
        
        /**
         * @brief Allows a client to determined derived type from base pointer
         * @param[in] typeInfo to compare against
         * @return true if this Bintr is of typeInfo, false otherwise
         */
        bool IsType(const std::type_info& typeInfo)
        {
            LOG_FUNC();
            
            return (typeInfo.hash_code() == typeid(*this).hash_code());
        }

        /**
         * @brief Adds this Bintr as a child to a ParentBinter
         * @param[in] pParentBintr to add to
         */
        virtual bool AddToParent(DSL_BASE_PTR pParent)
        {
            LOG_FUNC();
                
            return (bool)pParent->AddChild(shared_from_this());
        }
        
        /**
         * @brief removes this Bintr from the provided pParentBintr
         * @param[in] pParentBintr Bintr to remove from
         */
        virtual bool RemoveFromParent(DSL_BASE_PTR pParentBintr)
        {
            LOG_FUNC();
                
            return pParentBintr->RemoveChild(shared_from_this());
        }

        /**
         * @brief virtual function for derived classes to implement
         * a bintr type specific function to link all children.
         */
        virtual bool LinkAll() = 0;
        
        /**
         * @brief virtual function for derived classes to implement
         * a bintr type specific function to unlink all child elements.
         */
        virtual void UnlinkAll() = 0;
        
        /**
         * @brief called to determine if a Bintr's Child Elementrs are Linked
         * @return true if Child Elementrs are currently Linked, false otherwise
         */
        bool IsLinked()
        {
            LOG_FUNC();
            
            return m_isLinked;
        }

        /**
         * @brief called to determine if a Bintr is currently in use - has a Parent
         * @return true if the Bintr has a Parent, false otherwise
         */
        bool IsInUse()
        {
            LOG_FUNC();
            
            return (bool)GetParentGstElement();
        }
        
        /**
         * @brief gets the current batchSize in use by this Bintr
         * @return the current batchSize
         */
        virtual uint GetBatchSize()
        {
            LOG_FUNC();
            
            return m_batchSize;
        };
        
        /**
         * @brief sets the batch size for this Bintr
         * @param the new batchSize to use
         */
        virtual bool SetBatchSize(uint batchSize)
        {
            LOG_FUNC();
            LOG_INFO("Setting batch size to '" << batchSize << "' for Bintr '" << GetName() << "'");
            
            m_batchSize = batchSize;
            return true;
        };

        /**
         * @brief sets the interval for this Bintr
         * @param the new interval to use
         */
        bool SetInterval(uint interval, uint timeout);

        /**
         * @brief Adds a Pad Probe Handler callback function to the Bintr
         * @param[in] pad pad to add the handler to; DSL_PAD_SINK | DSL_PAD SRC
         * @param[in] pPadProbeHandler shared pointer to the PPH to add
         * @return true if successful, false otherwise
         */
        bool AddPadProbeHandler(DSL_BASE_PTR pPadProbeHandler, uint pad)
        {
            LOG_FUNC();
            
            if (pPadProbeHandler->IsInUse())
            {
                LOG_ERROR("Can't add Pad Probe Handler = '" << pPadProbeHandler->GetName() 
                    << "' as it is currently in use");
                return false;
            }
            bool result(false);

            if (pad == DSL_PAD_SINK)
            {
                result = m_pSinkPadProbe->AddPadProbeHandler(pPadProbeHandler);
            }
            else if (pad == DSL_PAD_SRC)
            {
                result = m_pSrcPadProbe->AddPadProbeHandler(pPadProbeHandler);
            }
            else
            {
                LOG_ERROR("Invalid Pad type = " << pad << " for Bintr '" << GetName() << "'");
            }
            if (result)
            {
                pPadProbeHandler->AssignParentName(GetName());
            }
            return result;
        }
            
        /**
         * @brief Removes a Pad Probe Handler callback function from the Bintr
         * @param[in] pad pad to remove the handler from; DSL_PAD_SINK | DSL_PAD SRC
         * @return false if the Bintr does not have a Meta Batch Handler to remove for the give pad.
         */
        bool RemovePadProbeHandler(DSL_BASE_PTR pPadProbeHandler, uint pad)
        {
            LOG_FUNC();
            
            bool result(false);

            if (pad == DSL_PAD_SINK)
            {
                result = m_pSinkPadProbe->RemovePadProbeHandler(pPadProbeHandler);
            }
            else if (pad == DSL_PAD_SRC)
            {
                result = m_pSrcPadProbe->RemovePadProbeHandler(pPadProbeHandler);
            }
            else
            {
                LOG_ERROR("Invalid Pad type = " << pad << " for Bintr '" << GetName() << "'");
            }
            if (result)
            {
                pPadProbeHandler->ClearParentName();
            }
            return result;
        }
        
        
        /**
         * @brief Gets the current GPU ID used by this Bintr
         * @return the ID for the current GPU in use.
         */
        uint GetGpuId()
        {
            LOG_FUNC();

            LOG_DEBUG("Returning a GPU ID of " << m_gpuId <<"' for Bintr '" << GetName() << "'");
            return m_gpuId;
        }

        /**
         * @brief Bintr type specific implementation to set the GPU ID.
         * @return true if successfully set, false otherwise.
         */
        virtual bool SetGpuId(uint gpuId)
        {
            LOG_FUNC();
            
            if (IsLinked())
            {
                LOG_ERROR("Unable to set GPU ID for Bintr '" << GetName() 
                    << "' as it's currently linked");
                return false;
            }
            m_gpuId = gpuId;
            return true;
        }

        /**
         * @brief Gets the current NVIDIA buffer memory type used by this Bintr
         * @return one of the DSL_NVBUF_MEM_TYPE constant values.
         */
        uint GetNvbufMemType()
        {
            LOG_FUNC();

            LOG_DEBUG("Returning NVIDIA buffer memory type of " << m_nvbufMemType 
                <<"' for Bintr '" << GetName() << "'");
            return m_nvbufMemType;
        }

        /**
         * @brief Bintr type specific implementation to set the memory type.
         * @brief nvbufMemType new memory type to use
         * @return true if successfully set, false otherwise.
         */
        virtual bool SetNvbufMemType(uint nvbufMemType)
        {
            LOG_FUNC();
            
            if (IsInUse())
            {
                LOG_ERROR("Unable to set NVIDIA buffer memory type for Bintr '" << GetName() 
                    << "' as it's currently linked");
                return false;
            }
            m_nvbufMemType = nvbufMemType;
            return true;
        }

    protected:
    
        /**
         * @brief flag to specify if derived as Pipeline or other Bintr.
         */
        bool m_isPipeline;

        /**
         * @brief unique request pad id managed by the 
         * parent from the point of add until removed
         */
        int m_requestPadId;
    
        /**
         * @brief current is-linked state for this Bintr
         */
        bool m_isLinked;
        
        /**
         * @brief current batch size for this Bintr
         */
        uint m_batchSize;

        /**
         * @brief current GPU Id in used by this Bintr
         */
        uint m_gpuId;

        /**
         * @brief current Memory Type used by this Bintr
         */
        uint m_nvbufMemType;

        /**
         * @brief Sink PadProbetr for this Bintr
         */
        DSL_PAD_PROBE_PTR m_pSinkPadProbe;

        /**
         * @brief Source PadProbetr for this Bintr
         */
        DSL_PAD_PROBE_PTR m_pSrcPadProbe;
    };

} // DSL

#endif // _DSL_BINTR_H
