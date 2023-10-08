#ifndef DSL_DSL_NODETR_H
#define DSL_DSL_NODETR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBase.h"

namespace DSL
{

    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_NODETR_PTR std::shared_ptr<Nodetr>
    #define DSL_NODETR_NEW(name) \
        std::shared_ptr<Nodetr>(new Nodetr(name))    

    #define DSL_GSTNODETR_PTR std::shared_ptr<GstNodetr>
    #define DSL_GSTNODETR_NEW(name) \
        std::shared_ptr<GstNodetr>(new GstNodetr(name))    

    /**
     * @class Nodetr
     * @brief Implements a container class for all DSL Tree Node types
     */
    class Nodetr : public Base
    {
    public:
        
        /**
         * @brief ctor for the Nodetr base class
         * @param[in] name for the new Nodetr
         */
        Nodetr(const char* name)
            : Base(name)
            , m_pGstObj(NULL)
            , m_pParentGstObj(NULL)
        {
            LOG_FUNC();

            LOG_DEBUG("New Nodetr '" << m_name << "' created");
        }

        /**
         * @brief dtor for the Nodetr base class
         */
        ~Nodetr()
        {
            LOG_FUNC();
            
            LOG_DEBUG("Nodetr '" << m_name << "' deleted");
        }
        
        /**
         * @brief Links this Noder, becoming a source, to a sink Nodre
         * @param[in] pSink Sink Nodre to link this Source Nodre to
         * @return True if this source Nodetr is successfully linked 
         * to the provided sink Nodetr
         */
        virtual bool LinkToSink(DSL_NODETR_PTR pSink)
        {
            LOG_FUNC();

            if (m_pSink)
            {
                LOG_ERROR("Nodetr '" << GetName() 
                    << "' is currently in a linked to Sink");
                return false;
            }
            m_pSink = pSink;
            LOG_DEBUG("Source '" << GetName() 
                << "' linked to Sink '" << pSink->GetName() << "'");
            
            return true;
        }
        
        /**
         * @brief Unlinks this Source Nodetr from its Sink Nodetr
         * @return True if this source Nodetr is successfully unlinked 
         * the sink Nodetr
         */
        virtual bool UnlinkFromSink()
        {
            LOG_FUNC();

            if (!m_pSink)
            {
                LOG_ERROR("Nodetr '" << GetName() 
                    << "' is not currently linked to Sink");
                return false;
            }
            LOG_DEBUG("Unlinking Source '" << GetName() 
                << "' from Sink '" << m_pSink->GetName() << "'");
            m_pSink = nullptr; 

            return true;
        }
        
        /**
         * @brief Links this Noder, becoming a source, to a sink Nodre
         * @param[in] pSrc Nodre to link this Sink Nodre back to
         */
        virtual bool LinkToSource(DSL_NODETR_PTR pSrc)
        {
            LOG_FUNC();

            if (m_pSrc)
            {
                LOG_ERROR("Nodetr '" << GetName() 
                    << "' is currently in a linked to a Source");
                return false;
            }
            m_pSrc = pSrc;
            LOG_DEBUG("Source '" << pSrc->GetName() 
                << "' linked to Sink '" << GetName() << "'");
            
            return true;
        }

        /**
         * @brief returns the currently linked to Sink Nodetr
         * @return shared pointer to Sink Nodetr, nullptr if Unlinked to sink
         */
        DSL_NODETR_PTR GetSource()
        {
            LOG_FUNC();
            
            return m_pSrc;
        }

        /**
         * @brief Unlinks this Sink Nodetr from its Source Nodetr
         */
        virtual bool UnlinkFromSource()
        {
            LOG_FUNC();

            if (!m_pSrc)
            {
                LOG_ERROR("Nodetr '" << GetName() 
                    << "' is not currently linked to Source");
                return false;
            }
            LOG_DEBUG("Unlinking self '" << GetName() 
                << "' as a Sink from '" << m_pSrc->GetName() << "' Source");
            m_pSrc = nullptr;
            
            return true;
        }
        
