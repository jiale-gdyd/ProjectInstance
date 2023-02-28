#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xvariant-internal.h>
#include <xlib/xlib/xvariant-serialiser.h>

xboolean x_variant_serialised_check(XVariantSerialised serialised)
{
    xuint alignment;
    xsize fixed_size;

    if (serialised.type_info == NULL) {
        return FALSE;
    }

    x_variant_type_info_query(serialised.type_info, &alignment, &fixed_size);

    if (fixed_size != 0 && serialised.size != fixed_size) {
        return FALSE;
    } else if (fixed_size == 0 && !(serialised.size == 0 || serialised.data != NULL)) {
        return FALSE;
    }

    if (serialised.ordered_offsets_up_to > serialised.checked_offsets_up_to) {
        return FALSE;
    }

    typedef struct _stLoalType { char a; union { xuint64 x; void *y; xdouble z; } b; } st_type_local;

    alignment &= sizeof(st_type_local) - 9;
    return (serialised.size <= alignment || (alignment & (xsize) serialised.data) == 0);
}

static xsize gvs_fixed_sized_maybe_n_children(XVariantSerialised value)
{
    xsize element_fixed_size;
    x_variant_type_info_query_element(value.type_info, NULL, &element_fixed_size);

    return (element_fixed_size == value.size) ? 1 : 0;
}

static XVariantSerialised gvs_fixed_sized_maybe_get_child(XVariantSerialised value, xsize index_)
{
    value.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_ref(value.type_info);
    value.depth++;
    value.ordered_offsets_up_to = 0;
    value.checked_offsets_up_to = 0;

    return value;
}

static xsize gvs_fixed_sized_maybe_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    if (n_children) {
        xsize element_fixed_size;
        x_variant_type_info_query_element(type_info, NULL, &element_fixed_size);

        return element_fixed_size;
    } else {
        return 0;
    }
}

static void gvs_fixed_sized_maybe_serialise(XVariantSerialised value, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    if (n_children) {
        XVariantSerialised child = { NULL, value.data, value.size, value.depth + 1, 0, 0 };
        gvs_filler(&child, children[0]);
    }
}

static xboolean gvs_fixed_sized_maybe_is_normal(XVariantSerialised value)
{
    if (value.size > 0) {
        xsize element_fixed_size;

        x_variant_type_info_query_element(value.type_info, NULL, &element_fixed_size);
        if (value.size != element_fixed_size) {
            return FALSE;
        }

        value.type_info = x_variant_type_info_element(value.type_info);
        value.depth++;
        value.ordered_offsets_up_to = 0;
        value.checked_offsets_up_to = 0;

        return x_variant_serialised_is_normal(value);
    }

    return TRUE;
}

static xsize gvs_variable_sized_maybe_n_children(XVariantSerialised value)
{
    return (value.size > 0) ? 1 : 0;
}

static XVariantSerialised gvs_variable_sized_maybe_get_child(XVariantSerialised value, xsize index_)
{
    value.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_ref(value.type_info);
    value.size--;

    if (value.size == 0) {
        value.data = NULL;
    }

    value.depth++;
    value.ordered_offsets_up_to = 0;
    value.checked_offsets_up_to = 0;

    return value;
}

static xsize gvs_variable_sized_maybe_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    if (n_children) {
        XVariantSerialised child = { 0, };
        gvs_filler(&child, children[0]);

        return child.size + 1;
    } else {
        return 0;
    }
}

static void gvs_variable_sized_maybe_serialise(XVariantSerialised value, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    if (n_children) {
        XVariantSerialised child = { NULL, value.data, value.size - 1, value.depth + 1, 0, 0 };

        gvs_filler(&child, children[0]);
        value.data[child.size] = '\0';
    }
}

static xboolean gvs_variable_sized_maybe_is_normal(XVariantSerialised value)
{
    if (value.size == 0) {
        return TRUE;
    }

    if (value.data[value.size - 1] != '\0') {
        return FALSE;
    }

    value.type_info = x_variant_type_info_element(value.type_info);
    value.size--;
    value.depth++;
    value.ordered_offsets_up_to = 0;
    value.checked_offsets_up_to = 0;

    return x_variant_serialised_is_normal(value);
}

