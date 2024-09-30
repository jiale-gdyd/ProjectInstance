#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xlib_trace.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xvarianttypeinfo.h>
#include <xlib/xlib/xvarianttype-private.h>

struct _XVariantTypeInfo {
    xsize  fixed_size;
    xuchar alignment;
    xuchar container_class;
};

typedef struct {
    XVariantTypeInfo info;
    xchar            *type_string;
    xatomicrefcount  ref_count;
} ContainerInfo;

typedef struct {
    ContainerInfo    container;
    XVariantTypeInfo *element;
} ArrayInfo;

typedef struct {
    ContainerInfo      container;
    XVariantMemberInfo *members;
    xsize              n_members;
} TupleInfo;

#define XV_ARRAY_INFO_CLASS         'a'
#define XV_TUPLE_INFO_CLASS         'r'

static const XVariantTypeInfo x_variant_type_info_basic_table[24] = {
#define fixed_aligned(x)  x, x - 1, 0
#define not_a_type        0,     0, 0
#define unaligned         0,     0, 0
#define aligned(x)        0, x - 1, 0
    /* 'b' */ { fixed_aligned(1) },   /* boolean */
    /* 'c' */ { not_a_type },
    /* 'd' */ { fixed_aligned(8) },   /* double */
    /* 'e' */ { not_a_type },
    /* 'f' */ { not_a_type },
    /* 'g' */ { unaligned        },   /* signature string */
    /* 'h' */ { fixed_aligned(4) },   /* file handle (int32) */
    /* 'i' */ { fixed_aligned(4) },   /* int32 */
    /* 'j' */ { not_a_type },
    /* 'k' */ { not_a_type },
    /* 'l' */ { not_a_type },
    /* 'm' */ { not_a_type },
    /* 'n' */ { fixed_aligned(2) },   /* int16 */
    /* 'o' */ { unaligned        },   /* object path string */
    /* 'p' */ { not_a_type },
    /* 'q' */ { fixed_aligned(2) },   /* uint16 */
    /* 'r' */ { not_a_type },
    /* 's' */ { unaligned        },   /* string */
    /* 't' */ { fixed_aligned(8) },   /* uint64 */
    /* 'u' */ { fixed_aligned(4) },   /* uint32 */
    /* 'v' */ { aligned(8)       },   /* variant */
    /* 'w' */ { not_a_type },
    /* 'x' */ { fixed_aligned(8) },   /* int64 */
    /* 'y' */ { fixed_aligned(1) },   /* byte */
#undef fixed_aligned
#undef not_a_type
#undef unaligned
#undef aligned
};

static const char x_variant_type_info_basic_chars[24][2] = {
    "b", " ", "d", " ", " ", "g", "h", "i", " ", " ", " ", " ",
    "n", "o", " ", "q", " ", "s", "t", "u", "v", " ", "x", "y"
};

static void x_variant_type_info_check(const XVariantTypeInfo *info, char container_class)
{
    x_assert(!container_class || info->container_class == container_class);

    x_assert(info->alignment == 0 || info->alignment == 1 || info->alignment == 3 || info->alignment == 7);

    if (info->container_class) {
        ContainerInfo *container = (ContainerInfo *)info;

        x_assert(!x_atomic_ref_count_compare(&container->ref_count, 0));
        x_assert(container->type_string != NULL);
    } else {
        xint index;

        index = info - x_variant_type_info_basic_table;

        x_assert(X_N_ELEMENTS(x_variant_type_info_basic_table) == 24);
        x_assert(X_N_ELEMENTS(x_variant_type_info_basic_chars) == 24);
        x_assert(0 <= index && index < 24);
        x_assert(x_variant_type_info_basic_chars[index][0] != ' ');
    }
}

const xchar *x_variant_type_info_get_type_string(XVariantTypeInfo *info)
{
    x_variant_type_info_check(info, 0);

    if (info->container_class) {
        ContainerInfo *container = (ContainerInfo *)info;
        return container->type_string;
    } else {
        xint index;

        index = info - x_variant_type_info_basic_table;
        return x_variant_type_info_basic_chars[index];
    }
}

void x_variant_type_info_query(XVariantTypeInfo *info, xuint *alignment, xsize *fixed_size)
{
    if (alignment) {
        *alignment = info->alignment;
    }

    if (fixed_size) {
        *fixed_size = info->fixed_size;
    }
}

xsize x_variant_type_info_query_depth(XVariantTypeInfo *info)
{
    x_variant_type_info_check (info, 0);

    if (info->container_class) {
        ContainerInfo *container = (ContainerInfo *)info;
        return x_variant_type_string_get_depth_(container->type_string);
    }

    return 1;
}

static ArrayInfo *GV_ARRAY_INFO(XVariantTypeInfo *info)
{
    x_variant_type_info_check (info, XV_ARRAY_INFO_CLASS);
    return (ArrayInfo *) info;
}

