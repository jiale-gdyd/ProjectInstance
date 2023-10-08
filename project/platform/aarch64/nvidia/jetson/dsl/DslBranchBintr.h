#ifndef DSL_DSL_BRANCHBINTR_H
#define DSL_DSL_BRANCHBINTR_H

#include "Dsl.h"
#include "DslApi.h"
#include "DslPreprocBintr.h"
#include "DslInferBintr.h"
#include "DslSegVisualBintr.h"
#include "DslTrackerBintr.h"
#include "DslOfvBintr.h"
#include "DslOsdBintr.h"
#include "DslTilerBintr.h"
#include "DslPipelineSInfersBintr.h"
#include "DslMultiBranchesBintr.h"
#include "DslSinkBintr.h"

namespace DSL 
{
    /**
     * @brief convenience macros for shared pointer abstraction
     */
    #define DSL_BRANCH_PTR std::shared_ptr<BranchBintr>
    #define DSL_BRANCH_NEW(name) \
        std::shared_ptr<BranchBintr>(new BranchBintr(name))

    /**
     * @class BranchBintr
     * @brief 
     */
    class BranchBintr : public Bintr
    {
    public:
    
        /** 
         * 
         */
        BranchBintr(const char* name, bool isPipeline = false);

        /**
         * @brief adds a PreprocBintr to this Branch 
         * @param[in] pPreprocBintr shared pointer to PreprocBintr to add
         * @return true on successful add, false otherwise
         */
        bool AddPreprocBintr(DSL_BASE_PTR pPreprocBintr);

        /**
         * @brief removes a PreprocBintr from this Branch 
         * @param[in] pPreprocBintr shared pointer to PreprocBintr to remove
         * @return true on successful remove, false otherwise
         */
        bool RemovePreprocBintr(DSL_BASE_PTR pPreprocBintr);
        
        /**
         * @brief adds a single GIE or TIS PrimaryInferBintr to this Branch 
         * @param[in] pPrimaryInferBintr shared pointer to PrmaryInferBintr to add
         * @return true on successful add, false otherwise
         */
        bool AddPrimaryInferBintr(DSL_BASE_PTR pPrimaryInferBintr);

        /**
         * @brief removes a single GIE or TIS PrimaryInferBintr from this Branch 
         * @param[in] pPrimaryInferBintr shared pointer to PrmaryInferBintr to remove
         * @return true on successful remove, false otherwise
         */
        bool RemovePrimaryInferBintr(DSL_BASE_PTR pPrimaryInferBintr);
        
        /**
         * @brief adds a single GIE or TIS SecondaryInferBintr to this Branch 
         * @param[in] pSecondaryInferBintr shared pointer to SecondaryInferBintr to add
         * @return true on successful add, false otherwise
         */
        bool AddSecondaryInferBintr(DSL_BASE_PTR pSecondaryInferBintr);

        /**
         * @brief adds a single SegVisualBintr to this Branch 
         * @param[in] pSegVisualBintr shared pointer to the SegVisual Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddSegVisualBintr(DSL_BASE_PTR pSegVisualBintr);

        /**
         * @brief adds a single TrackerBintr to this Branch 
         * @param[in] pTrackerBintr shared pointer to the Tracker Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddTrackerBintr(DSL_BASE_PTR pTrackerBintr);
        
        /**
         * @brief remove a single TrackerBintr from this Branch 
         * @param[in] pTrackerBintr shared pointer to the Tracker Bintr to remove
         * @return true on successful remove, false otherwise
         */
        bool RemoveTrackerBintr(DSL_BASE_PTR pTrackerBintr);
        
        /**
         * @brief adds a single OfvBintr to this Branch 
         * @param[in] pOfvBintr shared pointer to the OFV Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddOfvBintr(DSL_BASE_PTR pOfvBintr);
        
        /**
         * @brief adds a single TilerBintr to this Branch 
         * @param[in] pDisplayBintr shared pointer to Tiler Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddTilerBintr(DSL_BASE_PTR pTilerBintr);
        
        /**
         * @brief adds an OsdBintr to this Branch 
         * @param[in] pOsdBintr shared pointer to OSD Bintr to add
         * @return true on succesful add, false otherwise.
         */
        bool AddOsdBintr(DSL_BASE_PTR pOsdBintr);
        
        /**
         * @brief removes a OsdBintr from this Branch 
         * @param[in] pOsdBintr shared pointer to OSD Bintr to remove
         * @return true on succesful remove, false otherwise.
         */
        bool RemoveOsdBintr(DSL_BASE_PTR pOsdBintr);
        