static xsize gvs_fixed_sized_array_n_children(XVariantSerialised value)
{
    xsize element_fixed_size;
    x_variant_type_info_query_element(value.type_info, NULL, &element_fixed_size);

    if (value.size % element_fixed_size == 0) {
        return value.size / element_fixed_size;
    }

    return 0;
}

static XVariantSerialised gvs_fixed_sized_array_get_child(XVariantSerialised value, xsize index_)
{
    XVariantSerialised child = { 0, };

    child.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_query(child.type_info, NULL, &child.size);
    child.data = value.data + (child.size * index_);
    x_variant_type_info_ref(child.type_info);
    child.depth = value.depth + 1;

    return child;
}

static xsize gvs_fixed_sized_array_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    xsize element_fixed_size;
    x_variant_type_info_query_element(type_info, NULL, &element_fixed_size);

    return element_fixed_size * n_children;
}

static void gvs_fixed_sized_array_serialise(XVariantSerialised value, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    xsize i;
    XVariantSerialised child = { 0, };

    child.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_query(child.type_info, NULL, &child.size);
    child.data = value.data;
    child.depth = value.depth + 1;

    for (i = 0; i < n_children; i++) {
        gvs_filler(&child, children[i]);
        child.data += child.size;
    }
}

static xboolean gvs_fixed_sized_array_is_normal(XVariantSerialised value)
{
    XVariantSerialised child = { 0, };

    child.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_query(child.type_info, NULL, &child.size);
    child.depth = value.depth + 1;

    if (value.size % child.size != 0) {
        return FALSE;
    }

    for (child.data = value.data; child.data < value.data + value.size; child.data += child.size){
        if (!x_variant_serialised_is_normal(child)) {
            return FALSE;
        }
    }

    return TRUE;
}

static inline xsize gvs_read_unaligned_le(xuchar *bytes, xuint size)
{
    union {
        xuchar bytes[XLIB_SIZEOF_SIZE_T];
        xsize  integer;
    } tmpvalue;

    tmpvalue.integer = 0;
    if (bytes != NULL) {
        memcpy(&tmpvalue.bytes, bytes, size);
    }

    return XSIZE_FROM_LE(tmpvalue.integer);
}

static inline void gvs_write_unaligned_le(xuchar *bytes, xsize value, xuint size)
{
    union {
        xuchar bytes[XLIB_SIZEOF_SIZE_T];
        xsize  integer;
    } tmpvalue;

    tmpvalue.integer = XSIZE_TO_LE(value);
    memcpy(bytes, &tmpvalue.bytes, size);
}

static xuint gvs_get_offset_size(xsize size)
{
    if (size > X_MAXUINT32) {
        return 8;
    } else if (size > X_MAXUINT16) {
        return 4;
    } else if (size > X_MAXUINT8) {
        return 2;
    } else if (size > 0) {
        return 1;
    }

    return 0;
}

static xsize gvs_calculate_total_size(xsize body_size, xsize offsets)
{
    if (body_size + 1 * offsets <= X_MAXUINT8) {
        return body_size + 1 * offsets;
    }

    if (body_size + 2 * offsets <= X_MAXUINT16) {
        return body_size + 2 * offsets;
    }

    if (body_size + 4 * offsets <= X_MAXUINT32) {
        return body_size + 4 * offsets;
    }

    return body_size + 8 * offsets;
}

struct Offsets {
    xsize    data_size;
    xuchar   *array;
    xsize    length;
    xuint    offset_size;
    xboolean is_normal;
};

static xsize gvs_offsets_get_offset_n(struct Offsets *offsets, xsize n)
{
    return gvs_read_unaligned_le(offsets->array + (offsets->offset_size * n), offsets->offset_size);
}

static struct Offsets gvs_variable_sized_array_get_frame_offsets(XVariantSerialised value)
{
    xsize last_end;
    xsize offsets_array_size;
    struct Offsets out = { 0, };

    if (value.size == 0) {
        out.is_normal = TRUE;
        return out;
    }

    out.offset_size = gvs_get_offset_size(value.size);
    last_end = gvs_read_unaligned_le(value.data + value.size - out.offset_size, out.offset_size);