static void array_info_free(XVariantTypeInfo *info)
{
    ArrayInfo *array_info;

    x_assert(info->container_class == XV_ARRAY_INFO_CLASS);
    array_info = (ArrayInfo *)info;

    x_variant_type_info_unref(array_info->element);
    x_slice_free(ArrayInfo, array_info);
}

static ContainerInfo *array_info_new(const XVariantType *type)
{
    ArrayInfo *info;

    info = x_slice_new(ArrayInfo);
    info->container.info.container_class = XV_ARRAY_INFO_CLASS;

    info->element = x_variant_type_info_get(x_variant_type_element(type));
    info->container.info.alignment = info->element->alignment;
    info->container.info.fixed_size = 0;

    return (ContainerInfo *)info;
}

XVariantTypeInfo *x_variant_type_info_element(XVariantTypeInfo *info)
{
    return GV_ARRAY_INFO(info)->element;
}

void x_variant_type_info_query_element(XVariantTypeInfo *info, xuint *alignment, xsize *fixed_size)
{
    x_variant_type_info_query(GV_ARRAY_INFO(info)->element, alignment, fixed_size);
}

static TupleInfo *GV_TUPLE_INFO(XVariantTypeInfo *info)
{
    x_variant_type_info_check(info, XV_TUPLE_INFO_CLASS);
    return (TupleInfo *)info;
}

static void tuple_info_free(XVariantTypeInfo *info)
{
    xsize i;
    TupleInfo *tuple_info;

    x_assert(info->container_class == XV_TUPLE_INFO_CLASS);
    tuple_info = (TupleInfo *)info;

    for (i = 0; i < tuple_info->n_members; i++) {
        x_variant_type_info_unref(tuple_info->members[i].type_info);
    }

    x_slice_free1(sizeof(XVariantMemberInfo) * tuple_info->n_members, tuple_info->members);
    x_slice_free(TupleInfo, tuple_info);
}

static void tuple_allocate_members(const XVariantType *type, XVariantMemberInfo **members, xsize *n_members)
{
    xsize i = 0;
    const XVariantType *item_type;

    *n_members = x_variant_type_n_items(type);
    *members = (XVariantMemberInfo *)x_slice_alloc(sizeof(XVariantMemberInfo) * *n_members);

    item_type = x_variant_type_first(type);
    while (item_type) {
        XVariantMemberInfo *member = &(*members)[i++];

        member->type_info = x_variant_type_info_get(item_type);
        item_type = x_variant_type_next(item_type);

        if (member->type_info->fixed_size) {
            member->ending_type = X_VARIANT_MEMBER_ENDING_FIXED;
        } else if (item_type == NULL) {
            member->ending_type = X_VARIANT_MEMBER_ENDING_LAST;
        } else {
            member->ending_type = X_VARIANT_MEMBER_ENDING_OFFSET;
        }
    }

    x_assert(i == *n_members);
}

static xboolean tuple_get_item(TupleInfo *info, XVariantMemberInfo *item, xsize *d, xsize *e)
{
    if (&info->members[info->n_members] == item) {
        return FALSE;
    }

    *d = item->type_info->alignment;
    *e = item->type_info->fixed_size;
    return TRUE;
}

static void tuple_table_append(XVariantMemberInfo **items, xsize i, xsize a, xsize b, xsize c)
{
    XVariantMemberInfo *item = (*items)++;

    a += ~b & c;
    c &= b;

    item->i = i;
    item->a = a + b;
    item->b = ~b;
    item->c = c;
}

static xsize tuple_align(xsize offset, xuint alignment)
{
    return offset + ((-offset) & alignment);
}

static void tuple_generate_table(TupleInfo *info)
{
    xsize i = -1, a = 0, b = 0, c = 0, d, e;
    XVariantMemberInfo *items = info->members;

    while (tuple_get_item(info, items, &d, &e)) {
        if (d <= b) {
            c = tuple_align(c, d);
        } else {
            a += tuple_align(c, b), b = d, c = 0;
        }

        tuple_table_append(&items, i, a, b, c);
        if (e == 0) {
            i++, a = b = c = 0;
        } else {
            c += e;
        }
    }
}

static void tuple_set_base_info(TupleInfo *info)
{
    XVariantTypeInfo *base = &info->container.info;

    if (info->n_members > 0) {
        XVariantMemberInfo *m;

        base->alignment = 0;
        for (m = info->members; m < &info->members[info->n_members]; m++) {
            base->alignment |= m->type_info->alignment;
        }

        m--;
        if (m->i == (xsize) -1 && m->type_info->fixed_size) {
            base->fixed_size = tuple_align(((m->a & m->b) | m->c) + m->type_info->fixed_size, base->alignment);
        } else {
            base->fixed_size = 0;
        }
    } else {
        base->alignment = 0;
        base->fixed_size = 1;
    }
}

