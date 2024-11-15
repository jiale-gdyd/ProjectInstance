/*
 *
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file       h265d_syntax.h
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */
#ifndef __H265D_SYNTAX__
#define __H265D_SYNTAX__

typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       ULONG;
typedef unsigned short      USHORT;
typedef unsigned char       UCHAR;
typedef unsigned int        UINT;
typedef unsigned int        UINT32;

typedef signed   int        BOOL;
typedef signed   int        INT;
typedef signed   char       CHAR;
typedef signed   short      SHORT;
typedef signed   long       LONG;
typedef void               *PVOID;

/* HEVC Picture Entry structure */
#define MAX_SLICES 600
typedef struct _DXVA_PicEntry_HEVC {
    union {
        struct {
            UCHAR Index7Bits : 7;
            UCHAR AssociatedFlag : 1;
        };
        UCHAR bPicEntry;
    };
} DXVA_PicEntry_HEVC, *LPDXVA_PicEntry_HEVC;

typedef struct _Short_SPS_RPS_HEVC {
    UCHAR num_negative_pics;
    UCHAR num_positive_pics;
    SHORT delta_poc_s0[16];
    UCHAR s0_used_flag[16];
    SHORT delta_poc_s1[16];
    UCHAR  s1_used_flag[16];
} Short_SPS_RPS_HEVC;

typedef struct _LT_SPS_RPS_HEVC {
    USHORT lt_ref_pic_poc_lsb;
    UCHAR  used_by_curr_pic_lt_flag;
} LT_SPS_RPS_HEVC;
/* HEVC Picture Parameter structure */
typedef struct _DXVA_PicParams_HEVC {
    USHORT      PicWidthInMinCbsY;
    USHORT      PicHeightInMinCbsY;
    union {
        struct {
            USHORT  chroma_format_idc                       : 2;
            USHORT  separate_colour_plane_flag              : 1;
            USHORT  bit_depth_luma_minus8                   : 3;
            USHORT  bit_depth_chroma_minus8                 : 3;
            USHORT  log2_max_pic_order_cnt_lsb_minus4       : 4;
            USHORT  NoPicReorderingFlag                     : 1;
            USHORT  NoBiPredFlag                            : 1;
            USHORT  ReservedBits1                            : 1;
        };
        USHORT wFormatAndSequenceInfoFlags;
    };
    DXVA_PicEntry_HEVC  CurrPic;
    UCHAR   sps_max_dec_pic_buffering_minus1;
    UCHAR   log2_min_luma_coding_block_size_minus3;
    UCHAR   log2_diff_max_min_luma_coding_block_size;
    UCHAR   log2_min_transform_block_size_minus2;
    UCHAR   log2_diff_max_min_transform_block_size;
    UCHAR   max_transform_hierarchy_depth_inter;
    UCHAR   max_transform_hierarchy_depth_intra;
    UCHAR   num_short_term_ref_pic_sets;
    UCHAR   num_long_term_ref_pics_sps;
    UCHAR   num_ref_idx_l0_default_active_minus1;
    UCHAR   num_ref_idx_l1_default_active_minus1;
    CHAR    init_qp_minus26;
    UCHAR   ucNumDeltaPocsOfRefRpsIdx;
    USHORT  wNumBitsForShortTermRPSInSlice;
    USHORT  ReservedBits2;

    union {
        struct {
            UINT32  scaling_list_enabled_flag                    : 1;
            UINT32  amp_enabled_flag                            : 1;
            UINT32  sample_adaptive_offset_enabled_flag         : 1;
            UINT32  pcm_enabled_flag                            : 1;
            UINT32  pcm_sample_bit_depth_luma_minus1            : 4;
            UINT32  pcm_sample_bit_depth_chroma_minus1          : 4;
            UINT32  log2_min_pcm_luma_coding_block_size_minus3  : 2;
            UINT32  log2_diff_max_min_pcm_luma_coding_block_size : 2;
            UINT32  pcm_loop_filter_disabled_flag                : 1;
            UINT32  long_term_ref_pics_present_flag             : 1;
            UINT32  sps_temporal_mvp_enabled_flag               : 1;
            UINT32  strong_intra_smoothing_enabled_flag         : 1;
            UINT32  dependent_slice_segments_enabled_flag       : 1;
            UINT32  output_flag_present_flag                    : 1;
            UINT32  num_extra_slice_header_bits                 : 3;
            UINT32  sign_data_hiding_enabled_flag               : 1;
            UINT32  cabac_init_present_flag                     : 1;
            UINT32  ReservedBits3                               : 5;
        };
        UINT32 dwCodingParamToolFlags;
    };

    union {
        struct {
            UINT32  constrained_intra_pred_flag                 : 1;
            UINT32  transform_skip_enabled_flag                 : 1;
            UINT32  cu_qp_delta_enabled_flag                    : 1;
            UINT32  pps_slice_chroma_qp_offsets_present_flag    : 1;
            UINT32  weighted_pred_flag                          : 1;
            UINT32  weighted_bipred_flag                        : 1;
            UINT32  transquant_bypass_enabled_flag              : 1;
            UINT32  tiles_enabled_flag                          : 1;
            UINT32  entropy_coding_sync_enabled_flag            : 1;
            UINT32  uniform_spacing_flag                        : 1;
            UINT32  loop_filter_across_tiles_enabled_flag       : 1;
            UINT32  pps_loop_filter_across_slices_enabled_flag  : 1;
            UINT32  deblocking_filter_override_enabled_flag     : 1;
            UINT32  pps_deblocking_filter_disabled_flag         : 1;
            UINT32  lists_modification_present_flag             : 1;
            UINT32  slice_segment_header_extension_present_flag : 1;
            UINT32  IrapPicFlag                                 : 1;
            UINT32  IdrPicFlag                                  : 1;
            UINT32  IntraPicFlag                                : 1;
            // sps exension flags
            UINT32  sps_extension_flag                          : 1;
            UINT32  sps_range_extension_flag                    : 1;
            UINT32  transform_skip_rotation_enabled_flag        : 1;
            UINT32  transform_skip_context_enabled_flag         : 1;
            UINT32  implicit_rdpcm_enabled_flag                 : 1;
            UINT32  explicit_rdpcm_enabled_flag                 : 1;
            UINT32  extended_precision_processing_flag          : 1;
            UINT32  intra_smoothing_disabled_flag               : 1;
            UINT32  high_precision_offsets_enabled_flag         : 1;
            UINT32  persistent_rice_adaptation_enabled_flag     : 1;
            UINT32  cabac_bypass_alignment_enabled_flag         : 1;
            // pps exension flags
            UINT32  cross_component_prediction_enabled_flag     : 1;
            UINT32  chroma_qp_offset_list_enabled_flag          : 1;
        };
        UINT32 dwCodingSettingPicturePropertyFlags;
    };
    CHAR    pps_cb_qp_offset;
    CHAR    pps_cr_qp_offset;
    UCHAR   num_tile_columns_minus1;
    UCHAR   num_tile_rows_minus1;
    USHORT  column_width_minus1[19];
    USHORT  row_height_minus1[21];
    UCHAR   diff_cu_qp_delta_depth;
    CHAR    pps_beta_offset_div2;
    CHAR    pps_tc_offset_div2;
    UCHAR   log2_parallel_merge_level_minus2;
    INT     CurrPicOrderCntVal;
    DXVA_PicEntry_HEVC  RefPicList[15];
    UCHAR   ReservedBits5;
    INT     PicOrderCntValList[15];
    UCHAR   RefPicSetStCurrBefore[8];
    UCHAR   RefPicSetStCurrAfter[8];
    UCHAR   RefPicSetLtCurr[8];
    USHORT  ReservedBits6;
    USHORT  ReservedBits7;
    UINT    StatusReportFeedbackNumber;
    UINT32 vps_id;
    UINT32 pps_id;
    UINT32 sps_id;
    INT    current_poc;

    Short_SPS_RPS_HEVC sps_st_rps[64];
    LT_SPS_RPS_HEVC    sps_lt_rps[32];

    // PPS exentison
    UCHAR log2_max_transform_skip_block_size;
    UCHAR diff_cu_chroma_qp_offset_depth;
    CHAR  cb_qp_offset_list[6];
    CHAR  cr_qp_offset_list[6];
    UCHAR chroma_qp_offset_list_len_minus1;

    UCHAR  scaling_list_data_present_flag;
    UCHAR  ps_update_flag;
} DXVA_PicParams_HEVC, *LPDXVA_PicParams_HEVC;

