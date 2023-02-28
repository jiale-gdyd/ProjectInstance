#ifndef __X_STRINGCHUNK_H__
#define __X_STRINGCHUNK_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XStringChunk XStringChunk;

XLIB_AVAILABLE_IN_ALL
XStringChunk *x_string_chunk_new(xsize size);

XLIB_AVAILABLE_IN_ALL
void x_string_chunk_free(XStringChunk *chunk);

XLIB_AVAILABLE_IN_ALL
void x_string_chunk_clear(XStringChunk *chunk);

XLIB_AVAILABLE_IN_ALL
xchar *x_string_chunk_insert(XStringChunk *chunk, const xchar *string);

XLIB_AVAILABLE_IN_ALL
xchar *x_string_chunk_insert_len(XStringChunk *chunk, const xchar *string, xssize len);

XLIB_AVAILABLE_IN_ALL
xchar *x_string_chunk_insert_const(XStringChunk *chunk, const xchar *string);

X_END_DECLS

#endif