static ContainerInfo *tuple_info_new(const XVariantType *type)
{
    TupleInfo *info;

    info = x_slice_new(TupleInfo);
    info->container.info.container_class = XV_TUPLE_INFO_CLASS;

    tuple_allocate_members(type, &info->members, &info->n_members);
    tuple_generate_table(info);
    tuple_set_base_info(info);

    return (ContainerInfo *)info;
}

xsize x_variant_type_info_n_members(XVariantTypeInfo *info)
{
    return GV_TUPLE_INFO(info)->n_members;
}

const XVariantMemberInfo *x_variant_type_info_member_info(XVariantTypeInfo *info, xsize index)
{
    TupleInfo *tuple_info = GV_TUPLE_INFO(info);

    if (index < tuple_info->n_members) {
        return &tuple_info->members[index];
    }

    return NULL;
}

static XRecMutex x_variant_type_info_lock;
static XHashTable *x_variant_type_info_table;

XVariantTypeInfo *x_variant_type_info_get(const XVariantType *type)
{
    const xchar *type_string = x_variant_type_peek_string(type);
    const char type_char = type_string[0];

    if (type_char == X_VARIANT_TYPE_INFO_CHAR_MAYBE
        || type_char == X_VARIANT_TYPE_INFO_CHAR_ARRAY
        || type_char == X_VARIANT_TYPE_INFO_CHAR_TUPLE
        || type_char == X_VARIANT_TYPE_INFO_CHAR_DICT_ENTRY)
    {
        XVariantTypeInfo *info;

        x_rec_mutex_lock(&x_variant_type_info_lock);

        if (x_variant_type_info_table == NULL) {
            x_variant_type_info_table = x_hash_table_new((XHashFunc)_x_variant_type_hash, (XEqualFunc)_x_variant_type_equal);
        }

        info = (XVariantTypeInfo *)x_hash_table_lookup(x_variant_type_info_table, type_string);
        if (info == NULL) {
            ContainerInfo *container;

            if (type_char == X_VARIANT_TYPE_INFO_CHAR_MAYBE || type_char == X_VARIANT_TYPE_INFO_CHAR_ARRAY) {
                container = array_info_new(type);
            } else {
                container = tuple_info_new(type);
            }

            info = (XVariantTypeInfo *)container;
            container->type_string = x_variant_type_dup_string(type);
            x_atomic_ref_count_init(&container->ref_count);

            TRACE(XLIB_VARIANT_TYPE_INFO_NEW(info, container->type_string));

            x_hash_table_replace(x_variant_type_info_table, container->type_string, info);
        } else {
            x_variant_type_info_ref(info);
        }

        x_rec_mutex_unlock(&x_variant_type_info_lock);
        x_variant_type_info_check(info, 0);

        return info;
    } else {
        int index;
        const XVariantTypeInfo *info;

        index = type_char - 'b';
        x_assert(X_N_ELEMENTS(x_variant_type_info_basic_table) == 24);
        x_assert_cmpint(0, <=, index);
        x_assert_cmpint(index, <, 24);

        info = x_variant_type_info_basic_table + index;
        x_variant_type_info_check(info, 0);

        TRACE(XLIB_VARIANT_TYPE_INFO_NEW(info, x_variant_type_info_basic_chars[index]));

        return (XVariantTypeInfo *)info;
    }
}

XVariantTypeInfo *x_variant_type_info_ref(XVariantTypeInfo *info)
{
    x_variant_type_info_check(info, 0);

    if (info->container_class) {
        ContainerInfo *container = (ContainerInfo *)info;
        x_atomic_ref_count_inc(&container->ref_count);
    }

    return info;
}

void x_variant_type_info_unref(XVariantTypeInfo *info)
{
    x_variant_type_info_check(info, 0);

    if (info->container_class) {
        ContainerInfo *container = (ContainerInfo *)info;

        x_rec_mutex_lock(&x_variant_type_info_lock);
        if (x_atomic_ref_count_dec(&container->ref_count)) {
            TRACE(XLIB_VARIANT_TYPE_INFO_FREE(info));

            x_hash_table_remove(x_variant_type_info_table, container->type_string);
            if (x_hash_table_size(x_variant_type_info_table) == 0) {
                x_hash_table_unref(x_variant_type_info_table);
                x_variant_type_info_table = NULL;
            }
            x_rec_mutex_unlock(&x_variant_type_info_lock);

            x_free(container->type_string);

            if (info->container_class == XV_ARRAY_INFO_CLASS) {
                array_info_free(info);
            } else if (info->container_class == XV_TUPLE_INFO_CLASS) {
                tuple_info_free(info);
            } else {
                x_assert_not_reached();
            }
        } else {
            x_rec_mutex_unlock(&x_variant_type_info_lock);
        }
    }
}

void x_variant_type_info_assert_no_infos(void)
{
    x_assert(x_variant_type_info_table == NULL);
}
