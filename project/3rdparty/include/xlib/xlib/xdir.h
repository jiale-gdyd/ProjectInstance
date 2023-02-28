#ifndef __X_DIR_H__
#define __X_DIR_H__

#include "xerror.h"
#include <dirent.h>

X_BEGIN_DECLS

typedef struct _XDir XDir;

XLIB_AVAILABLE_IN_ALL
XDir *x_dir_open(const xchar *path, xuint flags, XError **error);

XLIB_AVAILABLE_IN_ALL
const xchar *x_dir_read_name(XDir *dir);

XLIB_AVAILABLE_IN_ALL
void x_dir_rewind(XDir *dir);

XLIB_AVAILABLE_IN_ALL
void x_dir_close(XDir *dir);

X_END_DECLS

#endif