        /**
         * @brief returns the Source-to-Sink linked state for this Nodetr
         * @return true if this Nodetr is linked to a Sink Nodetr
         */
        bool IsLinkedToSink()
        {
            LOG_FUNC();
            
            return bool(m_pSink);
        }
        
        /**
         * @brief returns the Sink-to-Source linked state for this Nodetr
         * @return true if this Nodetr is linked to a Source Nodetr
         */
        bool IsLinkedToSource()
        {
            LOG_FUNC();
            
            return bool(m_pSrc);
        }
        
        /**
         * @brief returns the Sink Nodetr that this Nodetr is.urrently linked to
         * @return shared pointer to Sink Nodetr, nullptr if Unlinked from Sink
         */
        DSL_NODETR_PTR GetSink()
        {
            LOG_FUNC();
            
            return m_pSink;
        }
        
        /**
         * @brief returns this Nodetr's GStreamer bin as a GST_OBJECT
         * @return this Nodetr's bin cast to GST_OBJECT
         */
        GstObject* GetGstObject()
        {
            LOG_FUNC();
            
            return GST_OBJECT(m_pGstObj);
        }
        
        /**
         * @brief returns this Nodetr's GStreamer bin as a GST_ELEMENT
         * @return this Nodetr's bin cast to GST_ELEMENT
         */
        GstElement* GetGstElement()
        {
            LOG_FUNC();
            
            return GST_ELEMENT(m_pGstObj);
        }
        
        /**
         * @brief returns this Nodetr's GStreamer bin as a G_OBJECT
         * @return this Nodetr's bin cast to G_OBJECT
         */
        GObject* GetGObject()
        {
            LOG_FUNC();
            
            return G_OBJECT(m_pGstObj);
        }

        /**
         * @brief returns this Nodetr's Parent's GStreamer bin as a GST_OBJECT
         * @return this Nodetr's Parent's bin cast to GST_OBJECT
         */
        GstObject* GetParentGstObject()
        {
            LOG_FUNC();
            
            return GST_OBJECT(m_pParentGstObj);
        }

        /**
         * @brief returns this Nodetr's Parent's GStreamer bin as a GST_OBJECT
         * @return this Nodetr's Parent's bin cast to GST_OBJECT
         */
        GstElement* GetParentGstElement()
        {
            LOG_FUNC();
            
            return GST_ELEMENT(m_pParentGstObj);
        }
        
        /**
         * @brief Sets this Nodetr's GStreamer bin to a new GST_OBJECT
         */
        void SetGstObject(GstObject* pGstObj)
        {
            LOG_FUNC();
            
            m_pGstObj = pGstObj;
        }

    public:
    
        /**
         * @brief Parent of this Nodetr if one exists. NULL otherwise
         */
        GstObject* m_pParentGstObj;
        
        
    protected:

        /**
         * @brief Gst object wrapped by the Nodetr
         */
        GstObject* m_pGstObj;

        /**
         * @brief defines the relationship between a Source Nodetr
         * linked to this Nodetr, making this Nodetr a Sink
         */
        DSL_NODETR_PTR m_pSrc;

        /**
         * @brief defines the relationship this Nodetr linked to
         * a Sink Nodetr making this Nodetr a Source
         */
        DSL_NODETR_PTR m_pSink;
    };

    static GstPadProbeReturn complete_unlink_from_source_tee_cb(GstPad* pad, 
        GstPadProbeInfo *info, gpointer pNoder);

   /**
     * @class GstNodetr
     * @brief Overrides the Base Class Virtual functions, adding the actuall 
     * GstObject* management. This allows the Nodetr class, and all its relational 
     * behavior, to be tested independent from GStreamer. Each method of this class
     * calls the base class to complete its behavior.
     */
    class GstNodetr : public Nodetr
    {
    public:
        
