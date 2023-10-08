#ifndef DSL_DSL_PIPELINE_SOURCES_BINTR_H
#define DSL_DSL_PIPELINE_SOURCES_BINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslSourceBintr.h"

namespace DSL
{
    #define DSL_PIPELINE_SOURCES_PTR std::shared_ptr<PipelineSourcesBintr>
    #define DSL_PIPELINE_SOURCES_NEW(name, uniquePipelineId) \
        std::shared_ptr<PipelineSourcesBintr> \
           (new PipelineSourcesBintr(name, uniquePipelineId))

    class PipelineSourcesBintr : public Bintr
    {
    public: 
    
        PipelineSourcesBintr(const char* name, uint uniquePipelineId);

        ~PipelineSourcesBintr();
        
        /**
         * @brief adds a child SourceBintr to this PipelineSourcesBintr
         * @param pChildSource shared pointer to SourceBintr to add
         * @return true if the SourceBintr was added correctly, false otherwise
         */
        bool AddChild(DSL_SOURCE_PTR pChildSource);
        
        /**
         * @brief removes a child SourceBintr from this PipelineSourcesBintr
         * @param pChildElement a shared pointer to SourceBintr to remove
         * @return true if the SourceBintr was removed correctly, false otherwise
         */
        bool RemoveChild(DSL_SOURCE_PTR pChildSource);

        /**
         * @brief overrides the base method and checks in m_pChildSources only.
         */
        bool IsChild(DSL_SOURCE_PTR pChildSource);

        /**
         * @brief overrides the base Noder method to only return the number of 
         * child SourceBintrs and not the total number of children... 
         * i.e. exclude the nuber of child Elementrs from the count
         * @return the number of Child SourceBintrs held by this PipelineSourcesBintr
         */
        uint GetNumChildren()
        {
            LOG_FUNC();
            
            return m_pChildSources.size();
        }

        /**
         * @brief interates through the list of child source bintrs setting 
         * their Sensor Id's and linking to the Streammux
         */
        bool LinkAll();
        
        /**
         * @brief interates through the list of child source bintrs unlinking
         * them from the Streammux and reseting their Sensor Id's
         */
        void UnlinkAll();

        void EosAll();

        /**
         * @brief Gets the current Streammuxer "play-type-is-live" setting
         * @return true if play-type is live, false otherwise
         */
        bool StreammuxPlayTypeIsLiveGet();

        /**
         * @brief Sets the current Streammuxer play type based on the first source added
         * @param isLive set to true if all sources are to be Live, and therefore live only.
         * @return true if live-source is succesfully set, false otherwise
         */
        bool StreammuxPlayTypeIsLiveSet(bool isLive);

        /**
         * @brief Gets the current Streammuxer NVIDIA buffer memory type.
         * @return one of the DSL_NVBUF_MEM_TYPE constant values.
         */
        uint GetStreammuxNvbufMemType();

        /**
         * @brief Sets the Streammuxer's NVIDIA buffer memory type.
         * @param[in] type one of the DSL_NVBUF_MEM_TYPE constant values.
         * @return true if nvbuf-memory-type is succesfully set, false otherwise
         */
        bool SetStreammuxNvbufMemType(uint type);
        
        /**
         * @brief Gets the current batch settings for the SourcesBintr's Stream Muxer.
         * @param[out] batchSize current batchSize, default == the number of source.
         * @param[out] batchTimeout current batch timeout. Default = -1, disabled.
         */
        void GetStreammuxBatchProperties(uint* batchSize, int* batchTimeout);

        /**
         * @brief Sets the current batch settings for the SourcesBintr's Stream Muxer.
         * @param[in] batchSize new batchSize to set, default == the number of sources.
         * @param[in] batchTimeout timeout value to set in ms. Set to -1 to disable.
         * @return true if batch-properties are succesfully set, false otherwise.
         */
        bool SetStreammuxBatchProperties(uint batchSize, int batchTimeout);

        /**
         * @brief Gets the current dimensions for the SourcesBintr's Stream Muxer.
         * @param[out] width width in pixels for the current setting.
         * @param[out] height height in pixels for the curren setting.
         */
        void GetStreammuxDimensions(uint* width, uint* height);

        /**
         * @brief Set the dimensions for the SourcesBintr's Streammuxer.
         * @param width width in pixels to set the streamMux Output.
         * @param height height in pixels to set the Streammux output.
         * @return true if dimensions are succesfully set, false otherwise.
         */
        bool SetStreammuxDimensions(uint width, uint height);
        
        /**
         * @brief Gets the current setting for the PipelineSourcesBintr's 
         * Streammuxer padding enabled property.
         * @preturn true if enabled, false otherwise.
         */
        boolean GetStreammuxPaddingEnabled();

        /**
         * @brief Sets the PipelineSourcesBintr's Streammuxer padding 
         * enabled property.
         * @param enabled set to true to enable padding, false otherwise.
         * @return true if padding enabled was succesfully set, false otherwise.
         */
        bool SetStreammuxPaddingEnabled(boolean enabled);

        /**
         * @brief Gets the current setting for the PipelineSourcesBintr's Streammuxer
         * num-surfaces-per-frame seting
         * @return current setting for the number of surfaces [1..4].
         */
        uint GetStreammuxNumSurfacesPerFrame();

        /**
         * @brief Sets the current setting for the PipelineSourcesBintr's 
         * Streammuxer num-surfaces-per-frame seting.
         * @param[in] num new value for the number of surfaces [1..4].
         * @return true if dimensions are succesfully set, false otherwise.
         */
        bool SetStreammuxNumSurfacesPerFrame(uint num);
        