    if (last_end > value.size) {
        return out;
    }

    offsets_array_size = value.size - last_end;
    if (offsets_array_size % out.offset_size) {
        return out;
    }

    out.data_size = last_end;
    out.array = value.data + last_end;
    out.length = offsets_array_size / out.offset_size;
    if (out.length > 0 && gvs_calculate_total_size(last_end, out.length) != value.size) {
        return out; 
    }
    out.is_normal = TRUE;

    return out;
}

static xsize gvs_variable_sized_array_n_children(XVariantSerialised value)
{
    return gvs_variable_sized_array_get_frame_offsets(value).length;
}

#define DEFINE_FIND_UNORDERED(type, le_to_native)                                                           \
    static xsize find_unordered_##type(const xuint8 *data, xsize start, xsize len)                          \
    {                                                                                                       \
        xsize off;                                                                                          \
        type current_le, previous_le, current, previous;                                                    \
                                                                                                            \
        memcpy(&previous_le, data + start * sizeof(current), sizeof(current));                              \
        previous = le_to_native(previous_le);                                                               \
        for (off = (start + 1) * sizeof(current); off < len * sizeof(current); off += sizeof(current)) {    \
            memcpy(&current_le, data + off, sizeof(current));                                               \
            current = le_to_native(current_le);                                                             \
            if (current < previous) {                                                                       \
                break;                                                                                      \
            }                                                                                               \
            previous = current;                                                                             \
        }                                                                                                   \
        return off / sizeof(current) - 1;                                                                   \
    }

#define NO_CONVERSION(x)        (x)
DEFINE_FIND_UNORDERED(xuint8, NO_CONVERSION);
DEFINE_FIND_UNORDERED(xuint16, XUINT16_FROM_LE);
DEFINE_FIND_UNORDERED(xuint32, XUINT32_FROM_LE);
DEFINE_FIND_UNORDERED(xuint64, XUINT64_FROM_LE);

static XVariantSerialised gvs_variable_sized_array_get_child(XVariantSerialised value, xsize index_)
{
    xsize end;
    xsize start;
    XVariantSerialised child = { 0, };
    struct Offsets offsets = gvs_variable_sized_array_get_frame_offsets(value);

    child.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_ref(child.type_info);
    child.depth = value.depth + 1;

    if (index_ > value.checked_offsets_up_to && value.ordered_offsets_up_to == value.checked_offsets_up_to) {
        switch (offsets.offset_size) {
            case 1: {
                value.ordered_offsets_up_to = find_unordered_xuint8(offsets.array, value.checked_offsets_up_to, index_ + 1);
                break;
            }

            case 2: {
                value.ordered_offsets_up_to = find_unordered_xuint16(offsets.array, value.checked_offsets_up_to, index_ + 1);
                break;
            }

            case 4: {
                value.ordered_offsets_up_to = find_unordered_xuint32(offsets.array, value.checked_offsets_up_to, index_ + 1);
                break;
            }

            case 8: {
                value.ordered_offsets_up_to = find_unordered_xuint64(offsets.array, value.checked_offsets_up_to, index_ + 1);
                break;
            }

            default:
                x_assert_not_reached();
        }

        value.checked_offsets_up_to = index_;
    }

    if (index_ > value.ordered_offsets_up_to) {
        return child;
    }

    if (index_ > 0) {
        xuint alignment;

        start = gvs_offsets_get_offset_n(&offsets, index_ - 1);
        x_variant_type_info_query(child.type_info, &alignment, NULL);
        start += (-start) & alignment;
    } else {
        start = 0;
    }

    end = gvs_offsets_get_offset_n(&offsets, index_);
    if (start < end && end <= value.size && end <= offsets.data_size) {
        child.data = value.data + start;
        child.size = end - start;
    }

    return child;
}

static xsize gvs_variable_sized_array_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    xsize i;
    xsize offset;
    xuint alignment;

    x_variant_type_info_query(type_info, &alignment, NULL);
    offset = 0;

    for (i = 0; i < n_children; i++) {
        XVariantSerialised child = { 0, };

        offset += (-offset) & alignment;
        gvs_filler(&child, children[i]);
        offset += child.size;
    }

    return gvs_calculate_total_size(offset, n_children);
}