        /**
         * @brief ctor for the GstNodetr base class
         * @param[in] name for the new GstNodetr
         */
        GstNodetr(const char* name)
            : Nodetr(name)
            , m_releaseRequestedPadOnUnlink(false)
        {
            LOG_FUNC();

            LOG_DEBUG("New GstNodetr '" << GetName() << "' created");
        }

        /**
         * @brief dtor for the GstNodetr base class
         */
        ~GstNodetr()
        {
            LOG_FUNC();
        
            if (!GetGstElement())
            {
                LOG_WARN("GstElement for GstNodetr '" << GetName() 
                    << "' has not been instantiated");
            }
            else
            {
                // Set the State to NULL to free up all resource before 
                // removing childern
                LOG_DEBUG("Setting GstElement for GstNodetr '" 
                    << GetName() << "' to GST_STATE_NULL");
                gst_element_set_state(GetGstElement(), GST_STATE_NULL);

                // Remove all child references 
                RemoveAllChildren();
                
                if (!m_pParentGstObj)
                {
                    LOG_DEBUG("Unreferencing GST Object contained by this GstNodetr '" 
                        << GetName() << "'");
                    gst_object_unref(m_pGstObj);
                }
            }
            LOG_DEBUG("Nodetr '" << GetName() << "' deleted");
        }

        /**
         * @brief adds a child GstNodetr to this parent Bintr
         * @param[in] pChild to add. Once added, calling InUse()
         *  on the Child Bintr will return true
         * @return true if pChild was added successfully, false otherwise
         */
        virtual bool AddChild(DSL_BASE_PTR pChild)
        {
            LOG_FUNC();
            
            DSL_NODETR_PTR pChildNodetr = std::dynamic_pointer_cast<Nodetr>(pChild);

            if (!gst_bin_add(GST_BIN(m_pGstObj), pChildNodetr->GetGstElement()))
            {
                LOG_ERROR("Failed to add " << pChildNodetr->GetName() 
                    << " to " << GetName() <<"'");
                throw;
            }
            pChildNodetr->m_pParentGstObj = m_pGstObj;
            return Nodetr::AddChild(pChild);
        }
        
        /**
         * @brief removes a child from this parent GstNodetr
         * @param[in] pChild to remove. Once removed, calling InUse()
         *  on the Child Bintr will return false
         */
        virtual bool RemoveChild(DSL_BASE_PTR pChild)
        {
            LOG_FUNC();
            
            if (!IsChild(pChild))
            {
                LOG_ERROR("'" << pChild->GetName() 
                    << "' is not a child of '" << GetName() <<"'");
                return false;
            }

            DSL_NODETR_PTR pChildNodetr = std::dynamic_pointer_cast<Nodetr>(pChild);
            
            // Increase the reference count so the child is not destroyed.
            gst_object_ref(pChildNodetr->GetGstElement());
            
            if (!gst_bin_remove(GST_BIN(m_pGstObj), pChildNodetr->GetGstElement()))
            {
                LOG_ERROR("Failed to remove " << pChildNodetr->GetName() 
                    << " from " << GetName() <<"'");
                return false;
            }
            pChildNodetr->m_pParentGstObj = NULL;
            return Nodetr::RemoveChild(pChildNodetr);
        }

        /**
         * @brief removed a child Nodetr of this parent Nodetr
         * @param[in] pChild to remove
         */
        void RemoveAllChildren()
        {
            LOG_FUNC();

            for (auto &imap: m_pChildren)
            {
                LOG_DEBUG("Removing Child GstNodetr'" << imap.second->GetName() 
                    << "' from Parent GST BIn'" << GetName() <<"'");
                
                DSL_NODETR_PTR pChildNodetr = 
                    std::dynamic_pointer_cast<Nodetr>(imap.second);

                // Increase the reference count so the child is not destroyed.
                gst_object_ref(pChildNodetr->GetGstElement());

                if (!gst_bin_remove(GST_BIN(m_pGstObj), 
                    pChildNodetr->GetGstElement()))
                {
                    LOG_ERROR("Failed to remove GstNodetr " 
                        << pChildNodetr->GetName() << " from " << GetName() <<"'");
                }
                pChildNodetr->m_pParentGstObj = NULL;
            }
            Nodetr::RemoveAllChildren();
        }
        