        /**
         * @brief Gets the current setting for the PipelineSourcesBintr's 
         * Streammuxer sync-inputs enabled property.
         * @preturn true if enabled, false otherwise.
         */
        boolean GetStreammuxSyncInputsEnabled();
        
        /**
         * @brief Sets the PipelineSourcesBintr's Streammuxer sync-inputs 
         * enabled property.
         * @param enabled set to true to enable sync-inputs, false otherwise.
         * @return true if padding enabled was succesfully set, false otherwise.
         */
        bool SetStreammuxSyncInputsEnabled(boolean enabled);
        
        /**
         * @brief Set the GPU ID for the PipelineSourcesBintr's Streammuxer
         * @return true if successfully set, false otherwise.
         */
        bool SetGpuId(uint gpuId);
        
        /**
         * @brief Calls on all child Sources to disable their EOS consumers.
         */
        void DisableEosConsumers();

    private:
        /**
         * @brief adds a child Elementr to this PipelineSourcesBintr
         * @param pChildElement a shared pointer to the Elementr to add
         * @return a shared pointer to the Elementr if added correctly, 
         * nullptr otherwise
         */
        bool AddChild(DSL_BASE_PTR pChildElement);
        
        /**
         * @brief removes a child Elementr from this PipelineSourcesBintr
         * @param pChildElement a shared pointer to the Elementr to remove
         */
        bool RemoveChild(DSL_BASE_PTR pChildElement);
        
        /**
         * @brief unique id for the Parent Pipeline, used to offset all source
         * Id's (if greater than 0)
         */
        uint m_uniquePipelineId; 
         
        /**
         * @brief Pad Probe Event Handler to consume all dowstream EOS events
         * Will be created if and when a RTSP source is added to this 
         * PipelineSourcesBintr.
         */
        DSL_PPEH_EOS_CONSUMER_PTR m_pEosConsumer;
        
        /**
         * @brief Source PadBufferProbetr for the SourceIdOffsetterPadProbeHandler 
         * m_pSourceIdOffsetter owned by this PipelineSourcesBintr.
         */
        DSL_PAD_BUFFER_PROBE_PTR m_pSrcPadBufferProbe;
        
        /**
         * @brief Pad Probe Handler to add the source-id offset (based on unique 
         * pipeline-id) for this PipelineSourcesBintr
         */
        DSL_PPH_SOURCE_ID_OFFSETTER_PTR m_pSourceIdOffsetter;

    public:

        DSL_ELEMENT_PTR m_pStreammux;
        
        /**
         * @brief container of all child sources mapped by their unique names
         */
        std::map<std::string, DSL_SOURCE_PTR> m_pChildSources;
        
        /**
         * @brief container of all child sources mapped by their unique stream-id
         */
        std::map<uint, DSL_SOURCE_PTR> m_pChildSourcesIndexed;

        /**
         * @brief Each source is assigned a unique pad/stream id used to define the
         * streammuxer sink pad when linking. The vector is used on add/remove 
         * to find the next available pad id.
         */
        std::vector<bool> m_usedRequestPadIds;
        
        /**
         * @brief true if all sources are live, false if all sources are non-live
         */
        bool m_areSourcesLive;

        /**
         * @brief current NVIDIA buffer memory type in use by the Streammuxer
         * set to DLS_NVBUF_MEM_DEFAULT on creation.
         */
        uint m_nvbufMemType;

        /**
         * @brief Stream-muxer batch timeout used when waiting for all sources
         * to produce a frame when batching together
         */
        gint m_batchTimeout;
        
        /**
         * @brief Stream-muxer batched frame output width in pixels
         */
        gint m_streamMuxWidth;

        /**
         * @brief Stream-muxer batched frame output height in pixels
         */
        gint m_streamMuxHeight;

        /**
         * @brief true if frame padding is enabled, false otherwise
         */
        boolean m_isPaddingEnabled;
        
        /**
         * @brief Number of surfaces-per-frame stream-muxer setting
         */
        int m_numSurfacesPerFrame;

        /**
         * @brief Compute Scaling HW to use. Applicable only for Jetson.
         * 0 (Default): Default, GPU for Tesla, VIC for Jetson
         * 1 (GPU): GPU
         * 2 (VIC): VIC
         */
        uint m_computeHw;
        
        /**
         * @brief Number of buffers in output buffer pool
         */
        uint m_bufferPoolSize;
        
        /**
         * @brief Attach system timestamp as ntp timestamp, otherwise ntp 
         * timestamp calculated from RTCP sender reports.
         */
        boolean m_attachSysTs;
        
        /**
         * @brief Interpolation method - refer to enum NvBufSurfTransform_Inter 
         * in nvbufsurftransform.h for valid values.
         */
        uint m_interpolationMethod;
        
        /**
         * @brief if true, sychronizes input frames using PTS.
         */
        boolean m_syncInputs;
        
        /**
         * @brief Duration of input frames in milliseconds for use in NTP timestamp 
         * correction based on frame rate. If set to 0 (default), frame duration is 
         * inferred automatically from PTS values seen at RTP jitter buffer. When 
         * there is change in frame duration between the RTP jitter buffer and the 
         * nvstreammux, this property can be used to indicate the correct frame rate 
         * to the nvstreammux, for e.g. when there is an audiobuffersplit GstElement 
         * before nvstreammux in the pipeline. If set to -1, disables frame rate 
         * based NTP timestamp correction. 
         */
        int m_frameDuration;
    };

    
}

#endif // _DSL_PIPELINE_SOURCES_BINTR_H
