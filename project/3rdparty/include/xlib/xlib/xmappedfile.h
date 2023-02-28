#ifndef __X_MAPPED_FILE_H__
#define __X_MAPPED_FILE_H__

#include "xbytes.h"
#include "xerror.h"

X_BEGIN_DECLS

typedef struct _XMappedFile XMappedFile;

XLIB_AVAILABLE_IN_ALL
XMappedFile *x_mapped_file_new(const xchar *filename, xboolean writable, XError **error);

XLIB_AVAILABLE_IN_ALL
XMappedFile *x_mapped_file_new_from_fd(xint fd, xboolean writable, XError **error);

XLIB_AVAILABLE_IN_ALL
xsize x_mapped_file_get_length(XMappedFile *file);

XLIB_AVAILABLE_IN_ALL
xchar *x_mapped_file_get_contents(XMappedFile *file);

XLIB_AVAILABLE_IN_2_34
XBytes *x_mapped_file_get_bytes(XMappedFile  *file);

XLIB_AVAILABLE_IN_ALL
XMappedFile *x_mapped_file_ref(XMappedFile  *file);

XLIB_AVAILABLE_IN_ALL
void x_mapped_file_unref(XMappedFile  *file);

XLIB_DEPRECATED_FOR(x_mapped_file_unref)
void x_mapped_file_free(XMappedFile *file);

X_END_DECLS

#endif
