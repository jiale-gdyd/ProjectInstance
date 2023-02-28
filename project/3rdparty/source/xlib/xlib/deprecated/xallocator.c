#include <xlib/xlib/config.h>

#ifndef XLIB_DISABLE_DEPRECATION_WARNINGS
#define XLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/deprecated/xallocator.h>

struct _XMemChunk {
    xuint alloc_size;
};

XMemChunk *x_mem_chunk_new(const xchar *name, xint atom_size, xsize area_size, xint type)
{
    XMemChunk *mem_chunk;

    x_return_val_if_fail(atom_size > 0, NULL);

    mem_chunk = x_slice_new(XMemChunk);
    mem_chunk->alloc_size = atom_size;

    return mem_chunk;
}

void x_mem_chunk_destroy(XMemChunk *mem_chunk)
{
    x_return_if_fail(mem_chunk != NULL);
    x_slice_free(XMemChunk, mem_chunk);
}

xpointer x_mem_chunk_alloc(XMemChunk *mem_chunk)
{
    x_return_val_if_fail(mem_chunk != NULL, NULL);
    return x_slice_alloc(mem_chunk->alloc_size);
}

xpointer x_mem_chunk_alloc0(XMemChunk *mem_chunk)
{
    x_return_val_if_fail(mem_chunk != NULL, NULL);
    return x_slice_alloc0(mem_chunk->alloc_size);
}

void x_mem_chunk_free(XMemChunk *mem_chunk, xpointer mem)
{
    x_return_if_fail(mem_chunk != NULL);
    x_slice_free1(mem_chunk->alloc_size, mem);
}

XAllocator *x_allocator_new(const xchar *name, xuint n_preallocs)
{
    return (XAllocator *)1;
}

void x_allocator_free(XAllocator *allocator)
{

}

void x_mem_chunk_clean(XMemChunk *mem_chunk)
{

}

void x_mem_chunk_reset(XMemChunk *mem_chunk)
{

}

void x_mem_chunk_print(XMemChunk *mem_chunk)
{

}

void x_mem_chunk_info(void)
{

}

void x_blow_chunks(void)
{

}

void x_list_push_allocator(XAllocator *allocator)
{

}

void x_list_pop_allocator(void)
{

}

void x_slist_push_allocator(XAllocator *allocator)
{

}

void x_slist_pop_allocator(void)
{

}

void x_node_push_allocator(XAllocator *allocator)
{

}

void x_node_pop_allocator(void)
{

}
