#ifndef DSL_DSL_PIPELINE_H
#define DSL_DSL_PIPELINE_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslBranchBintr.h"
#include "DslPipelineStateMgr.h"
#include "DslPipelineBusSyncMgr.h"
#include "DslSourceBintr.h"
#include "DslDewarperBintr.h"
#include "DslPipelineSourcesBintr.h"
    
namespace DSL 
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_PIPELINE_PTR std::shared_ptr<PipelineBintr>
    #define DSL_PIPELINE_NEW(name) \
        std::shared_ptr<PipelineBintr>(new PipelineBintr(name))

    /**
     * @class PipelineBintr
     * @brief 
     */
    class PipelineBintr : public BranchBintr, public PipelineStateMgr,
        public PipelineBusSyncMgr
    {
    public:
    
        /** 
         * 
         */
        PipelineBintr(const char* pipeline);
        ~PipelineBintr();
        
        /**
         * @brief Links all Child Bintrs owned by this Pipeline Bintr
         * @return True success, false otherwise
         */
        bool LinkAll();

        /**
         * @brief Attempts to link all and play the Pipeline
         * @return true if able to play, false otherwise
         */
        bool Play();

        /**
         * @brief Schedules a Timer Callback to call HandlePause in the mainloop context
         * @return true if HandlePause schedule correctly, false otherwise 
         */
        bool Pause();
        
        /**
         * @brief Pauses the Pipeline by setting its state to GST_STATE_PAUSED
         * Import: must be called in the mainloop's context, i.e. timer callback
         */
        void HandlePause();

        /**
         * @brief Schedules a Timer Callback to call HandleStop in the mainloop context
         * @return true if HandleStop schedule correctly, false otherwise 
         */
        bool Stop();
        
        /**
         * @brief Stops the Pipeline by setting its state to GST_STATE_NULL
         * Import: must be called in the mainloop's context, i.e. timer callback
         */
        void HandleStop();
        
        /**
         * @brief returns whether the Pipeline has all live sources or not.
         * @return true if all sources are live, false otherwise (default when no sources).
         */
        bool IsLive();

        /**
         * @brief adds a single Source Bintr to this Pipeline 
         * @param[in] pSourceBintr shared pointer to Source Bintr to add
         */
        bool AddSourceBintr(DSL_BASE_PTR pSourceBintr);

        bool IsSourceBintrChild(DSL_BASE_PTR pSourceBintr);

        /**
         * @brief returns the number of Sources currently in use by
         * this Pipeline
         */
        uint GetNumSourcesInUse()
        {
            if (!m_pPipelineSourcesBintr)
            {
                return 0;
            }
            return m_pPipelineSourcesBintr->GetNumChildren();
        } 
        
        /**
         * @brief removes a single Source Bintr from this Pipeline 
         * @param[in] pSourceBintr shared pointer to Source Bintr to add
         */
        bool RemoveSourceBintr(DSL_BASE_PTR pSourceBintr);

        /**
         * @brief Gets the current nvbuf memory type in use by the Stream-Muxer
         * @return one of DSL_NVBUF_MEM_TYPE constant values
         */
        uint GetStreammuxNvbufMemType();

        /**
         * @brief Sets the nvbuf memory type for the Stream-Muxer to use
         * @param[in] type one of DSL_NVBUF_MEM_TYPE constant values
         * @return true if the memory type could be set, false otherwise
         */
        bool SetStreammuxNvbufMemType(uint type);

        /**
         * @brief Gets the current batch settings for the Pipeline's Stream-Muxer
         * @param[out] batchSize current batchSize, default == the number of source
         * @param[out] batchTimeout current batch timeout
         * @return true if the batch properties could be read, false otherwise
         */
        void GetStreammuxBatchProperties(uint* batchSize, int* batchTimeout);

        /**
         * @brief Sets the current batch settings for the Pipeline's Stream-Muxer
         * @param[in] batchSize new batchSize to set, default == the number of sources
         * @param[in] batchTimeout timeout value to set in ms
         * @return true if the batch properties could be set, false otherwise
         */
        bool SetStreammuxBatchProperties(uint batchSize, int batchTimeout);

        /**
         * @brief Gets the current dimensions for the Pipeline's Stream Muxer
         * @param[out] width width in pixels for the current setting
         * @param[out] height height in pixels for the curren setting
         */
        void GetStreammuxDimensions(uint* width, uint* height);

        /**
         * @brief Set the dimensions for the Pipeline's Stream Muxer
         * @param width width in pixels to set the streamMux Output
         * @param height height in pixels to set the Streammux output
         * @return true if the output dimensions could be set, false otherwise
         */
        bool SetStreammuxDimensions(uint width, uint height);
        
        /**
         * @brief Gets the current setting for the Pipeline's Muxer padding
         * @return true if padding is enabled, false otherwisee
         */
        bool GetStreammuxPadding();

        /**
         * @brief Sets, enables/disables the Pipeline's Stream Muxer padding.
         * @param enabled set to true to enable padding, false otherwise.
         * @return true if the Padding enabled setting could be set, false otherwise.
         */
        bool SetStreammuxPadding(boolean enabled);
        
        /**
         * @brief Gets the current setting for the Pipeline's Streammuxer
         * num-surfaces-per-frame seting
         * @return current setting for the number of surfaces [1..4].
         */
        uint GetStreammuxNumSurfacesPerFrame();

        /**
         * @brief Sets the current setting for the PipelineSourcesBintr's Streammuxer
         * num-surfaces-per-frame seting
         * @param[in] num new value for the number of surfaces [1..4].
         * @return true if the number setting could be set, false otherwisee
         */
        bool SetStreammuxNumSurfacesPerFrame(uint num);
        
        /**
         * @brief Gets the current setting for the Pipeline's Muxer padding
         * @return true if padding is enabled, false otherwisee
         */
        bool GetStreammuxSyncInputsEnabled();

        /**
         * @brief Sets the Pipeline's Streammuxer padding.
         * @param enabled set to true to enable sync-inputs, false otherwise.
         * @return true if the Padding enabled setting could be set, false otherwise.
         */
        bool SetStreammuxSyncInputsEnabled(boolean enabled);
        
        /**
         * @brief Adds a TilerBintr to be added to the Stream-muxers output
         * on link and play.
         * @param[in] pTilerBintr shared pointer to the tiler to add.
         * @return true if the Tiler was successfully added, false otherwise.
         */
        bool AddStreammuxTiler(DSL_BASE_PTR pTilerBintr);
        
        /**
         * @brief Removes a TilerBintr previously added with AddStreammuxTiler.
         * @return true if the TileBintr was successfully removed, false otherwise.
         */
        bool RemoveStreammuxTiler();
        
        /**
         * @brief dumps a Pipeline's graph to dot file.
         * @param[in] filename name of the file without extention.
         * The caller is responsible for providing a correctly formated filename
         * The diretory location is specified by the GStreamer debug 
         * environment variable GST_DEBUG_DUMP_DOT_DIR
         */ 
        void DumpToDot(char* filename);
        
        /**
         * @brief dumps a Pipeline's graph to dot file prefixed
         * with the current timestamp.  
         * @param[in] filename name of the file without extention.
         * The caller is responsible for providing a correctly formated filename
         * The diretory location is specified by the GStreamer debug 
         * environment variable GST_DEBUG_DUMP_DOT_DIR
         */ 
        void DumpToDotWithTs(char* filename);
        
    private:

        /**
         * @brief 0-based unique (static) pipeline-id generator for the 
         * PipelineBintr class. Incremented after each pipeline instantiation.
         */
        static std::vector<bool> m_usedPipelineIds;
        
        /**
         * @brief unique pipeline-id for this PipelineBintr
         */
        uint m_pipelineId;

        /**
         * @brief parent bin for all Source bins in this PipelineBintr
         */
        DSL_PIPELINE_SOURCES_PTR m_pPipelineSourcesBintr;
        
        
        /**
         * @brief optional Tiler for the Stream-muxer's output
         */
        DSL_TILER_PTR m_pStreammuxTilerBintr;
        
        
    }; // Pipeline
    
    /**
     * @brief Timer callback function to Stop a Pipeline in the mainloop context.  
     * @param pPipeline shared pointer to the Pipeline that started the timer to 
     * schedule the stop
     * @return false always to self destroy the on-shot timer.
     */
    static int PipelineStop(gpointer pPipeline);
    
} // Namespace

#endif // _DSL_PIPELINE_H
