#ifndef __X_RCBOX_H__
#define __X_RCBOX_H__

#include "xmem.h"
#include "xlib-typeof.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_2_58
xpointer x_rc_box_alloc(xsize block_size) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_2_58
xpointer x_rc_box_alloc0(xsize block_size) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_2_58
xpointer x_rc_box_dup(xsize block_size, xconstpointer mem_block) X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_2_58
xpointer x_rc_box_acquire(xpointer mem_block);

XLIB_AVAILABLE_IN_2_58
void x_rc_box_release(xpointer mem_block);

XLIB_AVAILABLE_IN_2_58
void x_rc_box_release_full(xpointer mem_block, XDestroyNotify clear_func);

XLIB_AVAILABLE_IN_2_58
xsize x_rc_box_get_size(xpointer mem_block);

XLIB_AVAILABLE_IN_2_58
xpointer x_atomic_rc_box_alloc(xsize block_size) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_2_58
xpointer x_atomic_rc_box_alloc0(xsize block_size) X_GNUC_MALLOC X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_2_58
xpointer x_atomic_rc_box_dup(xsize block_size, xconstpointer mem_block) X_GNUC_ALLOC_SIZE(1);

XLIB_AVAILABLE_IN_2_58
xpointer x_atomic_rc_box_acquire(xpointer mem_block);

XLIB_AVAILABLE_IN_2_58
void x_atomic_rc_box_release(xpointer mem_block);

XLIB_AVAILABLE_IN_2_58
void x_atomic_rc_box_release_full(xpointer mem_block, XDestroyNotify clear_func);

XLIB_AVAILABLE_IN_2_58
xsize x_atomic_rc_box_get_size(xpointer mem_block);

#define x_rc_box_new(type)                          ((type *)x_rc_box_alloc(sizeof(type)))
#define x_rc_box_new0(type)                         ((type *)x_rc_box_alloc0(sizeof(type)))
#define x_atomic_rc_box_new(type)                   ((type *)x_atomic_rc_box_alloc(sizeof(type)))
#define x_atomic_rc_box_new0(type)                  ((type *)x_atomic_rc_box_alloc0(sizeof(type)))

#if defined(xlib_typeof)
#define x_rc_box_acquire(mem_block)                 ((xlib_typeof(mem_block))(x_rc_box_acquire)(mem_block))
#define x_atomic_rc_box_acquire(mem_block)          ((xlib_typeof(mem_block))(x_atomic_rc_box_acquire)(mem_block))

#define x_rc_box_dup(block_size, mem_block)         ((xlib_typeof(mem_block))(x_rc_box_dup)(block_size, mem_block))
#define x_atomic_rc_box_dup(block_size, mem_block)  ((xlib_typeof(mem_block))(x_atomic_rc_box_dup)(block_size, mem_block))
#endif

X_END_DECLS

#endif