static void gvs_variable_sized_array_serialise(XVariantSerialised value, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    xsize i;
    xsize offset;
    xuint alignment;
    xsize offset_size;
    xuchar *offset_ptr;

    x_variant_type_info_query(value.type_info, &alignment, NULL);
    offset_size = gvs_get_offset_size(value.size);
    offset = 0;

    offset_ptr = value.data + value.size - offset_size * n_children;

    for (i = 0; i < n_children; i++) {
        XVariantSerialised child = { 0, };

        while (offset & alignment) {
            value.data[offset++] = '\0';
        }

        child.data = value.data + offset;
        gvs_filler(&child, children[i]);
        offset += child.size;

        gvs_write_unaligned_le(offset_ptr, offset, offset_size);
        offset_ptr += offset_size;
    }
}

static xboolean gvs_variable_sized_array_is_normal(XVariantSerialised value)
{
    xsize i;
    xsize offset;
    xuint alignment;
    XVariantSerialised child = { 0, };
    struct Offsets offsets = gvs_variable_sized_array_get_frame_offsets(value);

    if (!offsets.is_normal) {
        return FALSE;
    }

    if (value.size != 0 && offsets.length == 0) {
        return FALSE;
    }

    child.type_info = x_variant_type_info_element(value.type_info);
    x_variant_type_info_query(child.type_info, &alignment, NULL);
    child.depth = value.depth + 1;
    offset = 0;

    for (i = 0; i < offsets.length; i++) {
        xsize this_end;

        this_end = gvs_read_unaligned_le(offsets.array + offsets.offset_size * i, offsets.offset_size);
        if (this_end < offset || this_end > offsets.data_size) {
            return FALSE;
        }

        while (offset & alignment) {
            if (!(offset < this_end && value.data[offset] == '\0')) {
                return FALSE;
            }

            offset++;
        }

        child.data = value.data + offset;
        child.size = this_end - offset;

        if (child.size == 0) {
            child.data = NULL;
        }

        if (!x_variant_serialised_is_normal(child)) {
            return FALSE;
        }

        offset = this_end;
    }

    x_assert(offset == offsets.data_size);

    value.ordered_offsets_up_to = X_MAXSIZE;
    value.checked_offsets_up_to = X_MAXSIZE;

    return TRUE;
}

static void gvs_tuple_get_member_bounds(XVariantSerialised value, xsize index_, xsize offset_size, xsize *out_member_start, xsize *out_member_end)
{
    xsize member_start, member_end;
    const XVariantMemberInfo *member_info;

    member_info = x_variant_type_info_member_info(value.type_info, index_);

    if (member_info->i + 1 && offset_size * (member_info->i + 1) <= value.size) {
        member_start = gvs_read_unaligned_le(value.data + value.size - offset_size * (member_info->i + 1), offset_size);
    } else {
        member_start = 0;
    }

    member_start += member_info->a;
    member_start &= member_info->b;
    member_start |= member_info->c;

    if (member_info->ending_type == X_VARIANT_MEMBER_ENDING_LAST && offset_size * (member_info->i + 1) <= value.size) {
        member_end = value.size - offset_size * (member_info->i + 1);
    } else if (member_info->ending_type == X_VARIANT_MEMBER_ENDING_FIXED) {
        xsize fixed_size;

        x_variant_type_info_query(member_info->type_info, NULL, &fixed_size);
        member_end = member_start + fixed_size;
    } else if (member_info->ending_type == X_VARIANT_MEMBER_ENDING_OFFSET && offset_size * (member_info->i + 2) <= value.size) {
        member_end = gvs_read_unaligned_le(value.data + value.size - offset_size * (member_info->i + 2), offset_size);
    } else {
        member_end = X_MAXSIZE;
    }

    if (out_member_start != NULL) {
        *out_member_start = member_start;
    }

    if (out_member_end != NULL) {
        *out_member_end = member_end;
    }
}

static xsize gvs_tuple_n_children(XVariantSerialised value)
{
    return x_variant_type_info_n_members(value.type_info);
}

