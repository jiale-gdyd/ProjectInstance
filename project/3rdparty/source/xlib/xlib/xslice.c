#include <stdio.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>
#include <xlib/xlib/xmem.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xprintf.h>
#include <xlib/xlib/xlib_trace.h>

void x_slice_set_config(XSliceConfig ckey, xint64 value)
{

}

xint64 x_slice_get_config(XSliceConfig ckey)
{
    return 0;
}

xint64 *x_slice_get_config_state(XSliceConfig ckey, xint64 address, xuint *n_values)
{
    return NULL;
}

xpointer x_slice_alloc(xsize mem_size)
{
    xpointer mem = x_malloc(mem_size);
    TRACE(XLIB_SLICE_ALLOC((void *)mem, mem_size));
    return mem;
}

xpointer x_slice_alloc0(xsize mem_size)
{
    xpointer mem = x_slice_alloc(mem_size);
    if (mem) {
        memset(mem, 0, mem_size);
    }

    return mem;
}

xpointer x_slice_copy(xsize mem_size, xconstpointer mem_block)
{
    xpointer mem = x_slice_alloc(mem_size);
    if (mem) {
        memcpy(mem, mem_block, mem_size);
    }

    return mem;
}

void x_slice_free1(xsize mem_size, xpointer mem_block)
{
    if (X_UNLIKELY(x_mem_gc_friendly && mem_block)) {
        memset(mem_block, 0, mem_size);
    }

    x_free_sized(mem_block, mem_size);
    TRACE(XLIB_SLICE_FREE((void *)mem_block, mem_size));
}

void x_slice_free_chain_with_offset(xsize mem_size, xpointer mem_chain, xsize next_offset)
{
    xpointer slice = mem_chain;
    while (slice) {
        xuint8 *current = slice;
        slice = *(xpointer *)(current + next_offset);
        if (X_UNLIKELY(x_mem_gc_friendly)) {
            memset(current, 0, mem_size);
        }

        x_free_sized(current, mem_size);
    }
}