        /**
         * @brief Creates a new Ghost Sink pad for this Gst Element
         * @param[in] padname which pad to add to, either "sink" or "src"
         * @throws a general exception on failure
         */
        void AddGhostPadToParent(const char* padname)
        {
            LOG_FUNC();

            // create a new ghost pad with the static Sink pad retrieved from 
            // this Elementr's pGstObj and adds it to the the Elementr's Parent 
            // Bintr's pGstObj.
            if (!gst_element_add_pad(GST_ELEMENT(GetParentGstObject()), 
                gst_ghost_pad_new(padname, 
                    gst_element_get_static_pad(GetGstElement(), padname))))
            {
                LOG_ERROR("Failed to add Pad '" << padname 
                    << "' for element'" << GetName() << "'");
                throw;
            }
        }

        /**
         * @brief links this Elementr as Source to a given Sink Elementr
         * @param[in] pSink to link to
         */
        bool LinkToSink(DSL_NODETR_PTR pSink)
        { 
            LOG_FUNC();
            
            // Call the base class to setup the relationship first
            // Then call GST to Link Source Element to Sink Element 
            if (!Nodetr::LinkToSink(pSink) or 
                !gst_element_link(GetGstElement(), m_pSink->GetGstElement()))
            {
                LOG_ERROR("Failed to link " << GetName() 
                    << " to " << pSink->GetName());
                return false;
            }
            return true;
        }

        /**
         * @brief unlinks this Nodetr from a previously linked-to Sink Notetr
         */
        bool UnlinkFromSink()
        { 
            LOG_FUNC();

            // Need to check here first, as we're calling the parent class 
            // last when unlinking
            if (!IsLinkedToSink())
            {
                LOG_ERROR("GstNodetr '" << GetName() 
                    << "' is not in a linked state");
                return false;
            }
            if (!GetGstElement() or !m_pSink->GetGstElement())
            {
                LOG_ERROR("Invalid GstElements for  '" << GetName());
                return false;
            }
            gst_element_unlink(GetGstElement(), m_pSink->GetGstElement());
            return Nodetr::UnlinkFromSink();
        }

        /**
         * @brief links this Elementr as Sink to a given Source Nodetr
         * @param[in] pSinkBintr to link to
         */
        bool LinkToSource(DSL_NODETR_PTR pSrc)
        { 
            LOG_FUNC();
            
            DSL_NODETR_PTR pSrcNodetr = std::dynamic_pointer_cast<Nodetr>(pSrc);

            // Call the base class to setup the relationship first
            // Then call GST to Link Source Element to Sink Element 
            if (!Nodetr::LinkToSource(pSrcNodetr) or 
                !gst_element_link(pSrcNodetr->GetGstElement(), GetGstElement()))
            {
                LOG_ERROR("Failed to link Source '" << pSrcNodetr->GetName() 
                    << " to Sink" << GetName());
                return false;
            }
            return true;
        }

        /**
         * @brief returns the currently linked to Sink Nodetr
         * @return shared pointer to Sink Nodetr, nullptr if Unlinked to sink
         */
        DSL_GSTNODETR_PTR GetGstSource()
        {
            LOG_FUNC();
            
            return std::dynamic_pointer_cast<GstNodetr>(m_pSrc);
        }

