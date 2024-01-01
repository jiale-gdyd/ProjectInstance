#ifndef __XLIB_TYPES_H__
#define __XLIB_TYPES_H__

#include "../xlib.h"

X_BEGIN_DECLS

#ifdef __XI_SCANNER__
typedef xsize XType;
#endif

#define X_TYPE_DATE                         (x_date_get_type())
#define X_TYPE_STRV                         (x_strv_get_type())
#define X_TYPE_GSTRING                      (x_gstring_get_type())
#define X_TYPE_HASH_TABLE                   (x_hash_table_get_type())
#define X_TYPE_REGEX                        (x_regex_get_type())
#define X_TYPE_MATCH_INFO                   (x_match_info_get_type())
#define X_TYPE_ARRAY                        (x_array_get_type())
#define X_TYPE_BYTE_ARRAY                   (x_byte_array_get_type())
#define X_TYPE_PTR_ARRAY                    (x_ptr_array_get_type())
#define X_TYPE_BYTES                        (x_bytes_get_type())
#define X_TYPE_VARIANT_TYPE                 (x_variant_type_get_gtype())
#define X_TYPE_ERROR                        (x_error_get_type())
#define X_TYPE_DATE_TIME                    (x_date_time_get_type())
#define X_TYPE_TIME_ZONE                    (x_time_zone_get_type())
#define X_TYPE_IO_CHANNEL                   (x_io_channel_get_type())
#define X_TYPE_IO_CONDITION                 (x_io_condition_get_type())
#define X_TYPE_VARIANT_BUILDER              (x_variant_builder_get_type())
#define X_TYPE_VARIANT_DICT                 (x_variant_dict_get_type())
#define X_TYPE_MAIN_LOOP                    (x_main_loop_get_type())
#define X_TYPE_MAIN_CONTEXT                 (x_main_context_get_type())
#define X_TYPE_SOURCE                       (x_source_get_type())
#define X_TYPE_POLLFD                       (x_pollfd_get_type())
#define X_TYPE_MARKUP_PARSE_CONTEXT         (x_markup_parse_context_get_type())
#define X_TYPE_KEY_FILE                     (x_key_file_get_type())
#define X_TYPE_MAPPED_FILE                  (x_mapped_file_get_type())
#define X_TYPE_THREAD                       (x_thread_get_type())
#define X_TYPE_CHECKSUM                     (x_checksum_get_type())
#define X_TYPE_OPTION_GROUP                 (x_option_group_get_type())
#define X_TYPE_URI                          (x_uri_get_type())
#define X_TYPE_TREE                         (x_tree_get_type())
#define X_TYPE_PATTERN_SPEC                 (x_pattern_spec_get_type())
#define X_TYPE_BOOKMARK_FILE                (x_bookmark_file_get_type())

#define X_TYPE_HMAC                         (x_hmac_get_type())
#define X_TYPE_DIR                          (x_dir_get_type())
#define X_TYPE_RAND                         (x_rand_get_type())
#define X_TYPE_STRV_BUILDER                 (x_strv_builder_get_type())

XLIB_AVAILABLE_IN_ALL
XType x_date_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_strv_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_gstring_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_hash_table_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_array_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_byte_array_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_ptr_array_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_bytes_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_variant_type_get_gtype(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_regex_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_30
XType x_match_info_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_error_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_date_time_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_time_zone_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_io_channel_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_io_condition_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_variant_builder_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_40
XType x_variant_dict_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XType x_key_file_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_30
XType x_main_loop_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_30
XType x_main_context_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_30
XType x_source_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_36
XType x_pollfd_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_36
XType x_thread_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_36
XType x_checksum_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_36
XType x_markup_parse_context_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_40
XType x_mapped_file_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_44
XType x_option_group_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_66
XType x_uri_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_68
XType x_tree_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_70
XType x_pattern_spec_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_76
XType x_bookmark_file_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_80
XType x_hmac_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_80
XType x_dir_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_80
XType x_rand_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_2_80
XType x_strv_builder_get_type(void) X_GNUC_CONST;

XLIB_DEPRECATED_FOR('X_TYPE_VARIANT')
XType x_variant_get_gtype(void) X_GNUC_CONST;

X_END_DECLS

#endif