        /**
         * @brief adds a single TeeBintr to this Branch 
         * @param[in] pDisplayBintr shared pointer to Tiler Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddTeeBintr(DSL_BASE_PTR pTeeBintr);
        
        /**
         * @brief adds a single DemuxerBintr to this Branch 
         * @param[in] pDisplayBintr shared pointer to Tiler Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddDemuxerBintr(DSL_BASE_PTR pDemuxerBintr);

        /**
         * @brief adds a single SplitterBintr to this Branch 
         * @param[in] pDisplayBintr shared pointer to Tiler Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddSplitterBintr(DSL_BASE_PTR pSplitterBintr);

        /**
         * @brief adds a single SinkBintr to this Branch 
         * @param[in] pSinkBintr shared pointer to Sink Bintr to add
         * @return true on successful add, false otherwise
         */
        bool AddSinkBintr(DSL_BASE_PTR pSinkBintr);

        /**
         * @brief check if a SinkBintr is a child of the BranchBintr
         * @param pSinkBintr
         * @return true if SinkBintr is a child, false otherwise
         */
        bool IsSinkBintrChild(DSL_BASE_PTR pSinkBintr);

        /**
         * @brief removes a single SinkBintr from this Branch 
         * @param[in] pSinkBintr shared pointer to Sink Bintr to add
         * @return true on successful remove, false otherwise
         */
        bool RemoveSinkBintr(DSL_BASE_PTR pSinkBintr);

        bool LinkAll();
        
        void UnlinkAll();
        
    protected:
    
        /**
         * @brief Created and added as first componet if instantiated as a
         * Branch t linked to Demuxer or Splitter Tees. 
         */
        DSL_ELEMENT_PTR  m_pBranchQueue;
        
        /**
         * @brief vector of linked components to simplfy the unlink process
         */
        std::vector<DSL_BINTR_PTR> m_linkedComponents;
        
        /**
         * @brief optional, one at most PreprocBintr for this Branch
         */
        DSL_PREPROC_PTR m_pPreprocBintr;
        
        /**
         * @brief Index variable to incremment/assign on Primary InferBintr add.
         */
        uint m_nextPrimaryInferBintrIndex;
        
        /**
         * @brief Map of child GIE or TIS PrimaryInferBintrs for this Branch
         */
        std::map <std::string, DSL_PRIMARY_INFER_PTR> m_pPrimaryInferBintrs;
        
        /**
         * @brief Map of child GIE or TIS PrimaryInferBintrs for this Branch
         * indexed by thier add-order for execution
         */
        std::map <uint, DSL_PRIMARY_INFER_PTR> m_pPrimaryInferBintrsIndexed;
        
        /**
         * @brief optional, one or more Secondary GIEs for this Branch
         */
        DSL_PIPELINE_SINFERS_PTR m_pSecondaryInfersBintr;

        /**
         * @brief optional, one at most Segmentation Visualizater for this Branch
         */
        DSL_SEGVISUAL_PTR m_pSegVisualBintr;
        
        /**
         * @brief optional, one at most Tracker for this Branch
         */
        DSL_TRACKER_PTR m_pTrackerBintr;

        /**
         * @brief optional, one at most Optical Flow Fisualizer for this Branch
         */
        DSL_OFV_PTR m_pOfvBintr;

        /**
         * @brief optional, one at most OSD for this Branch
         */
        DSL_OSD_PTR m_pOsdBintr;
                        
        /**
         * @brief optional, one at most Tiled Display mutually exclusive 
         * with the DemuxerBintr, however, a Branch must have one or the other
         */
        DSL_TILER_PTR m_pTilerBintr;
                        
        /**
         * @brief optional, one at most StreamStreamDemuxerBintr mutually exclusive 
         * with the TilerBintr, however, a Pipeline must have one or the other.
         */
        DSL_DEMUXER_PTR m_pDemuxerBintr;
        
        /**
         * @brief optional, one at most SplitterDemuxerBintr mutually exclusive 
         * with the TilerBintr and MultiSinkBintr.
         */
        DSL_SPLITTER_PTR m_pSplitterBintr;
        
        /**
         * @brief parent bin for all Sink bins in this Branch
         */
        DSL_MULTI_SINKS_PTR m_pMultiSinksBintr;
        
        
    }; // Branch
    
} // Namespace

#endif // _DSL_BRANCH_H