        /**
         * @brief unlinks this Elementr from a previously linked-to Source Element
         */
        bool UnlinkFromSource()
        { 
            LOG_FUNC();

            // Need to check here first, as we're calling the parent class 
            // last when unlinking
            if (!IsLinkedToSource())
            {
                LOG_ERROR("GstNodetr '" << GetName() << "' is not in a linked state");
                return false;
            }
            if (!m_pSrc->GetGstElement() or !GetGstElement())
            {
                LOG_ERROR("Invalid GstElements for  '" << m_pSrc->GetName() 
                    << "' and '" << GetName() << "'");
                return false;
            }
            gst_element_unlink(m_pSrc->GetGstElement(), GetGstElement());

            return Nodetr::UnlinkFromSource();
        }

        /**
         * @brief links this Noder to the Sink Pad of Muxer
         * @param[in] pMuxer nodetr to link to
         * @param[in] padName name to give the requested Sink Pad
         * @return true if able to successfully link with Muxer Sink Pad
         */
        bool LinkToSinkMuxer(DSL_NODETR_PTR pMuxer, const char* padName)
        {
            LOG_FUNC();
            
            // Get a reference to this GstNodetr's source pad
            GstPad* pStaticSrcPad = gst_element_get_static_pad(GetGstElement(), 
                "src");
            if (!pStaticSrcPad)
            {
                LOG_ERROR("Failed to get Static Src Pad for GstNodetr '" 
                    << GetName() << "'");
                return false;
            }

            // Request a new sink pad from the Muxer to connect to this 
            // GstNodetr's source pad
            GstPad* pRequestedSinkPad = gst_element_get_request_pad(
                pMuxer->GetGstElement(), padName);
            if (!pRequestedSinkPad)
            {
                LOG_ERROR("Failed to get requested Tee Sink Pad for GstNodetr '" 
                    << GetName() <<"'");
                return false;
            }
            
            LOG_INFO("Linking requested Sink Pad'" << pRequestedSinkPad 
                << "' for GstNodetr '" << GetName() << "'");
                
            if (gst_pad_link(pStaticSrcPad, 
                pRequestedSinkPad) != GST_PAD_LINK_OK)
            {
                LOG_ERROR("GstNodetr '" << GetName() 
                    << "' failed to link to Muxer");
                return false;
            }
            
            // unreference both the static source pad and requested sink
            gst_object_unref(pStaticSrcPad);
            gst_object_unref(pRequestedSinkPad);

            // call the parent class to complete the link-to-sink
            return Nodetr::LinkToSink(pMuxer);
        }
        