/* HEVC Quantizatiuon Matrix structure */
typedef struct _DXVA_Qmatrix_HEVC {
    UCHAR ucScalingLists0[6][16];
    UCHAR ucScalingLists1[6][64];
    UCHAR ucScalingLists2[6][64];
    UCHAR ucScalingLists3[2][64];
    UCHAR ucScalingListDCCoefSizeID2[6];
    UCHAR ucScalingListDCCoefSizeID3[2];
} DXVA_Qmatrix_HEVC, *LPDXVA_Qmatrix_HEVC;


/* HEVC Slice Control Structure */
typedef struct _DXVA_Slice_HEVC_Short {
    UINT    BSNALunitDataLocation;
    UINT    SliceBytesInBuffer;
    USHORT  wBadSliceChopping;
} DXVA_Slice_HEVC_Short, *LPDXVA_Slice_HEVC_Short;

/* just use in the case of pps->slice_header_extension_present_flag is 1 */
typedef struct _DXVA_Slice_HEVC_Cut_Param {
    UINT    start_bit;
    UINT    end_bit;
    USHORT  is_enable;
} DXVA_Slice_HEVC_Cut_Param, *LPDXVA_Slice_HEVC_Cut_Param;

typedef struct h265d_dxva2_picture_context {
    DXVA_PicParams_HEVC         pp;
    DXVA_Qmatrix_HEVC           qm;
    UINT32                      slice_count;
    DXVA_Slice_HEVC_Short       *slice_short;
    const UCHAR                 *bitstream;
    UINT32                      bitstream_size;
    DXVA_Slice_HEVC_Cut_Param   *slice_cut_param;
    INT                         max_slice_num;
} h265d_dxva2_picture_context_t;

#endif /*__H265D_SYNTAX__*/
