#ifndef __X_PATHBUF_H__
#define __X_PATHBUF_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XPathBuf XPathBuf;

struct _XPathBuf {
    xpointer dummy[8];
};

#define X_PATH_BUF_INIT                 { { NULL, } } XLIB_AVAILABLE_MACRO_IN_2_76

XLIB_AVAILABLE_IN_2_76
XPathBuf *x_path_buf_new(void);

XLIB_AVAILABLE_IN_2_76
XPathBuf *x_path_buf_new_from_path(const char *path);

XLIB_AVAILABLE_IN_2_76
XPathBuf *x_path_buf_init(XPathBuf *buf);

XLIB_AVAILABLE_IN_2_76
XPathBuf *x_path_buf_init_from_path(XPathBuf *buf, const char *path);

XLIB_AVAILABLE_IN_2_76
void x_path_buf_clear(XPathBuf *buf);

XLIB_AVAILABLE_IN_2_76
char *x_path_buf_clear_to_path(XPathBuf *buf) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_76
void x_path_buf_free(XPathBuf *buf);

XLIB_AVAILABLE_IN_2_76
char *x_path_buf_free_to_path(XPathBuf *buf) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_76
XPathBuf *x_path_buf_copy(XPathBuf *buf);

XLIB_AVAILABLE_IN_2_76
XPathBuf *x_path_buf_push(XPathBuf *buf, const char *path);

XLIB_AVAILABLE_IN_2_76
xboolean x_path_buf_pop(XPathBuf *buf);

XLIB_AVAILABLE_IN_2_76
xboolean x_path_buf_set_filename(XPathBuf *buf, const char *file_name);

XLIB_AVAILABLE_IN_2_76
xboolean x_path_buf_set_extension(XPathBuf *buf, const char *extension);

XLIB_AVAILABLE_IN_2_76
char *x_path_buf_to_path(XPathBuf *buf) X_GNUC_WARN_UNUSED_RESULT;

XLIB_AVAILABLE_IN_2_76
xboolean x_path_buf_equal(xconstpointer v1, xconstpointer v2);

X_END_DECLS

#endif
