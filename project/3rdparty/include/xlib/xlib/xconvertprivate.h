#ifndef __X_CONVERTPRIVATE_H__
#define __X_CONVERTPRIVATE_H__

X_BEGIN_DECLS

#include "../xlib.h"

xchar *_x_time_locale_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;
xchar *_x_ctype_locale_to_utf8(const xchar *opsysstring, xssize len, xsize *bytes_read, xsize *bytes_written, XError **error) X_GNUC_MALLOC;

X_END_DECLS

#endif