static XVariantSerialised gvs_tuple_get_child(XVariantSerialised value, xsize index_)
{
    xsize offset_size;
    xsize start, end, last_end;
    XVariantSerialised child = { 0, };
    const XVariantMemberInfo *member_info;

    member_info = x_variant_type_info_member_info(value.type_info, index_);
    child.type_info = x_variant_type_info_ref(member_info->type_info);
    child.depth = value.depth + 1;
    offset_size = gvs_get_offset_size(value.size);

    if (member_info->ending_type == X_VARIANT_MEMBER_ENDING_FIXED) {
        x_variant_type_info_query(child.type_info, NULL, &child.size);
    }

    if X_UNLIKELY(value.data == NULL && value.size != 0) {
        x_assert(child.size != 0);
        child.data = NULL;

        return child;
    }

    if (index_ > value.checked_offsets_up_to && value.ordered_offsets_up_to == value.checked_offsets_up_to) {
        xsize i, prev_i_end = 0;

        if (value.checked_offsets_up_to > 0) {
            gvs_tuple_get_member_bounds(value, value.checked_offsets_up_to - 1, offset_size, NULL, &prev_i_end);
        }

        for (i = value.checked_offsets_up_to; i <= index_; i++) {
            xsize i_start, i_end;

            gvs_tuple_get_member_bounds(value, i, offset_size, &i_start, &i_end);
            if (i_start > i_end || i_start < prev_i_end || i_end > value.size) {
                break;
            }

            prev_i_end = i_end;
        }

        value.ordered_offsets_up_to = i - 1;
        value.checked_offsets_up_to = index_;
    }

    if (index_ > value.ordered_offsets_up_to) {
        return child;
    }

    if (member_info->ending_type == X_VARIANT_MEMBER_ENDING_OFFSET) {
        if (offset_size * (member_info->i + 2) > value.size) {
            return child;
        }
    } else {
        if (offset_size * (member_info->i + 1) > value.size) {
            return child;
        }
    }

    gvs_tuple_get_member_bounds(value, index_, offset_size, &start, &end);
    gvs_tuple_get_member_bounds(value, x_variant_type_info_n_members(value.type_info) - 1, offset_size, NULL, &last_end);

    if (start < end && end <= value.size && end <= last_end) {
        child.data = value.data + start;
        child.size = end - start;
    }

    return child;
}

static xsize gvs_tuple_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    xsize i;
    xsize offset;
    xsize fixed_size;
    const XVariantMemberInfo *member_info = NULL;

    x_variant_type_info_query(type_info, NULL, &fixed_size);

    if (fixed_size) {
        return fixed_size;
    }

    offset = 0;
    x_assert(n_children > 0);

    for (i = 0; i < n_children; i++) {
        xuint alignment;

        member_info = x_variant_type_info_member_info(type_info, i);
        x_variant_type_info_query(member_info->type_info, &alignment, &fixed_size);
        offset += (-offset) & alignment;

        if (fixed_size) {
            offset += fixed_size;
        } else {
            XVariantSerialised child = { 0, };

            gvs_filler (&child, children[i]);
            offset += child.size;
        }
    }

    return gvs_calculate_total_size(offset, member_info->i + 1);
}

static void gvs_tuple_serialise(XVariantSerialised value, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    xsize i;
    xsize offset;
    xsize offset_size;

    offset_size = gvs_get_offset_size(value.size);
    offset = 0;

    for (i = 0; i < n_children; i++) {
        xuint alignment;
        XVariantSerialised child = { 0, };
        const XVariantMemberInfo *member_info;

        member_info = x_variant_type_info_member_info(value.type_info, i);
        x_variant_type_info_query(member_info->type_info, &alignment, NULL);

        while (offset & alignment) {
            value.data[offset++] = '\0';
        }

        child.data = value.data + offset;
        gvs_filler (&child, children[i]);
        offset += child.size;

        if (member_info->ending_type == X_VARIANT_MEMBER_ENDING_OFFSET) {
            value.size -= offset_size;
            gvs_write_unaligned_le(value.data + value.size, offset, offset_size);
        }
    }

    while (offset < value.size) {
        value.data[offset++] = '\0';
    }
}