        /**
         * @brief unlinks this Nodetr from a previously linked Muxer Sink Pad
         * @return true if able to successfully unlink from Muxer Sink Pad
         */
        bool UnlinkFromSinkMuxer()
        {
            LOG_FUNC();
            
            // need to initialize outside of case statement
            GstPad* pStaticSrcPad(NULL);
            GstPad* pRequestedSinkPad(NULL);
            
            if (!IsLinkedToSink())
            {
                return false;
            }

            
            GstStateChangeReturn changeResult = gst_element_set_state(
                GetGstElement(), GST_STATE_NULL);
                
            switch (changeResult)
            {
            case GST_STATE_CHANGE_FAILURE:
                LOG_ERROR("GstNodetr '" << GetName() 
                    << "' failed to set state to NULL");
                return false;

            case GST_STATE_CHANGE_ASYNC:
                LOG_INFO("GstNodetr '" << GetName() 
                    << "' changing state to NULL async");
                    
                // block on get state until change completes. 
                if (gst_element_get_state(GetGstElement(), 
                    NULL, NULL, GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE)
                {
                    LOG_ERROR("GstNodetr '" << GetName() 
                        << "' failed to set state to NULL");
                    return false;
                }
                // drop through on success - DO NOT BREAK

            case GST_STATE_CHANGE_SUCCESS:
                LOG_INFO("GstNodetr '" << GetName() 
                    << "' changed state to NULL successfully");
                    
                // Get a reference to this GstNodetr's source pad
                pStaticSrcPad = gst_element_get_static_pad(GetGstElement(), "src");
                if (!pStaticSrcPad)
                {
                    LOG_ERROR("Failed to get static source pad for GstNodetr '" 
                        << GetName() << "'");
                    return false;
                }
                
                // Get a reference to the Muxer's sink pad that is connected
                // to this GstNodetr's source pad
                pRequestedSinkPad = gst_pad_get_peer(pStaticSrcPad);
                if (!pRequestedSinkPad)
                {
                    LOG_ERROR("Failed to get requested sink pad peer for GstNodetr '" 
                        << GetName() << "'");
                    return false;
                }

                // Send a flush-stop event upstream to all elements and
                // downstream to the muxer for this GstNodetr's stream
                gst_pad_send_event(pStaticSrcPad, 
                    gst_event_new_flush_stop(FALSE));
                gst_pad_send_event(pRequestedSinkPad, 
                    gst_event_new_flush_stop(FALSE));
                gst_pad_send_event(pRequestedSinkPad, 
                    gst_event_new_eos());

                LOG_INFO("Unlinking and releasing requested sink pad '" 
                    << pRequestedSinkPad << "' for GstNodetr '" << GetName() << "'");

                // It should now be safe to unlink this GstNodetr from the Muxer
                if (!gst_pad_unlink(pStaticSrcPad, pRequestedSinkPad))
                {
                    LOG_ERROR("GstNodetr '" << GetName() 
                        << "' failed to unlink from Muxer");
                    Nodetr::UnlinkFromSink();
                    return false;
                }
                // Need to release the previously requested sink pad
                gst_element_release_request_pad(GetSink()->GetGstElement(), 
                    pRequestedSinkPad);

                // unreference both the static source pad and requested sink
                gst_object_unref(pStaticSrcPad);
                gst_object_unref(pRequestedSinkPad);
                
                // Call the parent class to complete the unlink from sink
                return Nodetr::UnlinkFromSink();
            default:
                break;
            }
            LOG_ERROR("Unknown state change for Bintr '" << GetName() << "'");
            return false;

        }
        
        /**
         * @brief links this Nodetr as a Sink to the Source Pad of a Splitter Tee
         * @param[in] pTee to link to
         * @param[in] padName name to give the requested Src Pad
         * @return true if able to successfully link with Tee Src Pad
         */
        virtual bool LinkToSourceTee(DSL_NODETR_PTR pTee, const char* padName)
        {
            LOG_FUNC();
            
            // Get a reference to the static sink pad for this GstNodetr
            GstPad* pStaticSinkPad = gst_element_get_static_pad(
                GetGstElement(), "sink");
            if (!pStaticSinkPad)
            {
                LOG_ERROR("Failed to get static sink pad for GstNodetr '" 
                    << GetName() << "'");
                return false;
            }

            // Request a new source pad from the Tee 
            GstPad* pRequestedSrcPad = gst_element_get_request_pad(
                pTee->GetGstElement(), padName);
            if (!pRequestedSrcPad)
            {
                LOG_ERROR("Failed to get a requested source pad for Tee '" 
                    << pTee->GetName() <<"'");
                return false;
            }
            m_releaseRequestedPadOnUnlink = true;

            LOG_INFO("Linking requested source pad'" << pRequestedSrcPad 
                << "' for GstNodetr '" << GetName() << "'");

            if (gst_pad_link(pRequestedSrcPad, pStaticSinkPad) != GST_PAD_LINK_OK)
            {
                LOG_ERROR("GstNodetr '" << GetName() 
                    << "' failed to link to Source Tee");
                return false;
            }

            // Unreference both the static sink pad and requested source pad
            gst_object_unref(pStaticSinkPad);
            gst_object_unref(pRequestedSrcPad);
            
            // Call the parent class to complete the link to source
            return Nodetr::LinkToSource(pTee);
        }

        /**
         * @brief links this Nodetr as a Sink to the Source Pad of a Splitter Tee
         * @param[in] pTee to link to
         * @param[in] pRequestedSrcPad requested source pad for the Tee to link with
         * @return true if able to successfully link with Tee Src Pad
         */
        virtual bool LinkToSourceTee(DSL_NODETR_PTR pTee, GstPad* pRequestedSrcPad)
        {
            LOG_FUNC();
            m_releaseRequestedPadOnUnlink = false;
            
            // Get a reference to the static sink pad for this GstNodetr
            GstPad* pStaticSinkPad = gst_element_get_static_pad(
                GetGstElement(), "sink");
            if (!pStaticSinkPad)
            {
                LOG_ERROR("Failed to get static sink pad for GstNodetr '" 
                    << GetName() << "'");
                return false;
            }

            LOG_INFO("Linking requested source pad'" << pRequestedSrcPad 
                << "' for GstNodetr '" << GetName() << "'");

            if (gst_pad_link(pRequestedSrcPad, pStaticSinkPad) != GST_PAD_LINK_OK)
            {
                LOG_ERROR("GstNodetr '" << GetName() 
                    << "' failed to link to Source Tee");
                return false;
            }

            // Unreference just the static sink pad and not the requested source pad
            gst_object_unref(pStaticSinkPad);
            
            // Call the parent class to complete the link to source
            return Nodetr::LinkToSource(pTee);
        }
        
        /**
         * @brief unlinks this Nodetr from a previously linked Source Tee
         * @return true if able to successfully unlink from Source Tee
         */
        virtual bool UnlinkFromSourceTee()
        {
            LOG_FUNC();
            
            if (!IsLinkedToSource())
            {
                return false;
            }

            // Get a reference to this GstNodetr's sink pad
            GstPad* pStaticSinkPad = gst_element_get_static_pad(GetGstElement(), "sink");
            if (!pStaticSinkPad)
            {
                LOG_ERROR("Failed to get static sink pad for GstNodetr '" 
                    << GetName() << "'");
                return false;
            }
            
            // Get a reference to the Tee's source pad that is connected
            // to this GstNodetr's sink pad
            GstPad* pRequestedSrcPad = gst_pad_get_peer(pStaticSinkPad);
            if (!pRequestedSrcPad)
            {
                LOG_ERROR("Failed to get requested source pad peer for GstNodetr '"
                    << GetName() << "'");
                return false;
            }

            LOG_INFO("Unlinking requested source pad '" 
                << pRequestedSrcPad << "' for GstNodetr '" << GetName() << "'");

            // It should now be safe to unlink this GstNodetr from the Muxer
            if (!gst_pad_unlink(pRequestedSrcPad, pStaticSinkPad))
            {
                LOG_ERROR("GstNodetr '" << GetName() 
                    << "' failed to unlink from source Tee");
                Nodetr::UnlinkFromSource();
                return false;
            }
            if (m_releaseRequestedPadOnUnlink)
            {
                LOG_INFO("Releasing requested source pad '" 
                    << pRequestedSrcPad << "' for GstNodetr '" << GetName() << "'");
                // Need to release the previously requested sink pad
                gst_element_release_request_pad(GetSource()->GetGstElement(), 
                    pRequestedSrcPad);
            }
            gst_object_unref(pStaticSinkPad);
            gst_object_unref(pRequestedSrcPad);

            return Nodetr::UnlinkFromSource();
        }
        
        /**
         * @brief Returns the current State of this GstNodetr's Parent
         * @return the current state of the Parenet, GST_STATE_NULL if the
         * GstNodetr is currently an orphen. 
         */
        uint GetParentState()
        {
            LOG_FUNC();
            
            if (!m_pParentGstObj)
            {
                return GST_STATE_NULL;
            }
            GstState currentState;
            
            gst_element_get_state(GetParentGstElement(), &currentState, NULL, 1);
            
            LOG_DEBUG("Returning a state of '" 
                << gst_element_state_get_name(currentState) 
                << "' for GstNodetr '" << GetName());
            
            return currentState;
        }

        bool SendEos()
        {
            LOG_FUNC();
            
            return gst_element_send_event(GetGstElement(), gst_event_new_eos());
        }

        uint GetState(GstState& state, GstClockTime timeout)
        {
            LOG_FUNC();

            uint retval = gst_element_get_state(GetGstElement(), 
                &state, NULL, timeout);
            LOG_DEBUG("Get state returned '" << gst_element_state_get_name(state) 
                << "' for GstNodetr '" << GetName() << "'");
            
            return retval;
        }
        
        /**
         * @brief Attempts to set the state of this GstNodetr's GST Element
         * @return true if successful transition, false on failure
         */
        bool SetState(GstState state, GstClockTime timeout)
        {
            LOG_FUNC();
            LOG_INFO("Changing state to '" << gst_element_state_get_name(state) 
                << "' for GstNodetr '" << GetName() << "'");

            GstStateChangeReturn returnVal = gst_element_set_state(GetGstElement(), 
                state);
            switch (returnVal) 
            {
                case GST_STATE_CHANGE_SUCCESS:
                    LOG_INFO("State change completed synchronously for GstNodetr'" 
                        << GetName() << "'");
                    return true;
                case GST_STATE_CHANGE_FAILURE:
                    LOG_ERROR("FAILURE occured when trying to change state to '" 
                        << gst_element_state_get_name(state) << "' for GstNodetr '" 
                        << GetName() << "'");
                    return false;
                case GST_STATE_CHANGE_NO_PREROLL:
                    LOG_INFO("Set state for GstNodetr '" << GetName() 
                        << "' returned GST_STATE_CHANGE_NO_PREROLL");
                    return true;
                case GST_STATE_CHANGE_ASYNC:
                    LOG_INFO("State change will complete asynchronously for GstNodetr '" 
                        << GetName() << "'");
                    break;
                default:
                    break;
            }
            
            // Wait until state change or failure, no timeout.
            if (gst_element_get_state(GetGstElement(), NULL, NULL, timeout) == 
                GST_STATE_CHANGE_FAILURE)
            {
                LOG_ERROR("FAILURE occured waiting for state to change to '" 
                    << gst_element_state_get_name(state) 
                        << "' for GstNodetr '" << GetName() << "'");
                return false;
            }
            LOG_INFO("State change completed asynchronously for GstNodetr'" 
                << GetName() << "'");
            return true;
        }

        uint SyncStateWithParent(GstState& parentState, GstClockTime timeout)
        {
            LOG_FUNC();
            
            uint returnVal = gst_element_sync_state_with_parent(GetGstElement());

            switch (returnVal) 
            {
                case GST_STATE_CHANGE_SUCCESS:
                    LOG_INFO("State change completed synchronously for GstNodetr'" 
                        << GetName() << "'");
                    return returnVal;
                case GST_STATE_CHANGE_FAILURE:
                    LOG_ERROR("FAILURE occured when trying to sync state with Parent for GstNodetr '" 
                        << GetName() << "'");
                    return returnVal;
                case GST_STATE_CHANGE_NO_PREROLL:
                    LOG_INFO("Set state for GstNodetr '" << GetName() 
                        << "' return GST_STATE_CHANGE_NO_PREROLL");
                    return returnVal;
                case GST_STATE_CHANGE_ASYNC:
                    LOG_INFO("State change will complete asynchronously for GstNodetr '" 
                        << GetName() << "'");
                    break;
                default:
                    break;
            }
            uint retval = gst_element_get_state(GST_ELEMENT_PARENT(GetGstElement()), 
                &parentState, NULL, timeout);
            LOG_INFO("Get state returned '" << gst_element_state_get_name(parentState) 
                << "' for Parent of GstNodetr '" << GetName() << "'");
            return retval;
        }
        
    private:
    
        bool m_releaseRequestedPadOnUnlink;
    };

} // DSL namespace    

#endif // _DSL_NODETR_H
