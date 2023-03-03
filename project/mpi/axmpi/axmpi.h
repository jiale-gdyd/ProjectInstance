#ifndef MPI_AXMPI_AXMPI_H
#define MPI_AXMPI_AXMPI_H

#if !defined(__AXERA_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <stdio.h>
#include <stdarg.h>

#include <ax_adec_api.h>
#include <ax_aenc_api.h>
#include <ax_audio_process.h>

#include <ax_base_type.h>
#include <ax_cipher_api.h>
#include <ax_buffer_tool.h>
#include <ax_channel_api.h>

#include <ax_comm_aio.h>
#include <ax_comm_vdec.h>
#include <ax_comm_venc.h>
#include <ax_comm_codec.h>
#include <ax_comm_venc_rc.h>

#include <ax_dl_api.h>
#include <ax_ext_ipt.h>
#include <ax_efuse_api.h>

#include <ax_global_type.h>

#include <ax_interpreter_external_api.h>
#include <ax_interpreter_external2_api.h>
#include <ax_interpreter_external_advanced_api.h>

#include <ax_isp_3a_api.h>
#include <ax_isp_3a_plus.h>
#include <ax_isp_3a_struct.h>

#include <ax_isp_api.h>
#include <ax_isp_debug.h>
#include <ax_isp_common.h>
#include <ax_isp_iq_api.h>
#include <ax_isp_error_code.h>

#include <ax_ives_api.h>
#include <ax_ivps_api.h>
#include <ax_ivps_type.h>

#include <ax_lens_af_struct.h>
#include <ax_lens_iris_struct.h>

#include <ax_mipi_api.h>

#include <ax_npu_imgproc.h>

#include <ax_nt_ctrl_api.h>
#include <ax_nt_stream_api.h>

#include <ax_sensor_struct.h>

#include <ax_sys_api.h>
#include <ax_sys_log.h>
#include <ax_sys_dma_api.h>

#include <ax_vo_api.h>
#include <ax_vin_api.h>
#include <ax_vdec_api.h>
#include <ax_venc_api.h>
#include <ax_vin_error_code.h>

#include <base_types.h>
#include <img_helper.h>

#include <joint.h>
#include <joint_adv.h>
#include <joint_shortcut.h>

#include <log_trace.h>

#include <mcv_macro.h>
#include <mcv_search.h>
#include <npu_common.h>
#include <mcv_compare.h>
#include <mcv_interface.h>
#include <mcv_config_param.h>
#include <mcv_common_struct.h>

#include <tinyalsa/plugin.h>
#include <tinyalsa/asoundlib.h>
#include <tinyalsa/attributes.h>

#ifndef axmpi_print
#define axmpi_print(msg, ...)               fprintf(stderr, msg, ##__VA_ARGS__);
#endif

#ifndef axmpi_error
#define axmpi_error(msg, ...)               axmpi_print("\033[1;31m[AXMPI][E]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axmpi_warn
#define axmpi_warn(msg, ...)                axmpi_print("\033[1;33m[AXMPI][W]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axmpi_info
#define axmpi_info(msg, ...)                axmpi_print("\033[1;32m[AXMPI][I]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#ifndef axmpi_debug
#define axmpi_debug(msg, ...)               axmpi_print("\033[1;34m[AXMPI][D]: " msg "\033[0m\n", ##__VA_ARGS__)
#endif

#endif