static xboolean gvs_tuple_is_normal(XVariantSerialised value)
{
    xsize i;
    xsize length;
    xsize offset;
    xsize offset_ptr;
    xuint offset_size;
    xsize offset_table_size;

    if X_UNLIKELY(value.data == NULL && value.size != 0) {
        return FALSE;
    }

    offset_size = gvs_get_offset_size(value.size);
    length = x_variant_type_info_n_members(value.type_info);
    offset_ptr = value.size;
    offset = 0;

    for (i = 0; i < length; i++) {
        xsize end;
        xuint alignment;
        xsize fixed_size;
        XVariantSerialised child = { 0, };
        const XVariantMemberInfo *member_info;

        member_info = x_variant_type_info_member_info(value.type_info, i);
        child.type_info = member_info->type_info;
        child.depth = value.depth + 1;

        x_variant_type_info_query(child.type_info, &alignment, &fixed_size);

        while (offset & alignment) {
            if (offset > value.size || value.data[offset] != '\0') {
                return FALSE;
            }

            offset++;
        }

        child.data = value.data + offset;
        switch (member_info->ending_type) {
            case X_VARIANT_MEMBER_ENDING_FIXED:
                end = offset + fixed_size;
                break;

            case X_VARIANT_MEMBER_ENDING_LAST:
                end = offset_ptr;
                break;

            case X_VARIANT_MEMBER_ENDING_OFFSET:
                if (offset_ptr < offset_size) {
                    return FALSE;
                }

                offset_ptr -= offset_size;
                if (offset_ptr < offset) {
                    return FALSE;
                }

                end = gvs_read_unaligned_le(value.data + offset_ptr, offset_size);
                break;

            default:
                x_assert_not_reached();
        }

        if (end < offset || end > offset_ptr) {
            return FALSE;
        }

        child.size = end - offset;
        if (child.size == 0) {
            child.data = NULL;
        }

        if (!x_variant_serialised_is_normal(child)) {
            return FALSE;
        }

        offset = end;
    }

    value.ordered_offsets_up_to = X_MAXSIZE;
    value.checked_offsets_up_to = X_MAXSIZE;

    {
        xuint alignment;
        xsize fixed_size;

        x_variant_type_info_query(value.type_info, &alignment, &fixed_size);

        if (fixed_size) {
            x_assert(fixed_size == value.size);
            x_assert(offset_ptr == value.size);

            if (i == 0) {
                if (value.data[offset++] != '\0') {
                    return FALSE;
                }
            } else {
                while (offset & alignment) {
                    if (value.data[offset++] != '\0') {
                        return FALSE;
                    }
                }
            }

            x_assert(offset == value.size);
        }
    }

    if (offset_ptr != offset) {
        return FALSE;
    }

    offset_table_size = value.size - offset_ptr;
    if (value.size > 0 && gvs_calculate_total_size(offset, offset_table_size / offset_size) != value.size) {
        return FALSE;
    }

    return TRUE;
}

static inline xsize gvs_variant_n_children(XVariantSerialised value)
{
    return 1;
}

static inline XVariantSerialised gvs_variant_get_child(XVariantSerialised value, xsize index_)
{
    XVariantSerialised child = { 0, };

    if (value.size) {
        for (child.size = value.size - 1; child.size; child.size--) {
            if (value.data[child.size] == '\0') {
                break;
            }
        }

        if (value.data[child.size] == '\0') {
            const xchar *end;
            const xchar *limit = (xchar *)&value.data[value.size];
            const xchar *type_string = (xchar *)&value.data[child.size + 1];

            if (x_variant_type_string_scan(type_string, limit, &end) && end == limit) {
                const XVariantType *type = (XVariantType *)type_string;

                if (x_variant_type_is_definite(type)) {
                    xsize fixed_size;
                    xsize child_type_depth;

                    child.type_info = x_variant_type_info_get(type);
                    child.depth = value.depth + 1;

                    if (child.size != 0) {
                        child.data = value.data;
                    }

                    x_variant_type_info_query(child.type_info, NULL, &fixed_size);
                    child_type_depth = x_variant_type_info_query_depth(child.type_info);

                    if ((!fixed_size || fixed_size == child.size) && value.depth < X_VARIANT_MAX_RECURSION_DEPTH - child_type_depth) {
                        return child;
                    }

                    x_variant_type_info_unref(child.type_info);
                }
            }
        }
    }

    child.type_info = x_variant_type_info_get(X_VARIANT_TYPE_UNIT);
    child.data = NULL;
    child.size = 1;
    child.depth = value.depth + 1;

    return child;
}

