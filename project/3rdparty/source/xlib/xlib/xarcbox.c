#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xlib_trace.h>
#include <xlib/xlib/xrcboxprivate.h>

#define X_ARC_BOX(p)            (XArcBox *)(((char *) (p)) - X_ARC_BOX_SIZE)

xpointer x_atomic_rc_box_alloc(xsize block_size)
{
    x_return_val_if_fail(block_size > 0, NULL);
    return x_rc_box_alloc_full(block_size, STRUCT_ALIGNMENT, TRUE, FALSE);
}

xpointer x_atomic_rc_box_alloc0(xsize block_size)
{
    x_return_val_if_fail(block_size > 0, NULL);
    return x_rc_box_alloc_full(block_size, STRUCT_ALIGNMENT, TRUE, TRUE);
}

xpointer (x_atomic_rc_box_dup)(xsize block_size, xconstpointer mem_block)
{
    xpointer res;

    x_return_val_if_fail(block_size > 0, NULL);
    x_return_val_if_fail(mem_block != NULL, NULL);

    res = x_rc_box_alloc_full(block_size, STRUCT_ALIGNMENT, TRUE, FALSE);
    memcpy(res, mem_block, block_size);

    return res;
}

xpointer (x_atomic_rc_box_acquire)(xpointer mem_block)
{
    XArcBox *real_box = X_ARC_BOX(mem_block);

    x_return_val_if_fail(mem_block != NULL, NULL);
    x_return_val_if_fail(real_box->magic == X_BOX_MAGIC, NULL);

    x_atomic_ref_count_inc(&real_box->ref_count);
    TRACE(XLIB_RCBOX_ACQUIRE(mem_block, 1));

    return mem_block;
}

void x_atomic_rc_box_release(xpointer mem_block)
{
    x_atomic_rc_box_release_full(mem_block, NULL);
}

void x_atomic_rc_box_release_full(xpointer mem_block, XDestroyNotify clear_func)
{
    XArcBox *real_box = X_ARC_BOX(mem_block);

    x_return_if_fail(mem_block != NULL);
    x_return_if_fail(real_box->magic == X_BOX_MAGIC);

    if (x_atomic_ref_count_dec(&real_box->ref_count)) {
        char *real_mem = (char *)real_box - real_box->private_offset;

        TRACE(XLIB_RCBOX_RELEASE(mem_block, 1));

        if (clear_func != NULL) {
            clear_func(mem_block);
        }

        TRACE(XLIB_RCBOX_FREE(mem_block));
        x_free(real_mem);
    }
}

xsize x_atomic_rc_box_get_size(xpointer mem_block)
{
    XArcBox *real_box = X_ARC_BOX(mem_block);

    x_return_val_if_fail(mem_block != NULL, 0);
    x_return_val_if_fail(real_box->magic == X_BOX_MAGIC, 0);

    return real_box->mem_size;
}
