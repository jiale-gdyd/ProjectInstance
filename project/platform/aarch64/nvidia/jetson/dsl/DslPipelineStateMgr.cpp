#include "Dsl.h"
#include "DslPipelineStateMgr.h"

namespace DSL
{
    PipelineStateMgr::PipelineStateMgr(GstObject* pGstPipeline)
        : m_pGstPipeline(pGstPipeline)
        , m_pMainContext(NULL)
        , m_pMainLoop(NULL)
        , m_pBusWatch(NULL)
        , m_eosFlag(false)
        , m_errorNotificationTimerId(0)
    {
        LOG_FUNC();

        _initMaps();

        m_pGstBus = gst_pipeline_get_bus(GST_PIPELINE(m_pGstPipeline));

        // Add the bus-watch and callback function to the default main context
        m_busWatchId = gst_bus_add_watch(m_pGstBus, bus_watch, this);
    }

    PipelineStateMgr::~PipelineStateMgr()
    {
        LOG_FUNC();
        
        if (m_pMainLoop)
        {
            DeleteMainLoop();
        }
        gst_bus_remove_watch(m_pGstBus);
        gst_object_unref(m_pGstBus);
    }

    bool PipelineStateMgr::NewMainLoop()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_busWatchMutex);

        if (m_pMainLoop)
        {
            LOG_ERROR("A main-loop has already been created for Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "'");
            return false;
        }

        // We need remove the current bus watch added to the default main-context
        gst_bus_remove_watch(m_pGstBus);
        m_busWatchId = 0;
        
        // Create own main-context for the Pipeline first
        m_pMainContext = g_main_context_new();
        if (!m_pMainContext)
        {
            LOG_ERROR("Pipeline '" << gst_object_get_name(m_pGstPipeline) 
                << "' failed to create own main-context");
            return false;
        }

        // Create the main-loop for the Pipeline to run in its own main-context
        m_pMainLoop = g_main_loop_new(m_pMainContext, FALSE);
        if (!m_pMainLoop)
        {
            LOG_ERROR("Pipeline '" << gst_object_get_name(m_pGstPipeline)
                << "' failed to create main-loop");
            return false;
        }
        
        // Create a new bus-watch 
        m_pBusWatch = gst_bus_create_watch(m_pGstBus);

        // Setup our bus-watch callback and then attach the bus-watch to 
        // the Pipeline's own main-context created above.
        g_source_set_callback(m_pBusWatch, (GSourceFunc)bus_watch, this, NULL);
        g_source_attach(m_pBusWatch, m_pMainContext);
        
        return true;
    }
    
    bool PipelineStateMgr::RunMainLoop()
    {
        LOG_FUNC();
        
        if (!m_pMainLoop)
        {
            LOG_ERROR("A Main-Loop has NOT been created for Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "'");
            return false;
        }
        if (g_main_loop_is_running(m_pMainLoop))
        {
            LOG_ERROR("A Main-Loop is already running for Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "'");
            return false;
        }
        // Acquire context and set it as the thread-default context for the current thread.
        g_main_context_push_thread_default(m_pMainContext);
        
        // call will block until QuitMainLoop is called from another thread.
        g_main_loop_run(m_pMainLoop);
        
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_mainLoopMutex);
        
        // Pop context off the thread-default context stack and signal client
        g_main_context_pop_thread_default(m_pMainContext);
        
        g_cond_signal(&m_mainLoopCond);
        
        return true;
    }
    
    
    bool PipelineStateMgr::QuitMainLoop()
    {
        LOG_FUNC();

        if (!m_pMainLoop)
        {
            LOG_ERROR("A Main-Loop has NOT been created for Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "'");
            return false;
        }
        if (!g_main_loop_is_running(m_pMainLoop))
        {
            LOG_ERROR("Main-loop for Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "' is not running");
            return false;
        }
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_mainLoopMutex);
        g_main_loop_quit(m_pMainLoop);
        g_cond_wait(&m_mainLoopCond, &m_mainLoopMutex);
        
        return true;
    }
    
    bool PipelineStateMgr::DeleteMainLoop()
    {
        LOG_FUNC();
        
        if (!m_pMainLoop)
        {
            LOG_ERROR("A Main-Loop has NOT been created for Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "'");
            return false;
        }
        // destroy the bus-watch - which unattaches the bus-watch from the main-context
        g_source_destroy(m_pBusWatch);
        
        g_main_loop_unref(m_pMainLoop);
        g_main_context_unref(m_pMainContext);
        m_pBusWatch = NULL;
        m_pMainLoop = NULL;
        m_pMainContext = NULL;

        // re-install the watch function for the message bus with the default 
        // main-context - setting it back to its default state.
        m_busWatchId = gst_bus_add_watch(m_pGstBus, bus_watch, this);
        
        return true;
    }

    bool PipelineStateMgr::AddStateChangeListener(dsl_state_change_listener_cb listener, void* clientData)
    {
        LOG_FUNC();
        
        if (m_stateChangeListeners.find(listener) != m_stateChangeListeners.end())
        {   
            LOG_ERROR("Pipeline listener is not unique");
            return false;
        }
        m_stateChangeListeners[listener] = clientData;
        
        return true;
    }

    bool PipelineStateMgr::RemoveStateChangeListener(dsl_state_change_listener_cb listener)
    {
        LOG_FUNC();
        
        if (m_stateChangeListeners.find(listener) == m_stateChangeListeners.end())
        {   
            LOG_ERROR("Pipeline listener was not found");
            return false;
        }
        m_stateChangeListeners.erase(listener);
        
        return true;
    }

    bool PipelineStateMgr::AddEosListener(dsl_eos_listener_cb listener, void* clientData)
    {
        LOG_FUNC();
        
        if (m_eosListeners.find(listener) != m_eosListeners.end())
        {   
            LOG_ERROR("Pipeline listener is not unique");
            return false;
        }
        m_eosListeners[listener] = clientData;
        
        return true;
    }

    bool PipelineStateMgr::IsEosListener(dsl_eos_listener_cb listener)
    {
        LOG_FUNC();
        
        return (m_eosListeners.find(listener) != m_eosListeners.end());
    }

    bool PipelineStateMgr::RemoveEosListener(dsl_eos_listener_cb listener)
    {
        LOG_FUNC();
        
        if (m_eosListeners.find(listener) == m_eosListeners.end())
        {   
            LOG_ERROR("Pipeline listener was not found");
            return false;
        }
        m_eosListeners.erase(listener);
        
        return true;
    }

    bool PipelineStateMgr::AddErrorMessageHandler(dsl_error_message_handler_cb handler, void* clientData)
    {
        LOG_FUNC();
        
        if (m_errorMessageHandlers.find(handler) != m_errorMessageHandlers.end())
        {   
            LOG_ERROR("Pipeline handler is not unique");
            return false;
        }
        m_errorMessageHandlers[handler] = clientData;
        
        return true;
    }

    bool PipelineStateMgr::RemoveErrorMessageHandler(dsl_error_message_handler_cb handler)
    {
        LOG_FUNC();
        
        if (m_errorMessageHandlers.find(handler) == m_errorMessageHandlers.end())
        {   
            LOG_ERROR("Pipeline handler was not found");
            return false;
        }
        m_errorMessageHandlers.erase(handler);
        
        return true;
    }
    
    bool PipelineStateMgr::HandleBusWatchMessage(GstMessage* pMessage)
    {
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_busWatchMutex);

        GstClockTime clockTime;
        GstStreamStatusType statusType;
        GstElement* pElement(NULL);
        GstFormat format(GST_FORMAT_UNDEFINED);
        guint64 processed(0);
        guint64 dropped(0);
        GError* error(NULL);
        gchar* debugInfo(NULL);
        gint percent(0);
        gchar* propertyName(NULL);
        GstProgressType progressType(GST_PROGRESS_TYPE_ERROR);
        gchar* code;
        gchar* text;

        const gchar* name = gst_message_type_get_name(GST_MESSAGE_TYPE(pMessage));

        switch (GST_MESSAGE_TYPE(pMessage))
        {
        case GST_MESSAGE_ASYNC_DONE:
            gst_message_parse_async_done(pMessage, &clockTime);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   source    : " << GST_OBJECT_NAME(pMessage->src));
            LOG_INFO("   time      : " << clockTime);
            break;
            
        case GST_MESSAGE_STREAM_STATUS:
            gst_message_parse_stream_status(pMessage, &statusType, &pElement);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   source    : " << GST_OBJECT_NAME(pMessage->src));
            LOG_INFO("   type      : " << statusType);
            LOG_INFO("   element   : " << GST_ELEMENT_NAME(pElement));
            break;
            
        case GST_MESSAGE_QOS:
            gst_message_parse_qos_stats(pMessage, &format, &processed, &dropped);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   source    : " << GST_OBJECT_NAME(pMessage->src));
            LOG_INFO("   format    : " << gst_format_get_name(format));
            LOG_INFO("   processed : " << processed);
            LOG_INFO("   dropped   : " << dropped);
            break;
            
        case GST_MESSAGE_BUFFERING:
            gst_message_parse_buffering(pMessage, &percent);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   source    : " << GST_OBJECT_NAME(pMessage->src));
            LOG_INFO("   percent   : " << percent);
            break;
            
        case GST_MESSAGE_LATENCY:
            LOG_INFO("Message type : " << name);
            break;
            
        case GST_MESSAGE_PROGRESS:
            gst_message_parse_progress(pMessage,
                &progressType, &code, &text);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   source    : " << GST_OBJECT_NAME(pMessage->src));
            LOG_INFO("   type      : " << progressType);
            LOG_INFO("   code      : " << code);
            LOG_INFO("   text      : " << text);
            break;
            
        case GST_MESSAGE_INFO:
            gst_message_parse_info(pMessage, &error, &debugInfo);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   info      : " << error->message);
            if(debugInfo)
                LOG_INFO("   debug     : " << debugInfo);
            g_error_free(error);
            g_free(debugInfo);
            break;

        case GST_MESSAGE_WARNING:
            gst_message_parse_warning(pMessage, &error, &debugInfo);
            LOG_INFO("Message type : " << name);
            LOG_INFO("   warning   : " << error->message);
            if(debugInfo)
                LOG_INFO("   debug     : " << debugInfo);
            g_error_free(error);
            g_free(debugInfo);
            break;
            
        case GST_MESSAGE_EOS:
            HandleEosMessage(pMessage);
            break;
        case GST_MESSAGE_ERROR:
            HandleErrorMessage(pMessage);            
            break;
        case GST_MESSAGE_STATE_CHANGED:
            HandleStateChanged(pMessage);
            break;
        case GST_MESSAGE_APPLICATION:
            HandleApplicationMessage(pMessage);
            break;

        case GST_MESSAGE_ELEMENT:
        case GST_MESSAGE_DURATION_CHANGED:
        case GST_MESSAGE_NEW_CLOCK:
        case GST_MESSAGE_TAG:
            break;
        default:
            LOG_INFO("Unhandled message type:: " << name);
        }
        
        return true;
    }

    bool PipelineStateMgr::HandleStateChanged(GstMessage* pMessage)
    {
        if (GST_ELEMENT(GST_MESSAGE_SRC(pMessage)) != GST_ELEMENT(m_pGstPipeline))
        {
            return false;
        }

        GstState oldstate, newstate;
        gst_message_parse_state_changed(pMessage, &oldstate, &newstate, NULL);

        LOG_INFO(m_mapPipelineStates[oldstate] << " => " << m_mapPipelineStates[newstate]);

        // iterate through the map of state-change-listeners calling each
        for(auto const& imap: m_stateChangeListeners)
        {
            try
            {
                imap.first((uint)oldstate, (uint)newstate, imap.second);
            }
            catch(...)
            {
                LOG_ERROR("Exception calling Client State-Change-Listener");
            }
        }
        return true;
    }
    
    void PipelineStateMgr::HandleEosMessage(GstMessage* pMessage)
    {
        LOG_INFO("EOS message recieved for Pipeline '" 
            << gst_object_get_name(m_pGstPipeline) << "'");
        
        // If the EOS event was sent from HandleStop
        if (m_eosFlag)
        {
            return;
        }
        
        // Action EOS so set the flag
        m_eosFlag = true;
        
        // iterate through the map of EOS-listeners calling each
        for(auto const& imap: m_eosListeners)
        {
            try
            {
                imap.first(imap.second);
            }
            catch(...)
            {
                LOG_ERROR("Exception calling Client EOS-Lister");
            }
        }
    }
    
    void PipelineStateMgr::HandleApplicationMessage(GstMessage* pMessage)
    {
        LOG_FUNC();
        
        const GstStructure* msgPayload = gst_message_get_structure(pMessage);

        // only one application message at this time. 
        if(gst_structure_has_name(msgPayload, "stop-pipline"))
        {
            HandleStop();
        }
        else
        {
            LOG_ERROR("Unknown Application message received by Pipeline '"
                << gst_object_get_name(m_pGstPipeline) << "'");
        }
    }
    
    void PipelineStateMgr::HandleErrorMessage(GstMessage* pMessage)
    {
        LOG_FUNC();
        
        GError* error = NULL;
        gchar* debugInfo = NULL;
        gst_message_parse_error(pMessage, &error, &debugInfo);

        LOG_ERROR("Error message '" << error->message << "' received from '" 
            << GST_OBJECT_NAME(pMessage->src) << "'");
            
        if (debugInfo)
        {
            LOG_DEBUG("Debug info: " << debugInfo);
        }

        // persist the last error information
        std::string cstrSource(GST_OBJECT_NAME(pMessage->src));
        std::string cstrMessage(error->message);

        std::wstring wstrSource(cstrSource.begin(), cstrSource.end());
        std::wstring wstrMessage(cstrMessage.begin(), cstrMessage.end());
        
        // Setting the last error message will invoke a timer thread to notify all client handlers.
        SetLastErrorMessage(wstrSource, wstrMessage);
        
        g_error_free(error);
        g_free(debugInfo);
    }    

    void PipelineStateMgr::GetLastErrorMessage(std::wstring& source, std::wstring& message)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_lastErrorMutex);
        
        source = m_lastErrorSource;
        message = m_lastErrorMessage;
    }

    void PipelineStateMgr::SetLastErrorMessage(std::wstring& source, std::wstring& message)
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_lastErrorMutex);

        m_lastErrorSource = source;
        m_lastErrorMessage = message;
        
        if (m_errorMessageHandlers.size())
        {
            m_errorNotificationTimerId = g_timeout_add(1, ErrorMessageHandlersNotificationHandler, this);
        }
    }
    
    int PipelineStateMgr::NotifyErrorMessageHandlers()
    {
        LOG_FUNC();
        LOCK_MUTEX_FOR_CURRENT_SCOPE(&m_lastErrorMutex);

        // iterate through the map of state-change-listeners calling each
        for(auto const& imap: m_errorMessageHandlers)
        {
            try
            {
                imap.first(m_lastErrorSource.c_str(), m_lastErrorMessage.c_str(), imap.second);
            }
            catch(...)
            {
                LOG_ERROR("PipelineStateMgr threw exception calling Client Error-Message-Handler");
            }
        }
        // clear the timer id and return false to self remove
        m_errorNotificationTimerId = 0;
        return false;
    }

    void PipelineStateMgr::_initMaps()
    {
        m_mapPipelineStates[GST_STATE_READY] = "GST_STATE_READY";
        m_mapPipelineStates[GST_STATE_PLAYING] = "GST_STATE_PLAYING";
        m_mapPipelineStates[GST_STATE_PAUSED] = "GST_STATE_PAUSED";
        m_mapPipelineStates[GST_STATE_NULL] = "GST_STATE_NULL";
    }
    
    static gboolean bus_watch(GstBus* bus, GstMessage* pMessage, gpointer pPipeline)
    {
        return static_cast<PipelineStateMgr*>(pPipeline)->HandleBusWatchMessage(pMessage);
    }    
    
    static int ErrorMessageHandlersNotificationHandler(gpointer pPipeline)
    {
        return static_cast<PipelineStateMgr*>(pPipeline)->
            NotifyErrorMessageHandlers();
    }
    
}
