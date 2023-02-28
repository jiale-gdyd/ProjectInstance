#include <string.h>
#include <xlib/xlib/config.h>

#include <xlib/xlib/xrcbox.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xlib_trace.h>
#include <xlib/xlib/xrcboxprivate.h>

#define ALIGN_STRUCT(offset)            ((offset + (STRUCT_ALIGNMENT - 1)) & -STRUCT_ALIGNMENT)
#define X_RC_BOX(p)                     (XRcBox *)(((char *)(p)) - X_RC_BOX_SIZE)

xpointer x_rc_box_alloc_full(xsize block_size, xsize alignment, xboolean atomic, xboolean clear)
{
    xsize real_size;
    char *allocated;
    xsize private_offset = 0;
    xsize private_size = X_ARC_BOX_SIZE;

    x_assert(alignment != 0);

    if (private_size % alignment != 0) {
        private_offset = private_size % alignment;
        private_size += (alignment - private_offset);
    }

    x_assert(block_size < (X_MAXSIZE - private_size));
    real_size = private_size + block_size;

    if (real_size % alignment != 0) {
        xsize offset = real_size % alignment;
        x_assert(real_size < (X_MAXSIZE - (alignment - offset)));
        real_size += (alignment - offset);
    }

    {
        if (clear) {
            allocated = (char *)x_malloc0(real_size);
        } else {
            allocated = (char *)x_malloc(real_size);
        }
    }

    if (atomic) {
        XArcBox *real_box = (XArcBox *)(allocated + private_offset);
        real_box->mem_size = block_size;
        real_box->private_offset = private_offset;
        real_box->magic = X_BOX_MAGIC;
        x_atomic_ref_count_init(&real_box->ref_count);
    } else {
        XRcBox *real_box = (XRcBox *) (allocated + private_offset);
        real_box->mem_size = block_size;
        real_box->private_offset = private_offset;
        real_box->magic = X_BOX_MAGIC;
        x_ref_count_init(&real_box->ref_count);
    }

    TRACE(XLIB_RCBOX_ALLOC(allocated, block_size, atomic, clear));
    return allocated + private_size;
}

xpointer x_rc_box_alloc(xsize block_size)
{
    x_return_val_if_fail(block_size > 0, NULL);
    return x_rc_box_alloc_full(block_size, STRUCT_ALIGNMENT, FALSE, FALSE);
}

xpointer x_rc_box_alloc0(xsize block_size)
{
    x_return_val_if_fail(block_size > 0, NULL);
    return x_rc_box_alloc_full(block_size, STRUCT_ALIGNMENT, FALSE, TRUE);
}

xpointer (x_rc_box_dup)(xsize block_size, xconstpointer mem_block)
{
    xpointer res;

    x_return_val_if_fail(block_size > 0, NULL);
    x_return_val_if_fail(mem_block != NULL, NULL);

    res = x_rc_box_alloc_full(block_size, STRUCT_ALIGNMENT, FALSE, FALSE);
    memcpy(res, mem_block, block_size);

    return res;
}

xpointer (x_rc_box_acquire)(xpointer mem_block)
{
    XRcBox *real_box = X_RC_BOX(mem_block);

    x_return_val_if_fail(mem_block != NULL, NULL);
    x_return_val_if_fail(real_box->magic == X_BOX_MAGIC, NULL);

    x_ref_count_inc(&real_box->ref_count);
    TRACE(XLIB_RCBOX_ACQUIRE(mem_block, 0));

    return mem_block;
}

void x_rc_box_release(xpointer mem_block)
{
    x_rc_box_release_full(mem_block, NULL);
}

void x_rc_box_release_full(xpointer mem_block, XDestroyNotify clear_func)
{
    XRcBox *real_box = X_RC_BOX(mem_block);

    x_return_if_fail(mem_block != NULL);
    x_return_if_fail(real_box->magic == X_BOX_MAGIC);

    if (x_ref_count_dec(&real_box->ref_count)) {
        char *real_mem = (char *)real_box - real_box->private_offset;

        TRACE(XLIB_RCBOX_RELEASE(mem_block, 0));

        if (clear_func != NULL) {
            clear_func (mem_block);
        }

        TRACE(XLIB_RCBOX_FREE(mem_block));
        x_free(real_mem);
    }
}

xsize x_rc_box_get_size(xpointer mem_block)
{
    XRcBox *real_box = X_RC_BOX(mem_block);

    x_return_val_if_fail(mem_block != NULL, 0);
    x_return_val_if_fail(real_box->magic == X_BOX_MAGIC, 0);

    return real_box->mem_size;
}