static inline xsize gvs_variant_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    const xchar *type_string;
    XVariantSerialised child = { 0, };

    gvs_filler (&child, children[0]);
    type_string = x_variant_type_info_get_type_string(child.type_info);

    return child.size + 1 + strlen(type_string);
}

static inline void gvs_variant_serialise(XVariantSerialised value, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    const xchar *type_string;
    XVariantSerialised child = { 0, };

    child.data = value.data;

    gvs_filler (&child, children[0]);
    type_string = x_variant_type_info_get_type_string(child.type_info);
    value.data[child.size] = '\0';
    memcpy(value.data + child.size + 1, type_string, strlen(type_string));
}

static inline xboolean gvs_variant_is_normal(XVariantSerialised value)
{
    xboolean normal;
    xsize child_type_depth;
    XVariantSerialised child;

    child = gvs_variant_get_child(value, 0);
    child_type_depth = x_variant_type_info_query_depth(child.type_info);

    normal = (value.depth < X_VARIANT_MAX_RECURSION_DEPTH - child_type_depth) && (child.data != NULL || child.size == 0) && x_variant_serialised_is_normal (child);
    x_variant_type_info_unref(child.type_info);

    return normal;
}

#define DISPATCH_FIXED(type_info, before, after)                                        \
    {                                                                                   \
        xsize fixed_size;                                                               \
                                                                                        \
        x_variant_type_info_query_element(type_info, NULL, &fixed_size);                \
                                                                                        \
        if (fixed_size) {                                                               \
            before ## fixed_sized ## after                                              \
        } else {                                                                        \
            before ## variable_sized ## after                                           \
        }                                                                               \
    }

