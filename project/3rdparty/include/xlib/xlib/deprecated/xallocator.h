#ifndef __X_ALLOCATOR_H__
#define __X_ALLOCATOR_H__

#include "../xtypes.h"

X_BEGIN_DECLS

typedef struct _XMemChunk XMemChunk;
typedef struct _XAllocator XAllocator;

#define X_ALLOC_ONLY                    1
#define X_ALLOC_AND_FREE                2
#define X_ALLOCATOR_LIST                1
#define X_ALLOCATOR_SLIST               2
#define X_ALLOCATOR_NODE                3

#define x_chunk_new(type, chunk)        ((type *)x_mem_chunk_alloc(chunk))
#define x_chunk_new0(type, chunk)       ((type *)x_mem_chunk_alloc0(chunk))
#define x_chunk_free(mem, mem_chunk)    (x_mem_chunk_free(mem_chunk, mem))
#define x_mem_chunk_create(type, x, y)  (x_mem_chunk_new(NULL, sizeof (type), 0, 0))

XLIB_DEPRECATED
XMemChunk *x_mem_chunk_new(const xchar *name, xint atom_size, xsize area_size, xint type);

XLIB_DEPRECATED
void x_mem_chunk_destroy(XMemChunk *mem_chunk);

XLIB_DEPRECATED
xpointer x_mem_chunk_alloc(XMemChunk *mem_chunk);

XLIB_DEPRECATED
xpointer x_mem_chunk_alloc0(XMemChunk *mem_chunk);

XLIB_DEPRECATED
void x_mem_chunk_free(XMemChunk *mem_chunk, xpointer mem);

XLIB_DEPRECATED
void x_mem_chunk_clean(XMemChunk *mem_chunk);

XLIB_DEPRECATED
void x_mem_chunk_reset(XMemChunk *mem_chunk);

XLIB_DEPRECATED
void x_mem_chunk_print(XMemChunk *mem_chunk);

XLIB_DEPRECATED
void x_mem_chunk_info(void);

XLIB_DEPRECATED
void x_blow_chunks(void);

XLIB_DEPRECATED
XAllocator *x_allocator_new(const xchar *name, xuint n_preallocs);

XLIB_DEPRECATED
void x_allocator_free(XAllocator *allocator);

XLIB_DEPRECATED
void x_list_push_allocator(XAllocator *allocator);

XLIB_DEPRECATED
void x_list_pop_allocator(void);

XLIB_DEPRECATED
void x_slist_push_allocator(XAllocator *allocator);

XLIB_DEPRECATED
void x_slist_pop_allocator(void);

XLIB_DEPRECATED
void x_node_push_allocator(XAllocator *allocator);

XLIB_DEPRECATED
void x_node_pop_allocator(void);

X_END_DECLS

#endif
