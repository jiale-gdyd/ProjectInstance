#ifndef __X_RCBOX_PRIVATE_H__
#define __X_RCBOX_PRIVATE_H__

#include "xtypes.h"
#include "xrcbox.h"

X_BEGIN_DECLS

typedef struct {
    xrefcount ref_count;
    xsize     mem_size;
    xsize     private_offset;
    xuint32   magic;
} XRcBox;

typedef struct {
    xatomicrefcount ref_count;
    xsize           mem_size;
    xsize           private_offset;
    xuint32         magic;
} XArcBox;

#define X_BOX_MAGIC             0x44ae2bf0

X_STATIC_ASSERT(sizeof(XRcBox) == sizeof(XArcBox));

#define STRUCT_ALIGNMENT        (2 * sizeof (xsize))

#define X_RC_BOX_SIZE           sizeof(XRcBox)
#define X_ARC_BOX_SIZE          sizeof(XArcBox)

xpointer x_rc_box_alloc_full(xsize block_size, xsize alignment, xboolean atomic, xboolean clear);

X_END_DECLS

#endif