#define DISPATCH_CASES(type_info, before, after)                                        \
    switch (x_variant_type_info_get_type_char(type_info)) {                             \
        case X_VARIANT_TYPE_INFO_CHAR_MAYBE:                                            \
            DISPATCH_FIXED (type_info, before, _maybe ## after)                         \
                                                                                        \
        case X_VARIANT_TYPE_INFO_CHAR_ARRAY:                                            \
            DISPATCH_FIXED (type_info, before, _array ## after)                         \
                                                                                        \
        case X_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY:                                       \
        case X_VARIANT_TYPE_INFO_CHAR_TUPLE: {                                          \
            before ## tuple ## after                                                    \
        }                                                                               \
                                                                                        \
        case X_VARIANT_TYPE_INFO_CHAR_VARIANT: {                                        \
            before ## variant ## after                                                  \
        }                                                                               \
    }

xsize x_variant_serialised_n_children(XVariantSerialised serialised)
{
    x_assert(x_variant_serialised_check(serialised));

    DISPATCH_CASES(serialised.type_info, return gvs_/**/,/**/_n_children(serialised);)
    x_assert_not_reached ();
}

XVariantSerialised x_variant_serialised_get_child(XVariantSerialised serialised, xsize index_)
{
    XVariantSerialised child;

    x_assert(x_variant_serialised_check(serialised));

    if X_LIKELY(index_ < x_variant_serialised_n_children(serialised)) {
        DISPATCH_CASES(serialised.type_info,
            child = gvs_/**/,/**/_get_child(serialised, index_);
            x_assert(child.size || child.data == NULL);
            x_assert(x_variant_serialised_check(child));
            return child;
        )
        x_assert_not_reached();
    }

    x_error("Attempt to access item %" X_XSIZE_FORMAT " in a container with only %" X_XSIZE_FORMAT " items", index_, x_variant_serialised_n_children(serialised));
}

void x_variant_serialiser_serialise(XVariantSerialised serialised, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    x_assert(x_variant_serialised_check(serialised));

    DISPATCH_CASES(serialised.type_info,
        gvs_/**/,/**/_serialise(serialised, gvs_filler, children, n_children);
        return;
    )
    x_assert_not_reached();
}

xsize x_variant_serialiser_needed_size(XVariantTypeInfo *type_info, XVariantSerialisedFiller gvs_filler, const xpointer *children, xsize n_children)
{
    DISPATCH_CASES(type_info,
        return gvs_/**/,/**/_needed_size(type_info, gvs_filler, children, n_children);
    )
    x_assert_not_reached();
}

void x_variant_serialised_byteswap(XVariantSerialised serialised)
{
    xuint alignment;
    xsize fixed_size;

    x_assert(x_variant_serialised_check(serialised));

    if (!serialised.data) {
        return;
    }

    x_variant_type_info_query(serialised.type_info, &alignment, &fixed_size);
    if (!alignment) {
        return;
    }

    if (alignment + 1 == fixed_size) {
        switch (fixed_size) {
            case 2: {
                xuint16 *ptr = (xuint16 *)serialised.data;
                x_assert_cmpint(serialised.size, ==, 2);
                *ptr = XUINT16_SWAP_LE_BE(*ptr);
            }
            return;

            case 4: {
                xuint32 *ptr = (xuint32 *)serialised.data;
                x_assert_cmpint(serialised.size, ==, 4);
                *ptr = XUINT32_SWAP_LE_BE(*ptr);
            }
            return;

            case 8: {
                xuint64 *ptr = (xuint64 *)serialised.data;
                x_assert_cmpint(serialised.size, ==, 8);
                *ptr = XUINT64_SWAP_LE_BE (*ptr);
            }
            return;

            default:
                x_assert_not_reached ();
        }
    } else {
        xsize children, i;

        children = x_variant_serialised_n_children(serialised);
        for (i = 0; i < children; i++) {
            XVariantSerialised child;

            child = x_variant_serialised_get_child(serialised, i);
            x_variant_serialised_byteswap(child);
            x_variant_type_info_unref(child.type_info);
        }
    }
}

xboolean x_variant_serialised_is_normal(XVariantSerialised serialised)
{
    if (serialised.depth >= X_VARIANT_MAX_RECURSION_DEPTH) {
        return FALSE;
    }

    DISPATCH_CASES(serialised.type_info,
        return gvs_/**/,/**/_is_normal(serialised);
    )

    if (serialised.data == NULL) {
        return FALSE;
    }

    switch (x_variant_type_info_get_type_char(serialised.type_info)) {
        case 'b':
            return serialised.data[0] < 2;

        case 's':
            return x_variant_serialiser_is_string(serialised.data, serialised.size);

        case 'o':
            return x_variant_serialiser_is_object_path(serialised.data, serialised.size);

        case 'g':
            return x_variant_serialiser_is_signature(serialised.data, serialised.size);

        default:
            return TRUE;
    }
}

xboolean x_variant_serialiser_is_string(xconstpointer data, xsize size)
{
    const xchar *end;
    const xchar *expected_end;

    if (size == 0) {
        return FALSE;
    }

    expected_end = ((xchar *)data) + size - 1;
    if (*expected_end != '\0') {
        return FALSE;
    }

    x_utf8_validate_len((const xchar *)data, size, &end);
    return end == expected_end;
}

xboolean x_variant_serialiser_is_object_path(xconstpointer data, xsize size)
{
    xsize i;
    const xchar *string = (const xchar *)data;

    if (!x_variant_serialiser_is_string(data, size)) {
        return FALSE;
    }

    if (string[0] != '/') {
        return FALSE;
    }

    for (i = 1; string[i]; i++) {
        if (x_ascii_isalnum(string[i]) || string[i] == '_') {
            ;
        } else if (string[i] == '/') {
            if (string[i - 1] == '/') {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (i > 1 && string[i - 1] == '/') {
        return FALSE;
    }

    return TRUE;
}

xboolean x_variant_serialiser_is_signature(xconstpointer data, xsize size)
{
    xsize first_invalid;
    const xchar *string = (const xchar *)data;

    if (!x_variant_serialiser_is_string(data, size)) {
        return FALSE;
    }

    first_invalid = strspn(string, "ybnqiuxthdvasog(){}");
    if (string[first_invalid]) {
        return FALSE;
    }

    while (*string) {
        if (!x_variant_type_string_scan(string, NULL, &string)) {
            return FALSE;
        }
    }

    return TRUE;
}
