#ifndef __HAL_H265E_VEPU580_PRIVATE_H__
#define __HAL_H265E_VEPU580_PRIVATE_H__

#include <string.h>
#include <stdlib.h>

#include "../../../../osal/inc/mpp_env.h"
#include "../../../../osal/inc/mpp_mem.h"
#include "../../../../osal/inc/mpp_soc.h"
#include "../../../../osal/inc/mpp_common.h"
#include "../../../base/inc/mpp_frame_impl.h"
#include "../../../base/inc/mpp_packet_impl.h"

#include "../../common/h265/hal_h265e_debug.h"
#include "../../../common/h265e_syntax_new.h"
#include "../../common/hal_bufs.h"
#include "../common/rkv_enc_def.h"
#include "../../../codec/enc/h265/h265e_dpb.h"
#include "../common/vepu541_common.h"
#include "hal_h265e_vepu580.h"
#include "hal_h265e_vepu580_reg.h"
#include "../../../base/inc/mpp_enc_cb_param.h"

#include "../../../../osal/inc/mpp_service.h"

#define MAX_FRAME_TASK_NUM      2
#define MAX_TILE_NUM            4
#define MAX_REGS_SET            ((MAX_FRAME_TASK_NUM) * (MAX_TILE_NUM))

typedef struct vepu580_h265_fbk_t {
    RK_U32 hw_status; /* 0:corret, 1:error */
    RK_U32 qp_sum;
    RK_U32 out_strm_size;
    RK_U32 out_hw_strm_size;
    RK_S64 sse_sum;
    RK_U32 st_lvl64_inter_num;
    RK_U32 st_lvl32_inter_num;
    RK_U32 st_lvl16_inter_num;
    RK_U32 st_lvl8_inter_num;
    RK_U32 st_lvl32_intra_num;
    RK_U32 st_lvl16_intra_num;
    RK_U32 st_lvl8_intra_num;
    RK_U32 st_lvl4_intra_num;
    RK_U32 st_cu_num_qp[52];
    RK_U32 st_madp;
    RK_U32 st_madi;
    RK_U32 st_md_sad_b16num0;
    RK_U32 st_md_sad_b16num1;
    RK_U32 st_md_sad_b16num2;
    RK_U32 st_md_sad_b16num3;
    RK_U32 st_madi_b16num0;
    RK_U32 st_madi_b16num1;
    RK_U32 st_madi_b16num2;
    RK_U32 st_madi_b16num3;
    RK_U32 st_mb_num;
    RK_U32 st_ctu_num;
} Vepu580H265Fbk;

typedef struct Vepu580RoiH265BsCfg_t {
    RK_U8 amv_en        : 1;
    RK_U8 qp_adj        : 1;
    RK_U8 force_split   : 1;
    RK_U8 force_intra   : 2;
    RK_U8 force_inter   : 2;
} Vepu580RoiH265BsCfg;

typedef struct Vepu580H265eFrmCfg_t {
    RK_S32              frame_count;
    RK_S32              frame_type;

    /* dchs cfg on frame parallel */
    RK_S32              dchs_curr_idx;
    RK_S32              dchs_prev_idx;

    /* hal dpb management slot idx */
    RK_S32              hal_curr_idx;
    RK_S32              hal_refr_idx;

    /* regs cfg */
    H265eV580RegSet     *regs_set[MAX_TILE_NUM];
    H265eV580StatusElem *regs_ret[MAX_TILE_NUM];

    /* hardware return info collection cfg */
    Vepu580H265Fbk      feedback;

    /* tile buffer */
    MppBuffer           hw_tile_buf[MAX_TILE_NUM];
    MppBuffer           hw_tile_stream[MAX_TILE_NUM - 1];

    /* osd cfg */
    Vepu541OsdCfg       osd_cfg;
    void                *roi_data;

    /* gdr roi cfg */
    MppBuffer           roi_base_cfg_buf;
    void                *roi_base_cfg_sw_buf;
    RK_S32              roi_base_buf_size;

    /* variable length cfg */
    MppDevRegOffCfgs    *reg_cfg;
} Vepu580H265eFrmCfg;

typedef struct H265eV580HalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    Vepu580H265eFrmCfg  *frms[MAX_FRAME_TASK_NUM];

    /* current used frame config */
    Vepu580H265eFrmCfg  *frm;

    /* slice split poll cfg */
    RK_S32              poll_slice_max;
    RK_S32              poll_cfg_size;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_count;

    /* frame parallel info */
    RK_S32              task_cnt;
    RK_S32              task_idx;

    /* dchs cfg */
    RK_S32              curr_idx;
    RK_S32              prev_idx;

    /* debug cfg */
    void                *dump_files;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    MppBufferGroup      roi_grp;

    MppEncCfgSet        *cfg;
    H265eSyntax_new     *syn;
    H265eDpb            *dpb;

    /* single frame tile parallel info */
    MppBufferGroup      tile_grp;
    RK_U32              tile_num;
    RK_U32              tile_parall_en;
    RK_U32              tile_dump_err;

    MppBuffer           buf_pass1;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_S32              max_buf_cnt;
    RK_S32              hdr_status;
    void                *input_fmt;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    HalBufs             dpb_bufs;
    RK_S32              fbc_header_len;

    MppDevPollCfg       *poll_cfgs;
    MppCbCtx            *output_cb;

    /* finetune */
    void                *tune;
} H265eV580HalContext;

#define TILE_BUF_SIZE  MPP_ALIGN(128 * 1024, 256)

#endif
