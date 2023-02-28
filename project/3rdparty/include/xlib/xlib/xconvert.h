#ifndef __X_CONVERT_H__
#define __X_CONVERT_H__

#include "xerror.h"
#include "xtypes.h"

X_BEGIN_DECLS

typedef enum {
    X_CONVERT_ERROR_NO_CONVERSION,
    X_CONVERT_ERROR_ILLEGAL_SEQUENCE,
    X_CONVERT_ERROR_FAILED,
    X_CONVERT_ERROR_PARTIAL_INPUT,
    X_CONVERT_ERROR_BAD_URI,
    X_CONVERT_ERROR_NOT_ABSOLUTE_PATH,
    X_CONVERT_ERROR_NO_MEMORY,
    X_CONVERT_ERROR_EMBEDDED_NUL
} XConvertError;

#define X_CONVERT_ERROR         x_convert_error_quark()

typedef struct _XIConv *XIConv;

XLIB_AVAILABLE_IN_ALL
XQuark x_convert_error_quark(void);

XLIB_AVAILABLE_IN_ALL
XIConv x_iconv_open(const xchar *to_codeset, const xchar *from_codeset);

XLIB_AVAILABLE_IN_ALL
xsize x_iconv(XIConv converter, xchar **inbuf, xsize *inbytes_left, xchar **outbuf, xsize *outbytes_left);

XLIB_AVAILABLE_IN_ALL
xint x_iconv_close(XIConv converter);

XLIB_AVAILABLE_IN_ALL
xchar *x_convert(const xchar *str, xssize len, const xchar *to_codeset, const xchar *from_codeset, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_convert_with_iconv(const xchar *str, xssize len, XIConv converter, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_convert_with_fallback(const xchar *str, xssize len, const xchar *to_codeset, const xchar *from_codeset, const xchar *fallback, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_locale_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_locale_from_utf8(const xchar *utf8string, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_filename_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_filename_from_utf8(const xchar *utf8string, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_filename_from_uri(const xchar *uri, xchar **hostname, XError **error) X_GNUC_MALLOC;
  
XLIB_AVAILABLE_IN_ALL
xchar *x_filename_to_uri(const xchar *filename, const xchar *hostname, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_filename_display_name(const xchar *filename) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xboolean x_get_filename_charsets(const xchar ***filename_charsets);

XLIB_AVAILABLE_IN_ALL
xchar *x_filename_display_basename(const xchar *filename) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar **x_uri_list_extract_uris(const xchar *uri_list);

X_END_DECLS

#endif
