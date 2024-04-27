#include <string.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xtype.h>
#include <xlib/xobj/xtypeplugin.h>
#include <xlib/xobj/xatomicarray.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xconstructor.h>
#include <xlib/xobj/xobject_trace.h>
#include <xlib/xobj/xtype-private.h>
#include <xlib/xobj/xvaluecollector.h>

#define X_READ_LOCK(rw_lock)                        x_rw_lock_reader_lock(rw_lock)
#define X_READ_UNLOCK(rw_lock)                      x_rw_lock_reader_unlock(rw_lock)
#define X_WRITE_LOCK(rw_lock)                       x_rw_lock_writer_lock(rw_lock)
#define X_WRITE_UNLOCK(rw_lock)                     x_rw_lock_writer_unlock(rw_lock)

#define INVALID_RECURSION(func, arg, type_name)                                                 \
    X_STMT_START {                                                                              \
        static const xchar _action[] = " invalidly modified type ";                             \
        xpointer _arg = (xpointer)(arg);                                                        \
        const xchar *_tname = (type_name), *_fname = (func);                                    \
        if (_arg) {                                                                             \
            x_error("%s(%p)%s'%s'", _fname, _arg, _action, _tname);                             \
        } else {                                                                                \
            x_error("%s()%s'%s'", _fname, _action, _tname);                                     \
        }                                                                                       \
    } X_STMT_END

#define x_assert_type_system_initialized()                                                      \
    x_assert(static_quark_type_flags)

#define x_type_test_flags(t, f)                     _x_type_test_flags(t, f)

#define TYPE_FUNDAMENTAL_FLAG_MASK                  (X_TYPE_FLAG_CLASSED | X_TYPE_FLAG_INSTANTIATABLE | X_TYPE_FLAG_DERIVABLE | X_TYPE_FLAG_DEEP_DERIVABLE)
#define TYPE_FLAG_MASK                              (X_TYPE_FLAG_ABSTRACT | X_TYPE_FLAG_VALUE_ABSTRACT | X_TYPE_FLAG_FINAL | X_TYPE_FLAG_DEPRECATED)
#define	NODE_FLAG_MASK                              (X_TYPE_FLAG_ABSTRACT | X_TYPE_FLAG_CLASSED | X_TYPE_FLAG_DEPRECATED | X_TYPE_FLAG_INSTANTIATABLE | X_TYPE_FLAG_FINAL)
#define SIZEOF_FUNDAMENTAL_INFO                     ((xssize)MAX(MAX(sizeof(XTypeFundamentalInfo), sizeof(xpointer)), sizeof(xlong)))

#define STRUCT_ALIGNMENT                            (2 * sizeof(xsize))
#define ALIGN_STRUCT(offset)                        ((offset + (STRUCT_ALIGNMENT - 1)) & -STRUCT_ALIGNMENT)

typedef struct _TypeNode TypeNode;
typedef union  _TypeData TypeData;
typedef struct _BoxedData BoxedData;
typedef struct _IFaceData IFaceData;
typedef struct _ClassData ClassData;
typedef struct _CommonData CommonData;
typedef struct _IFaceEntry IFaceEntry;
typedef struct _IFaceHolder IFaceHolder;
typedef struct _InstanceData InstanceData;
typedef struct _IFaceEntries IFaceEntries;

static inline xboolean _x_type_test_flags(XType type, xuint flags);
static inline XTypeFundamentalInfo *type_node_fundamental_info_I(TypeNode *node);
static void type_add_flags_W(TypeNode *node, XTypeFlags flags);
static void type_data_make_W(TypeNode *node, const XTypeInfo *info, const XTypeValueTable *value_table);
static inline void type_data_ref_Wm(TypeNode *node);
static inline void type_data_unref_U(TypeNode *node, xboolean uncached);
static void type_data_last_unref_Wm(TypeNode *node, xboolean uncached);
static inline xpointer type_get_qdata_L(TypeNode *node, XQuark quark);
static inline void type_set_qdata_W(TypeNode *node, XQuark quark, xpointer data);
static IFaceHolder *type_iface_peek_holder_L(TypeNode *iface, XType instance_type);
static xboolean type_iface_vtable_base_init_Wm(TypeNode *iface, TypeNode *node);
static void type_iface_vtable_iface_init_Wm(TypeNode *iface, TypeNode *node);
static xboolean type_node_is_a_L(TypeNode *node, TypeNode *iface_node);

typedef enum {
    UNINITIALIZED,
    BASE_CLASS_INIT,
    BASE_IFACE_INIT,
    CLASS_INIT,
    IFACE_INIT,
    INITIALIZED
} InitState;

struct _TypeNode {
    xuint       ref_count;
    XTypePlugin *plugin;
    xuint       n_children;
    xuint       n_supers : 8;
    xuint       n_prerequisites : 9;
    xuint       is_abstract : 1;
    xuint       is_classed : 1;
    xuint       is_deprecated : 1;
    xuint       is_instantiatable : 1;
    xuint       is_final : 1;
    xuint       mutatable_check_cache : 1;
    XType       *children;
    TypeData    *data;
    XQuark      qname;
    XData       *global_gdata;

    union {
        XAtomicArray iface_entries;
        XAtomicArray offsets;
    } _prot;

    XType       *prerequisites;
    XType       supers[1];
};

#define SIZEOF_BASE_TYPE_NODE()                                         (X_STRUCT_OFFSET(TypeNode, supers))
#define MAX_N_SUPERS                                                    (255)
#define MAX_N_CHILDREN                                                  (X_MAXUINT)
#define MAX_N_INTERFACES                                                (255)
#define MAX_N_PREREQUISITES                                             (511)
#define NODE_TYPE(node)                                                 (node->supers[0])
#define NODE_PARENT_TYPE(node)                                          (node->supers[1])
#define NODE_FUNDAMENTAL_TYPE(node)                                     (node->supers[node->n_supers])
#define NODE_NAME(node)                                                 (x_quark_to_string(node->qname))
#define NODE_REFCOUNT(node)                                             ((xuint)x_atomic_int_get((int *)&(node)->ref_count))
#define NODE_IS_BOXED(node)                                             (NODE_FUNDAMENTAL_TYPE(node) == X_TYPE_BOXED)
#define NODE_IS_IFACE(node)                                             (NODE_FUNDAMENTAL_TYPE(node) == X_TYPE_INTERFACE)
#define CLASSED_NODE_IFACES_ENTRIES(node)                               (&(node)->_prot.iface_entries)
#define CLASSED_NODE_IFACES_ENTRIES_LOCKED(node)                        (X_ATOMIC_ARRAY_GET_LOCKED(CLASSED_NODE_IFACES_ENTRIES((node)), IFaceEntries))
#define IFACE_NODE_N_PREREQUISITES(node)                                ((node)->n_prerequisites)
#define IFACE_NODE_PREREQUISITES(node)                                  ((node)->prerequisites)
#define iface_node_get_holders_L(node)                                  ((IFaceHolder *)type_get_qdata_L((node), static_quark_iface_holder))
#define iface_node_set_holders_W(node, holders)                         (type_set_qdata_W ((node), static_quark_iface_holder, (holders)))
#define iface_node_get_dependants_array_L(n)                            ((XType *)type_get_qdata_L((n), static_quark_dependants_array))
#define iface_node_set_dependants_array_W(n,d)                          (type_set_qdata_W((n), static_quark_dependants_array, (d)))
#define TYPE_ID_MASK                                                    ((XType) ((1 << X_TYPE_FUNDAMENTAL_SHIFT) - 1))

#define NODE_IS_ANCESTOR(ancestor, node)                                ((ancestor)->n_supers <= (node)->n_supers && (node)->supers[(node)->n_supers - (ancestor)->n_supers] == NODE_TYPE(ancestor))

struct _IFaceHolder {
    XType          instance_type;
    XInterfaceInfo *info;
    XTypePlugin    *plugin;
    IFaceHolder    *next;
};

struct _IFaceEntry {
    XType          iface_type;
    XTypeInterface *vtable;
    InitState      init_state;
};

struct _IFaceEntries {
    xsize      offset_index;
    IFaceEntry entry[1];
};

#define IFACE_ENTRIES_HEADER_SIZE                                   (sizeof(IFaceEntries) - sizeof(IFaceEntry))
#define IFACE_ENTRIES_N_ENTRIES(_entries)                           ((X_ATOMIC_ARRAY_DATA_SIZE((_entries)) - IFACE_ENTRIES_HEADER_SIZE) / sizeof(IFaceEntry))

struct _CommonData {
    XTypeValueTable *value_table;
};

struct _BoxedData {
    CommonData     data;
    XBoxedCopyFunc copy_func;
    XBoxedFreeFunc free_func;
};

struct _IFaceData {
    CommonData         common;
    xuint16            vtable_size;
    XBaseInitFunc      vtable_init_base;
    XBaseFinalizeFunc  vtable_finalize_base;
    XClassInitFunc     dflt_init;
    XClassFinalizeFunc dflt_finalize;
    xconstpointer      dflt_data;
    xpointer           dflt_vtable;
};

struct _ClassData {
    CommonData         common;
    xuint16            class_size;
    xuint16            class_private_size;
    int                init_state;
    XBaseInitFunc      class_init_base;
    XBaseFinalizeFunc  class_finalize_base;
    XClassInitFunc     class_init;
    XClassFinalizeFunc class_finalize;
    xconstpointer      class_data;
    xpointer           classt;
};

struct _InstanceData {
    CommonData         common;
    xuint16            class_size;
    xuint16            class_private_size;
    int                init_state;
    XBaseInitFunc      class_init_base;
    XBaseFinalizeFunc  class_finalize_base;
    XClassInitFunc     class_init;
    XClassFinalizeFunc class_finalize;
    xconstpointer      class_data;
    xpointer           classt;
    xuint16            instance_size;
    xuint16            private_size;
    XInstanceInitFunc  instance_init;
};

union _TypeData {
    CommonData   common;
    BoxedData    boxed;
    IFaceData    iface;
    ClassData    classt;
    InstanceData instance;
};

typedef struct {
    xpointer            cache_data;
    XTypeClassCacheFunc cache_func;
} ClassCacheFunc;

typedef struct {
    xpointer                check_data;
    XTypeInterfaceCheckFunc check_func;
} IFaceCheckFunc;

static XRWLock type_rw_lock;
static XRecMutex class_init_rec_mutex;
static XQuark static_quark_type_flags = 0;
static xuint type_registration_serial = 0;
static xuint static_n_class_cache_funcs = 0;
static XQuark static_quark_iface_holder = 0;
static xuint static_n_iface_check_funcs = 0;
static XQuark static_quark_dependants_array = 0;
static ClassCacheFunc *static_class_cache_funcs = NULL;
static IFaceCheckFunc *static_iface_check_funcs = NULL;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XTypeDebugFlags _x_type_debug_flags = (XTypeDebugFlags)0;
X_GNUC_END_IGNORE_DEPRECATIONS

static XHashTable *static_type_nodes_ht = NULL;
static XType static_fundamental_next = X_TYPE_RESERVED_USER_FIRST;
static TypeNode *static_fundamental_type_nodes[(X_TYPE_FUNDAMENTAL_MAX >> X_TYPE_FUNDAMENTAL_SHIFT) + 1] = { NULL, };

static inline TypeNode *lookup_type_node_I(XType utype)
{
    if (utype > X_TYPE_FUNDAMENTAL_MAX) {
        return (TypeNode *)(utype & ~TYPE_ID_MASK);
    } else {
        return static_fundamental_type_nodes[utype >> X_TYPE_FUNDAMENTAL_SHIFT];
    }
}

xuint x_type_get_type_registration_serial(void)
{
    return (xuint)x_atomic_int_get((xint *)&type_registration_serial);
}

static TypeNode *type_node_any_new_W(TypeNode *pnode, XType ftype, const xchar *name, XTypePlugin *plugin, XTypeFundamentalFlags type_flags)
{
    XType type;
    TypeNode *node;
    xuint n_supers;
    xuint i, node_size = 0;

    n_supers = pnode ? pnode->n_supers + 1 : 0;
    if (!pnode) {
        node_size += SIZEOF_FUNDAMENTAL_INFO;
    }

    node_size += SIZEOF_BASE_TYPE_NODE();
    node_size += (sizeof(XType) * (1 + n_supers + 1));
    node = (TypeNode *)x_malloc0(node_size);

    if (!pnode) {
        node = (TypeNode *)X_STRUCT_MEMBER_P(node, SIZEOF_FUNDAMENTAL_INFO);
        static_fundamental_type_nodes[ftype >> X_TYPE_FUNDAMENTAL_SHIFT] = node;
        type = ftype;
    } else {
        type = XPOINTER_TO_TYPE(node);
    }

    x_assert((type & TYPE_ID_MASK) == 0);

    node->n_supers = n_supers;
    if (!pnode) {
        node->supers[0] = type;
        node->supers[1] = 0;

        node->is_abstract = (type_flags & X_TYPE_FLAG_ABSTRACT) != 0;
        node->is_classed = (type_flags & X_TYPE_FLAG_CLASSED) != 0;
        node->is_deprecated = (type_flags & X_TYPE_FLAG_DEPRECATED) != 0;
        node->is_instantiatable = (type_flags & X_TYPE_FLAG_INSTANTIATABLE) != 0;

        if (NODE_IS_IFACE(node)) {
            IFACE_NODE_N_PREREQUISITES(node) = 0;
            IFACE_NODE_PREREQUISITES(node) = NULL;
        } else {
            _x_atomic_array_init(CLASSED_NODE_IFACES_ENTRIES(node));
        }
    } else     {
        node->supers[0] = type;
        memcpy(node->supers + 1, pnode->supers, sizeof(XType) * (1 + pnode->n_supers + 1));

        node->is_abstract = (type_flags & X_TYPE_FLAG_ABSTRACT) != 0;
        node->is_classed = pnode->is_classed;
        node->is_deprecated = (type_flags & X_TYPE_FLAG_DEPRECATED) != 0;
        node->is_instantiatable = pnode->is_instantiatable;

        node->is_deprecated |= pnode->is_deprecated;

        if (NODE_IS_IFACE(node)) {
            IFACE_NODE_N_PREREQUISITES(node) = 0;
            IFACE_NODE_PREREQUISITES(node) = NULL;
        } else {
            xuint j;
            IFaceEntries *entries;

            entries = (IFaceEntries *)_x_atomic_array_copy(CLASSED_NODE_IFACES_ENTRIES(pnode), IFACE_ENTRIES_HEADER_SIZE, 0);
            if (entries) {
                for (j = 0; j < IFACE_ENTRIES_N_ENTRIES(entries); j++) {
                    entries->entry[j].vtable = NULL;
                    entries->entry[j].init_state = UNINITIALIZED;
                }

                _x_atomic_array_update(CLASSED_NODE_IFACES_ENTRIES(node), entries);
            }
        }

        i = pnode->n_children++;
        pnode->children = x_renew(XType, pnode->children, pnode->n_children);
        pnode->children[i] = type;
    }

    TRACE(GOBJECT_TYPE_NEW(name, node->supers[1], type));

    node->plugin = plugin;
    node->n_children = 0;
    node->children = NULL;
    node->data = NULL;
    node->qname = x_quark_from_string(name);
    node->global_gdata = NULL;
    x_hash_table_insert(static_type_nodes_ht, (xpointer)x_quark_to_string(node->qname), XTYPE_TO_POINTER(type));

    x_atomic_int_inc((xint *)&type_registration_serial);

    return node;
}

static inline XTypeFundamentalInfo *type_node_fundamental_info_I(TypeNode *node)
{
    XType ftype = NODE_FUNDAMENTAL_TYPE(node);

    if (ftype != NODE_TYPE(node)) {
        node = lookup_type_node_I(ftype);
    }

    return node ? (XTypeFundamentalInfo *)X_STRUCT_MEMBER_P(node, -SIZEOF_FUNDAMENTAL_INFO) : NULL;
}

static TypeNode *type_node_fundamental_new_W(XType ftype, const xchar *name, XTypeFundamentalFlags type_flags)
{
    TypeNode *node;
    XTypeFundamentalInfo *finfo;

    x_assert((ftype & TYPE_ID_MASK) == 0);
    x_assert(ftype <= X_TYPE_FUNDAMENTAL_MAX);
    
    if (ftype >> X_TYPE_FUNDAMENTAL_SHIFT == static_fundamental_next) {
        static_fundamental_next++;
    }

    node = type_node_any_new_W(NULL, ftype, name, NULL, type_flags);
    finfo = type_node_fundamental_info_I(node);
    finfo->type_flags = type_flags & TYPE_FUNDAMENTAL_FLAG_MASK;

    return node;
}

static TypeNode *type_node_new_W(TypeNode *pnode, const xchar *name, XTypePlugin *plugin)
{
    x_assert(pnode);
    x_assert(pnode->n_supers < MAX_N_SUPERS);
    x_assert(pnode->n_children < MAX_N_CHILDREN);

    return type_node_any_new_W(pnode, NODE_FUNDAMENTAL_TYPE(pnode), name, plugin, (XTypeFundamentalFlags)0);
}

static inline IFaceEntry *lookup_iface_entry_I(IFaceEntries *entries, TypeNode *iface_node)
{
    xsize index;
    xuint8 *offsets;
    IFaceEntry *check;
    IFaceEntry *entry;
    xsize offset_index;

    if (entries == NULL) {
        return NULL;
    }

    X_ATOMIC_ARRAY_DO_TRANSACTION(&iface_node->_prot.offsets, xuint8,
        entry = NULL;
        offsets = transaction_data;
        offset_index = entries->offset_index;
        if (offsets != NULL && offset_index < X_ATOMIC_ARRAY_DATA_SIZE(offsets)) {
            index = offsets[offset_index];
            if (index > 0) {
                index -= 1;

                if (index < IFACE_ENTRIES_N_ENTRIES(entries)) {
                    check = (IFaceEntry *)&entries->entry[index];
                    if (check->iface_type == NODE_TYPE(iface_node)) {
                        entry = check;
                    }
                }
            }
        }
    );

    return entry;
}

static inline IFaceEntry *type_lookup_iface_entry_L(TypeNode *node, TypeNode *iface_node)
{
    if (!NODE_IS_IFACE(iface_node)) {
        return NULL;
    }

    return lookup_iface_entry_I(CLASSED_NODE_IFACES_ENTRIES_LOCKED(node), iface_node);
}

static inline xboolean type_lookup_iface_vtable_I(TypeNode *node, TypeNode *iface_node, xpointer *vtable_ptr)
{
    xboolean res;
    IFaceEntry *entry;

    if (!NODE_IS_IFACE(iface_node)) {
        if (vtable_ptr) {
            *vtable_ptr = NULL;
        }

        return FALSE;
    }

    X_ATOMIC_ARRAY_DO_TRANSACTION
        (CLASSED_NODE_IFACES_ENTRIES(node), IFaceEntries,

        entry = lookup_iface_entry_I(transaction_data, iface_node);
        res = entry != NULL;
        if (vtable_ptr) {
            if (entry) {
                *vtable_ptr = entry->vtable;
            } else {
                *vtable_ptr = NULL;
            }
        }
    );

    return res;
}

static inline xboolean type_lookup_prerequisite_L(TypeNode *iface, XType prerequisite_type)
{
    if (NODE_IS_IFACE(iface) && IFACE_NODE_N_PREREQUISITES(iface)) {
        XType *prerequisites = IFACE_NODE_PREREQUISITES(iface) - 1;
        xuint n_prerequisites = IFACE_NODE_N_PREREQUISITES(iface);

        do {
            xuint i;
            XType *check;

            i = (n_prerequisites + 1) >> 1;
            check = prerequisites + i;
            if (prerequisite_type == *check) {
                return TRUE;
            } else if (prerequisite_type > *check) {
                n_prerequisites -= i;
                prerequisites = check;
            } else {
                n_prerequisites = i - 1;
            }
        } while (n_prerequisites);
    }

    return FALSE;
}

static const xchar *type_descriptive_name_I(XType type)
{
    if (type) {
        TypeNode *node = lookup_type_node_I(type);
        return node ? NODE_NAME(node) : "<unknown>";
    } else {
        return "<invalid>";
    }
}

static xboolean check_plugin_U(XTypePlugin *plugin, xboolean need_complete_type_info, xboolean need_complete_interface_info, const xchar *type_name)
{
    if (!plugin) {
        x_critical("plugin handle for type '%s' is NULL", type_name);
        return FALSE;
    }

    if (!X_IS_TYPE_PLUGIN(plugin)) {
        x_critical("plugin pointer (%p) for type '%s' is invalid", plugin, type_name);
        return FALSE;
    
}
    if (need_complete_type_info && !X_TYPE_PLUGIN_GET_CLASS(plugin)->complete_type_info) {
        x_critical("plugin for type '%s' has no complete_type_info() implementation", type_name);
        return FALSE;
    }

    if (need_complete_interface_info && !X_TYPE_PLUGIN_GET_CLASS(plugin)->complete_interface_info) {
        x_critical("plugin for type '%s' has no complete_interface_info() implementation", type_name);
        return FALSE;
    }

    return TRUE;
}

static xboolean check_type_name_I(const xchar *type_name)
{
    xboolean name_valid;
    const xchar *p = type_name;
    static const xchar extra_chars[] = "-_+";

    if (!type_name[0] || !type_name[1] || !type_name[2]) {
        x_critical("type name '%s' is too short", type_name);
        return FALSE;
    }

    name_valid = (p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z') || p[0] == '_';
    for (p = type_name + 1; *p; p++) {
        name_valid &= ((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z') || (p[0] >= '0' && p[0] <= '9') || strchr(extra_chars, p[0]));
    }

    if (!name_valid) {
        x_critical("type name '%s' contains invalid characters", type_name);
        return FALSE;
    }

    if (x_type_from_name(type_name)) {
        x_critical("cannot register existing type '%s'", type_name);
        return FALSE;
    }

    return TRUE;
}

static xboolean check_derivation_I(XType parent_type, const xchar *type_name)
{
    TypeNode *pnode;
    XTypeFundamentalInfo* finfo;

    pnode = lookup_type_node_I(parent_type);
    if (!pnode) {
        x_critical("cannot derive type '%s' from invalid parent type '%s'", type_name, type_descriptive_name_I(parent_type));
        return FALSE;
    }

    if (pnode->is_final) {
        x_critical("cannot derive '%s' from final parent type '%s'", type_name, NODE_NAME(pnode));
        return FALSE;
    }

    finfo = type_node_fundamental_info_I(pnode);
    if (!(finfo->type_flags & X_TYPE_FLAG_DERIVABLE)) {
        x_critical("cannot derive '%s' from non-derivable parent type '%s'", type_name, NODE_NAME(pnode));
        return FALSE;
    }

    if (parent_type != NODE_FUNDAMENTAL_TYPE(pnode) && !(finfo->type_flags & X_TYPE_FLAG_DEEP_DERIVABLE)) {
        x_critical("cannot derive '%s' from non-fundamental parent type '%s'", type_name, NODE_NAME(pnode));
        return FALSE;
    }

    return TRUE;
}

static xboolean check_collect_format_I(const xchar *collect_format)
{
    const xchar *p = collect_format;
    xchar valid_format[] = { X_VALUE_COLLECT_INT, X_VALUE_COLLECT_LONG, X_VALUE_COLLECT_INT64, X_VALUE_COLLECT_DOUBLE, X_VALUE_COLLECT_POINTER, 0 };
    
    while (*p) {
        if (!strchr(valid_format, *p++)) {
            return FALSE;
        }
    }

    return p - collect_format <= X_VALUE_COLLECT_FORMAT_MAX_LENGTH;
}

static xboolean check_value_table_I(const xchar *type_name, const XTypeValueTable *value_table)
{
    if (!value_table) {
        return FALSE;
    } else if (value_table->value_init == NULL) {
        if (value_table->value_free || value_table->value_copy
            || value_table->value_peek_pointer
            || value_table->collect_format || value_table->collect_value
            || value_table->lcopy_format || value_table->lcopy_value)
        {
            x_critical("cannot handle uninitializable values of type '%s'", type_name);
        }

        return FALSE;
    } else {
        if (!value_table->value_free) {

        }

        if (!value_table->value_copy) {
            x_critical("missing 'value_copy()' for type '%s'", type_name);
            return FALSE;
        }

        if ((value_table->collect_format || value_table->collect_value) && (!value_table->collect_format || !value_table->collect_value)) {
            x_critical("one of 'collect_format' and 'collect_value()' is unspecified for type '%s'", type_name);
            return FALSE;
        }

        if (value_table->collect_format && !check_collect_format_I(value_table->collect_format)) {
            x_critical("the '%s' specification for type '%s' is too long or invalid", "collect_format", type_name);
            return FALSE;
        }

        if ((value_table->lcopy_format || value_table->lcopy_value) && (!value_table->lcopy_format || !value_table->lcopy_value)) {
            x_critical("one of 'lcopy_format' and 'lcopy_value()' is unspecified for type '%s'", type_name);
            return FALSE;
        }

        if (value_table->lcopy_format && !check_collect_format_I(value_table->lcopy_format)) {
            x_critical("the '%s' specification for type '%s' is too long or invalid", "lcopy_format", type_name);
            return FALSE;
        }
    }

    return TRUE;
}

static xboolean check_type_info_I(TypeNode *pnode, XType ftype, const xchar *type_name, const XTypeInfo *info)
{
    xboolean is_interface = ftype == X_TYPE_INTERFACE;
    XTypeFundamentalInfo *finfo = type_node_fundamental_info_I(lookup_type_node_I(ftype));

    x_assert(ftype <= X_TYPE_FUNDAMENTAL_MAX && !(ftype & TYPE_ID_MASK));

    if (!(finfo->type_flags & X_TYPE_FLAG_INSTANTIATABLE) && (info->instance_size || info->instance_init)) {
        if (pnode) {
            x_critical("cannot instantiate '%s', derived from non-instantiatable parent type '%s'", type_name, NODE_NAME(pnode));
        } else {
            x_critical("cannot instantiate '%s' as non-instantiatable fundamental", type_name);
        }

        return FALSE;
    }

    if (!((finfo->type_flags & X_TYPE_FLAG_CLASSED) || is_interface) && (info->class_init || info->class_finalize || info->class_data || info->class_size || info->base_init || info->base_finalize)) {
        if (pnode) {
            x_critical("cannot create class for '%s', derived from non-classed parent type '%s'", type_name, NODE_NAME(pnode));
        } else {
            x_critical("cannot create class for '%s' as non-classed fundamental", type_name);
        }

        return FALSE;
    }

    if (is_interface && info->class_size < sizeof(XTypeInterface)) {
        x_critical("specified interface size for type '%s' is smaller than 'XTypeInterface' size", type_name);
        return FALSE;
    }

    if (finfo->type_flags & X_TYPE_FLAG_CLASSED) {
        if (info->class_size < sizeof(XTypeClass)) {
            x_critical("specified class size for type '%s' is smaller than 'XTypeClass' size", type_name);
            return FALSE;
        }

        if (pnode && info->class_size < pnode->data->classt.class_size) {
            x_critical("specified class size for type '%s' is smaller than the parent type's '%s' class size", type_name, NODE_NAME(pnode));
            return FALSE;
        }
    }

    if (finfo->type_flags & X_TYPE_FLAG_INSTANTIATABLE) {
        if (info->instance_size < sizeof(XTypeInstance)) {
            x_critical("specified instance size for type '%s' is smaller than 'XTypeInstance' size", type_name);
            return FALSE;
        }

        if (pnode && info->instance_size < pnode->data->instance.instance_size) {
            x_critical("specified instance size for type '%s' is smaller than the parent type's '%s' instance size", type_name, NODE_NAME(pnode));
            return FALSE;
        }
    }

    return TRUE;
}

static TypeNode *find_conforming_child_type_L(TypeNode *pnode, TypeNode *iface)
{
    xuint i;
    TypeNode *node = NULL;

    if (type_lookup_iface_entry_L(pnode, iface)) {
        return pnode;
    }
    
    for (i = 0; i < pnode->n_children && !node; i++) {
        node = find_conforming_child_type_L(lookup_type_node_I(pnode->children[i]), iface);
    }

    return node;
}

static xboolean check_add_interface_L(XType instance_type, XType iface_type)
{
    xuint i;
    TypeNode *tnode;
    IFaceEntry *entry;
    XType *prerequisites;
    TypeNode *iface = lookup_type_node_I(iface_type);
    TypeNode *node = lookup_type_node_I(instance_type);

    if (!node || !node->is_instantiatable) {
        x_critical("cannot add interfaces to invalid (non-instantiatable) type '%s'", type_descriptive_name_I(instance_type));
        return FALSE;
    }

    if (!iface || !NODE_IS_IFACE(iface)) {
        x_critical("cannot add invalid (non-interface) type '%s' to type '%s'", type_descriptive_name_I(iface_type), NODE_NAME(node));
        return FALSE;
    }

    if (node->data && node->data->classt.classt) {
        x_critical("attempting to add an interface (%s) to class (%s) after class_init", NODE_NAME(iface), NODE_NAME(node));
        return FALSE;
    }

    tnode = lookup_type_node_I(NODE_PARENT_TYPE (iface));
    if (NODE_PARENT_TYPE (tnode) && !type_lookup_iface_entry_L(node, tnode)) {
        x_critical("cannot add sub-interface '%s' to type '%s' which does not conform to super-interface '%s'", NODE_NAME(iface), NODE_NAME(node), NODE_NAME(tnode));
        return FALSE;
    }

    entry = type_lookup_iface_entry_L(node, iface);
    if (entry && entry->vtable == NULL && !type_iface_peek_holder_L(iface, NODE_TYPE(node))) {
        return TRUE;
    }

    tnode = find_conforming_child_type_L(node, iface);
    if (tnode) {
        x_critical("cannot add interface type '%s' to type '%s', since type '%s' already conforms to interface", NODE_NAME(iface), NODE_NAME(node), NODE_NAME(tnode));
        return FALSE;
    }

    prerequisites = IFACE_NODE_PREREQUISITES(iface);
    for (i = 0; i < IFACE_NODE_N_PREREQUISITES(iface); i++) {
        tnode = lookup_type_node_I(prerequisites[i]);
        if (!type_node_is_a_L (node, tnode)) {
            x_critical("cannot add interface type '%s' to type '%s' which does not conform to prerequisite '%s'", NODE_NAME(iface), NODE_NAME(node), NODE_NAME(tnode));
            return FALSE;
        }
    }

    return TRUE;
}

static xboolean check_interface_info_I(TypeNode *iface, XType instance_type, const XInterfaceInfo *info)
{
    if ((info->interface_finalize || info->interface_data) && !info->interface_init) {
        x_critical("interface type '%s' for type '%s' comes without initializer", NODE_NAME(iface), type_descriptive_name_I(instance_type));
        return FALSE;
    }

    return TRUE;
}

static void type_data_make_W(TypeNode *node, const XTypeInfo *info, const XTypeValueTable *value_table)
{
    TypeData *data;
    size_t vtable_size = 0;
    XTypeValueTable *vtable = NULL;

    x_assert(node->data == NULL && info != NULL);

    if (!value_table) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE (node));
        if (pnode) {
            vtable = pnode->data->common.value_table;
        } else {
            static const XTypeValueTable zero_vtable = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
            value_table = &zero_vtable;
        }
    }

    if (value_table) {
        vtable_size = sizeof(XTypeValueTable);
        if (value_table->collect_format) {
            vtable_size += strlen(value_table->collect_format);
        }

        if (value_table->lcopy_format) {
            vtable_size += strlen(value_table->lcopy_format);
        }

        vtable_size += 2;
    }

    if (node->is_instantiatable) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE (node));

        data = (TypeData *)x_malloc0(sizeof(InstanceData) + vtable_size);
        if (vtable_size) {
            vtable = (XTypeValueTable *)X_STRUCT_MEMBER_P(data, sizeof(InstanceData));
        }

        data->instance.class_size = info->class_size;
        data->instance.class_init_base = info->base_init;
        data->instance.class_finalize_base = info->base_finalize;
        data->instance.class_init = info->class_init;
        data->instance.class_finalize = info->class_finalize;
        data->instance.class_data = info->class_data;
        data->instance.classt = NULL;
        data->instance.init_state = UNINITIALIZED;
        data->instance.instance_size = info->instance_size;

        data->instance.private_size = 0;
        data->instance.class_private_size = 0;
        if (pnode) {
            data->instance.class_private_size = pnode->data->instance.class_private_size;
        }

        data->instance.instance_init = info->instance_init;
    } else if (node->is_classed) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE (node));

        data = (TypeData *)x_malloc0(sizeof(ClassData) + vtable_size);
        if (vtable_size) {
            vtable = (XTypeValueTable *)X_STRUCT_MEMBER_P(data, sizeof(ClassData));
        }

        data->classt.class_size = info->class_size;
        data->classt.class_init_base = info->base_init;
        data->classt.class_finalize_base = info->base_finalize;
        data->classt.class_init = info->class_init;
        data->classt.class_finalize = info->class_finalize;
        data->classt.class_data = info->class_data;
        data->classt.classt = NULL;
        data->classt.class_private_size = 0;
        if (pnode) {
            data->classt.class_private_size = pnode->data->classt.class_private_size;
        }

        data->classt.init_state = UNINITIALIZED;
    } else if (NODE_IS_IFACE(node)) {
        data = (TypeData *)x_malloc0(sizeof(IFaceData) + vtable_size);
        if (vtable_size) {
            vtable = (XTypeValueTable *)X_STRUCT_MEMBER_P(data, sizeof(IFaceData));
        }

        data->iface.vtable_size = info->class_size;
        data->iface.vtable_init_base = info->base_init;
        data->iface.vtable_finalize_base = info->base_finalize;
        data->iface.dflt_init = info->class_init;
        data->iface.dflt_finalize = info->class_finalize;
        data->iface.dflt_data = info->class_data;
        data->iface.dflt_vtable = NULL;
    } else if (NODE_IS_BOXED(node)) {
        data = (TypeData *)x_malloc0(sizeof(BoxedData) + vtable_size);
        if (vtable_size) {
            vtable = (XTypeValueTable *)X_STRUCT_MEMBER_P(data, sizeof(BoxedData));
        }
    } else {
        data = (TypeData *)x_malloc0(sizeof(CommonData) + vtable_size);
        if (vtable_size) {
            vtable = (XTypeValueTable *)X_STRUCT_MEMBER_P(data, sizeof(CommonData));
        }
    }

    node->data = data;
    if (vtable_size) {
        xchar *p;

        *vtable = *value_table;
        p = (xchar *)X_STRUCT_MEMBER_P(vtable, sizeof(*vtable));
        p[0] = 0;
        vtable->collect_format = p;
        if (value_table->collect_format) {
            strcat (p, value_table->collect_format);
            p += strlen(value_table->collect_format);
        }

        p++;
        p[0] = 0;
        vtable->lcopy_format = p;
        if (value_table->lcopy_format) {
            strcat(p, value_table->lcopy_format);
        }
    }

    node->data->common.value_table = vtable;
    x_assert(node->data->common.value_table != NULL);
    node->mutatable_check_cache = (node->data->common.value_table->value_init != NULL && !((X_TYPE_FLAG_VALUE_ABSTRACT | X_TYPE_FLAG_ABSTRACT) & XPOINTER_TO_UINT(type_get_qdata_L(node, static_quark_type_flags))));

    x_atomic_int_set((int *)&node->ref_count, 1);
}

static inline void type_data_ref_Wm(TypeNode *node)
{
    if (!node->data) {
        XTypeInfo tmp_info;
        XTypeValueTable tmp_value_table;
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE(node));

        x_assert(node->plugin != NULL);

        if (pnode) {
            type_data_ref_Wm(pnode);
            if (node->data) {
                INVALID_RECURSION("x_type_plugin_*", node->plugin, NODE_NAME(node));
            }
        }

        memset(&tmp_info, 0, sizeof(tmp_info));
        memset(&tmp_value_table, 0, sizeof(tmp_value_table));

        X_WRITE_UNLOCK(&type_rw_lock);
        x_type_plugin_use(node->plugin);
        x_type_plugin_complete_type_info(node->plugin, NODE_TYPE(node), &tmp_info, &tmp_value_table);

        X_WRITE_LOCK(&type_rw_lock);
        if (node->data) {
            INVALID_RECURSION ("x_type_plugin_*", node->plugin, NODE_NAME(node));
        }

        check_type_info_I(pnode, NODE_FUNDAMENTAL_TYPE(node), NODE_NAME(node), &tmp_info);
        type_data_make_W(node, &tmp_info, check_value_table_I(NODE_NAME(node), &tmp_value_table) ? &tmp_value_table : NULL);
    } else {
        x_assert(NODE_REFCOUNT(node) > 0);
        x_atomic_int_inc((int *)&node->ref_count);
    }
}

static inline xboolean type_data_ref_U(TypeNode *node)
{
    xuint current;

    do {
        current = NODE_REFCOUNT(node);
        if (current < 1) {
            return FALSE;
        }
    } while (!x_atomic_int_compare_and_exchange((int *) &node->ref_count, current, current + 1));

    return TRUE;
}

static xboolean iface_node_has_available_offset_L(TypeNode *iface_node, xsize offset, int for_index)
{
    xuint8 *offsets;

    offsets = X_ATOMIC_ARRAY_GET_LOCKED(&iface_node->_prot.offsets, xuint8);
    if (offsets == NULL) {
        return TRUE;
    }

    if (X_ATOMIC_ARRAY_DATA_SIZE(offsets) <= offset) {
        return TRUE;
    }

    if (offsets[offset] == 0 || offsets[offset] == for_index+1) {
        return TRUE;
    }

    return FALSE;
}

static xsize find_free_iface_offset_L(IFaceEntries *entries)
{
    int i;
    xsize offset;
    int n_entries;
    IFaceEntry *entry;
    TypeNode *iface_node;

    n_entries = IFACE_ENTRIES_N_ENTRIES(entries);
    offset = 0;

    do {
        for (i = 0; i < n_entries; i++) {
            entry = &entries->entry[i];
            iface_node = lookup_type_node_I(entry->iface_type);

            if (!iface_node_has_available_offset_L(iface_node, offset, i)) {
                offset++;
                break;
            }
        }
    } while (i != n_entries);

    return offset;
}

static void iface_node_set_offset_L(TypeNode *iface_node, xsize offset, int index)
{
    xsize i;
    xsize new_size, old_size;
    xuint8 *offsets, *old_offsets;

    old_offsets = X_ATOMIC_ARRAY_GET_LOCKED(&iface_node->_prot.offsets, xuint8);
    if (old_offsets == NULL) {
        old_size = 0;
    } else {
        old_size = X_ATOMIC_ARRAY_DATA_SIZE(old_offsets);
        if (offset < old_size && old_offsets[offset] == index + 1) {
            return;
        }
    }

    new_size = MAX(old_size, offset + 1);
    offsets = (xuint8 *)_x_atomic_array_copy(&iface_node->_prot.offsets, 0, new_size - old_size);

    for (i = old_size; i < new_size; i++) {
        offsets[i] = 0;
    }
    offsets[offset] = index + 1;

    _x_atomic_array_update(&iface_node->_prot.offsets, offsets);
}

static void type_node_add_iface_entry_W(TypeNode *node, XType iface_type, IFaceEntry *parent_entry)
{
    xuint i, j;
    xuint num_entries;
    IFaceEntry *entry;
    TypeNode *iface_node;
    IFaceEntries *entries;

    x_assert(node->is_instantiatable);

    entries = CLASSED_NODE_IFACES_ENTRIES_LOCKED(node);
    if (entries != NULL) {
        num_entries = IFACE_ENTRIES_N_ENTRIES(entries);
        x_assert(num_entries < MAX_N_INTERFACES);

        for (i = 0; i < num_entries; i++) {
            entry = &entries->entry[i];
            if (entry->iface_type == iface_type) {
                if (!parent_entry) {
                    x_assert(entry->vtable == NULL && entry->init_state == UNINITIALIZED);
                } else {

                }

                return;
            }
        }
    }

    entries = (IFaceEntries *)_x_atomic_array_copy(CLASSED_NODE_IFACES_ENTRIES(node), IFACE_ENTRIES_HEADER_SIZE, sizeof(IFaceEntry));
    num_entries = IFACE_ENTRIES_N_ENTRIES (entries);
    i = num_entries - 1;
    if (i == 0) {
        entries->offset_index = 0;
    }

    entries->entry[i].iface_type = iface_type;
    entries->entry[i].vtable = NULL;
    entries->entry[i].init_state = UNINITIALIZED;

    if (parent_entry) {
        if (node->data && x_atomic_int_get(&node->data->classt.init_state) >= BASE_IFACE_INIT) {
            entries->entry[i].init_state = INITIALIZED;
            entries->entry[i].vtable = parent_entry->vtable;
        }
    }

    iface_node = lookup_type_node_I(iface_type);
    if (iface_node_has_available_offset_L(iface_node, entries->offset_index, i)) {
        iface_node_set_offset_L(iface_node, entries->offset_index, i);
    } else {
        entries->offset_index = find_free_iface_offset_L(entries);
        for (j = 0; j < IFACE_ENTRIES_N_ENTRIES(entries); j++) {
            entry = &entries->entry[j];
            iface_node = lookup_type_node_I(entry->iface_type);
            iface_node_set_offset_L(iface_node, entries->offset_index, j);
        }
    }

    _x_atomic_array_update(CLASSED_NODE_IFACES_ENTRIES(node), entries);
    if (parent_entry) {
        for (i = 0; i < node->n_children; i++) {
            type_node_add_iface_entry_W(lookup_type_node_I(node->children[i]), iface_type, &entries->entry[i]);
        }
    }
}

static void type_add_interface_Wm(TypeNode *node, TypeNode *iface, const XInterfaceInfo *info, XTypePlugin *plugin)
{
    xuint i;
    IFaceEntry *entry;
    IFaceHolder *iholder = x_new0(IFaceHolder, 1);

    x_assert(node->is_instantiatable && NODE_IS_IFACE(iface) && ((info && !plugin) || (!info && plugin)));

    iholder->next = iface_node_get_holders_L(iface);
    iface_node_set_holders_W (iface, iholder);
    iholder->instance_type = NODE_TYPE(node);
    iholder->info = info ? (XInterfaceInfo *)x_memdup2(info, sizeof(*info)) : NULL;
    iholder->plugin = plugin;

    type_node_add_iface_entry_W(node, NODE_TYPE(iface), NULL);

    if (node->data) {
        InitState class_state = (InitState)x_atomic_int_get(&node->data->classt.init_state);

        if (class_state >= BASE_IFACE_INIT) {
            type_iface_vtable_base_init_Wm(iface, node);
        }

        if (class_state >= IFACE_INIT) {
            type_iface_vtable_iface_init_Wm(iface, node);
        }
    }

    entry = type_lookup_iface_entry_L(node, iface);
    for (i = 0; i < node->n_children; i++) {
        type_node_add_iface_entry_W(lookup_type_node_I(node->children[i]), NODE_TYPE(iface), entry);
    }
}

static void type_iface_add_prerequisite_W(TypeNode *iface, TypeNode *prerequisite_node)
{
    xuint n_dependants, i;
    XType *prerequisites, *dependants;
    XType prerequisite_type = NODE_TYPE(prerequisite_node);

    x_assert(NODE_IS_IFACE(iface) && IFACE_NODE_N_PREREQUISITES(iface) < MAX_N_PREREQUISITES && (prerequisite_node->is_instantiatable || NODE_IS_IFACE(prerequisite_node)));
    
    prerequisites = IFACE_NODE_PREREQUISITES(iface);
    for (i = 0; i < IFACE_NODE_N_PREREQUISITES(iface); i++) {
        if (prerequisites[i] == prerequisite_type) {
            return;
        } else if (prerequisites[i] > prerequisite_type) {
            break;
        }
    }

    IFACE_NODE_N_PREREQUISITES(iface) += 1;
    IFACE_NODE_PREREQUISITES(iface) = x_renew(XType, IFACE_NODE_PREREQUISITES(iface), IFACE_NODE_N_PREREQUISITES(iface));
    prerequisites = IFACE_NODE_PREREQUISITES(iface);
    memmove(prerequisites + i + 1, prerequisites + i, sizeof(prerequisites[0]) * (IFACE_NODE_N_PREREQUISITES(iface) - i - 1));
    prerequisites[i] = prerequisite_type;

    if (NODE_IS_IFACE(prerequisite_node)) {
        dependants = iface_node_get_dependants_array_L(prerequisite_node);
        n_dependants = dependants ? dependants[0] : 0;
        n_dependants += 1;
        dependants = x_renew(XType, dependants, n_dependants + 1);
        dependants[n_dependants] = NODE_TYPE(iface);
        dependants[0] = n_dependants;
        iface_node_set_dependants_array_W(prerequisite_node, dependants);
    }

    dependants = iface_node_get_dependants_array_L(iface);
    n_dependants = dependants ? dependants[0] : 0;
    for (i = 1; i <= n_dependants; i++) {
        type_iface_add_prerequisite_W(lookup_type_node_I(dependants[i]), prerequisite_node);
    }
}

void x_type_interface_add_prerequisite(XType interface_type, XType prerequisite_type)
{
    IFaceHolder *holders;
    TypeNode *iface, *prerequisite_node;

    x_return_if_fail(X_TYPE_IS_INTERFACE(interface_type));
    x_return_if_fail(!x_type_is_a(interface_type, prerequisite_type));
    x_return_if_fail(!x_type_is_a(prerequisite_type, interface_type));
    
    iface = lookup_type_node_I(interface_type);
    prerequisite_node = lookup_type_node_I(prerequisite_type);
    if (!iface || !prerequisite_node || !NODE_IS_IFACE(iface)) {
        x_critical("interface type '%s' or prerequisite type '%s' invalid", type_descriptive_name_I(interface_type), type_descriptive_name_I(prerequisite_type));
        return;
    }

    X_WRITE_LOCK(&type_rw_lock);
    holders = iface_node_get_holders_L(iface);
    if (holders) {
        X_WRITE_UNLOCK(&type_rw_lock);
        x_critical("unable to add prerequisite '%s' to interface '%s' which is already in use for '%s'", type_descriptive_name_I(prerequisite_type), type_descriptive_name_I(interface_type), type_descriptive_name_I(holders->instance_type));
        return;
    }

    if (prerequisite_node->is_instantiatable) {
        xuint i;

        for (i = 0; i < IFACE_NODE_N_PREREQUISITES(iface); i++) {
            TypeNode *prnode = lookup_type_node_I(IFACE_NODE_PREREQUISITES(iface)[i]);

            if (prnode->is_instantiatable) {
                X_WRITE_UNLOCK(&type_rw_lock);
                x_critical("adding prerequisite '%s' to interface '%s' conflicts with existing prerequisite '%s'", type_descriptive_name_I(prerequisite_type), type_descriptive_name_I(interface_type), type_descriptive_name_I(NODE_TYPE(prnode)));
                return;
            }
        }

        for (i = 0; i < prerequisite_node->n_supers + 1u; i++) {
            type_iface_add_prerequisite_W(iface, lookup_type_node_I(prerequisite_node->supers[i]));
        }

        X_WRITE_UNLOCK(&type_rw_lock);
    } else if (NODE_IS_IFACE(prerequisite_node)) {
        xuint i;
        XType *prerequisites;

        prerequisites = IFACE_NODE_PREREQUISITES(prerequisite_node);
        for (i = 0; i < IFACE_NODE_N_PREREQUISITES(prerequisite_node); i++) {
            type_iface_add_prerequisite_W(iface, lookup_type_node_I(prerequisites[i]));
        }

        type_iface_add_prerequisite_W(iface, prerequisite_node);
        X_WRITE_UNLOCK(&type_rw_lock);
    } else {
        X_WRITE_UNLOCK(&type_rw_lock);
        x_critical("prerequisite '%s' for interface '%s' is neither instantiatable nor interface", type_descriptive_name_I(prerequisite_type), type_descriptive_name_I(interface_type));
    }
}

XType *x_type_interface_prerequisites(XType interface_type, xuint *n_prerequisites)
{
    TypeNode *iface;

    x_return_val_if_fail(X_TYPE_IS_INTERFACE(interface_type), NULL);

    iface = lookup_type_node_I(interface_type);
    if (iface) {
        XType *types;
        xuint i, n = 0;
        TypeNode *inode = NULL;

        X_READ_LOCK(&type_rw_lock);
        types = x_new0(XType, IFACE_NODE_N_PREREQUISITES(iface) + 1);
        for (i = 0; i < IFACE_NODE_N_PREREQUISITES(iface); i++) {
            XType prerequisite = IFACE_NODE_PREREQUISITES(iface)[i];
            TypeNode *node = lookup_type_node_I(prerequisite);
            if (node->is_instantiatable) {
                if (!inode || type_node_is_a_L(node, inode)) {
                    inode = node;
                }
            } else {
                types[n++] = NODE_TYPE(node);
            }
        }

        if (inode) {
            types[n++] = NODE_TYPE(inode);
        }

        if (n_prerequisites) {
            *n_prerequisites = n;
        }
        X_READ_UNLOCK(&type_rw_lock);

        return types;
    } else {
        if (n_prerequisites) {
            *n_prerequisites = 0;
        }

        return NULL;
    }
}

XType x_type_interface_instantiatable_prerequisite(XType interface_type)
{
    xuint i;
    TypeNode *iface;
    TypeNode *inode = NULL;

    x_return_val_if_fail(X_TYPE_IS_INTERFACE(interface_type), X_TYPE_INVALID);

    iface = lookup_type_node_I(interface_type);
    if (iface == NULL) {
        return X_TYPE_INVALID;
    }

    X_READ_LOCK(&type_rw_lock);

    for (i = 0; i < IFACE_NODE_N_PREREQUISITES(iface); i++) {
        XType prerequisite = IFACE_NODE_PREREQUISITES(iface)[i];
        TypeNode *node = lookup_type_node_I(prerequisite);
        if (node->is_instantiatable) {
            if (!inode || type_node_is_a_L(node, inode)) {
                inode = node;
            }
        }
    }

    X_READ_UNLOCK(&type_rw_lock);

    if (inode) {
        return NODE_TYPE(inode);
    } else {
        return X_TYPE_INVALID;
    }
}

static IFaceHolder *type_iface_peek_holder_L(TypeNode *iface, XType instance_type)
{
    IFaceHolder *iholder;

    x_assert(NODE_IS_IFACE(iface));

    iholder = iface_node_get_holders_L(iface);
    while (iholder && iholder->instance_type != instance_type) {
        iholder = iholder->next;
    }

    return iholder;
}

static IFaceHolder *type_iface_retrieve_holder_info_Wm(TypeNode *iface, XType instance_type, xboolean need_info)
{
    IFaceHolder *iholder = type_iface_peek_holder_L(iface, instance_type);
    if (iholder && !iholder->info && need_info) {
        XInterfaceInfo tmp_info;

        x_assert(iholder->plugin != NULL);

        type_data_ref_Wm (iface);
        if (iholder->info) {
            INVALID_RECURSION ("x_type_plugin_*", iface->plugin, NODE_NAME(iface));
        }
        memset(&tmp_info, 0, sizeof(tmp_info));

        X_WRITE_UNLOCK(&type_rw_lock);
        x_type_plugin_use(iholder->plugin);
        x_type_plugin_complete_interface_info(iholder->plugin, instance_type, NODE_TYPE(iface), &tmp_info);
        X_WRITE_LOCK(&type_rw_lock);

        if (iholder->info) {
            INVALID_RECURSION ("x_type_plugin_*", iholder->plugin, NODE_NAME(iface));
        }

        check_interface_info_I(iface, instance_type, &tmp_info);
        iholder->info = (XInterfaceInfo *)x_memdup2(&tmp_info, sizeof(tmp_info));
    }

    return iholder;
}

static void type_iface_blow_holder_info_Wm(TypeNode *iface, XType instance_type)
{
    IFaceHolder *iholder = iface_node_get_holders_L(iface);

    x_assert(NODE_IS_IFACE(iface));

    while (iholder->instance_type != instance_type) {
        iholder = iholder->next;
    }

    if (iholder->info && iholder->plugin) {
        x_free(iholder->info);
        iholder->info = NULL;

        X_WRITE_UNLOCK(&type_rw_lock);
        x_type_plugin_unuse(iholder->plugin);
        type_data_unref_U(iface, FALSE);
        X_WRITE_LOCK(&type_rw_lock);
    }
}

static void maybe_issue_deprecation_warning(XType type)
{
    xboolean already;
    const char *name;
    static XMutex already_warned_lock;
    static const xchar *enable_diagnostic;
    static XHashTable *already_warned_table;

    if (x_once_init_enter_pointer(&enable_diagnostic)) {
        const xchar *value = x_getenv("X_ENABLE_DIAGNOSTIC");
        if (!value) {
            value = "0";
        }

        x_once_init_leave_pointer(&enable_diagnostic, value);
    }

    if (enable_diagnostic[0] == '0') {
        return;
    }

    x_mutex_lock(&already_warned_lock);
    if (already_warned_table == NULL) {
        already_warned_table = x_hash_table_new(NULL, NULL);
    }

    name = x_type_name(type);

    already = x_hash_table_contains(already_warned_table, (xpointer)name);
    if (!already) {
        x_hash_table_add(already_warned_table, (xpointer)name);
    }
    x_mutex_unlock(&already_warned_lock);

    if (!already) {
        x_warning("The type %s is deprecated and shouldn’t be used any more. It may be removed in a future version.", name);
    }
}

XTypeInstance *x_type_create_instance(XType type)
{
    xuint i;
    TypeNode *node;
    xint ivar_size;
    xchar *allocated;
    xint private_size;
    XTypeClass *classt;
    XTypeInstance *instance;

    node = lookup_type_node_I(type);
    if (X_UNLIKELY(!node || !node->is_instantiatable)) {
        x_error("cannot create new instance of invalid (non-instantiatable) type '%s'", type_descriptive_name_I(type));
    }

    if (X_UNLIKELY(!node->mutatable_check_cache && X_TYPE_IS_ABSTRACT(type))) {
        x_error("cannot create instance of abstract (non-instantiatable) type '%s'", type_descriptive_name_I(type));
    }

    if (X_UNLIKELY(X_TYPE_IS_DEPRECATED(type))) {
        maybe_issue_deprecation_warning(type);
    }

    classt = (XTypeClass *)x_type_class_ref(type);

    private_size = node->data->instance.private_size;
    ivar_size = node->data->instance.instance_size;

    allocated = (xchar *)x_malloc0(private_size + ivar_size);
    instance = (XTypeInstance *)(allocated + private_size);

    for (i = node->n_supers; i > 0; i--) {
        TypeNode *pnode;

        pnode = lookup_type_node_I(node->supers[i]);
        if (pnode->data->instance.instance_init) {
            instance->x_class = (XTypeClass *)pnode->data->instance.classt;
            pnode->data->instance.instance_init(instance, classt);
        }
    }

    instance->x_class = classt;
    if (node->data->instance.instance_init) {
        node->data->instance.instance_init(instance, classt);
    }

    TRACE(XOBJECT_OBJECT_NEW(instance, type));
    return instance;
}

void x_type_free_instance(XTypeInstance *instance)
{
    xint ivar_size;
    TypeNode *node;
    xchar *allocated;
    xint private_size;
    XTypeClass *classt;

    x_return_if_fail(instance != NULL && instance->x_class != NULL);

    classt = instance->x_class;
    node = lookup_type_node_I(classt->x_type);
    if (X_UNLIKELY(!node || !node->is_instantiatable || !node->data || node->data->classt.classt != (xpointer)classt)) {
        x_critical("cannot free instance of invalid (non-instantiatable) type '%s'", type_descriptive_name_I(classt->x_type));
        return;
    }

    if (X_UNLIKELY(!node->mutatable_check_cache && X_TYPE_IS_ABSTRACT(NODE_TYPE(node)))) {
        x_critical("cannot free instance of abstract (non-instantiatable) type '%s'", NODE_NAME(node));
        return;
    }

    instance->x_class = NULL;
    private_size = node->data->instance.private_size;
    ivar_size = node->data->instance.instance_size;
    allocated = ((xchar *)instance) - private_size;

    x_free_sized(allocated, private_size + ivar_size);
    x_type_class_unref(classt);
}

static void type_iface_ensure_dflt_vtable_Wm(TypeNode *iface)
{
    x_assert(iface->data);

    if (!iface->data->iface.dflt_vtable) {
        XTypeInterface *vtable = (XTypeInterface *)x_malloc0(iface->data->iface.vtable_size);

        iface->data->iface.dflt_vtable = vtable;
        vtable->x_type = NODE_TYPE(iface);
        vtable->x_instance_type = 0;

        if (iface->data->iface.vtable_init_base || iface->data->iface.dflt_init) {
            X_WRITE_UNLOCK(&type_rw_lock);
            if (iface->data->iface.vtable_init_base) {
                iface->data->iface.vtable_init_base(vtable);
            }

            if (iface->data->iface.dflt_init) {
                iface->data->iface.dflt_init(vtable, (xpointer)iface->data->iface.dflt_data);
            }
            X_WRITE_LOCK(&type_rw_lock);
        }
    }
}

static xboolean type_iface_vtable_base_init_Wm(TypeNode *iface, TypeNode *node)
{
    TypeNode *pnode;
    IFaceEntry *entry;
    IFaceHolder *iholder;
    XTypeInterface *vtable = NULL;

    iholder = type_iface_retrieve_holder_info_Wm(iface, NODE_TYPE(node), TRUE);
    if (!iholder) {
        return FALSE;
    }

    type_iface_ensure_dflt_vtable_Wm(iface);
    entry = type_lookup_iface_entry_L(node, iface);

    x_assert(iface->data && entry && entry->vtable == NULL && iholder && iholder->info);

    entry->init_state = IFACE_INIT;
    pnode = lookup_type_node_I(NODE_PARENT_TYPE (node));

    if (pnode) {
        IFaceEntry *pentry = type_lookup_iface_entry_L(pnode, iface);
        if (pentry) {
            vtable = (XTypeInterface *)x_memdup2(pentry->vtable, iface->data->iface.vtable_size);
        }
    }

    if (!vtable) {
        vtable = (XTypeInterface *)x_memdup2(iface->data->iface.dflt_vtable, iface->data->iface.vtable_size);
    }

    entry->vtable = vtable;
    vtable->x_type = NODE_TYPE(iface);
    vtable->x_instance_type = NODE_TYPE(node);

    if (iface->data->iface.vtable_init_base) {
        X_WRITE_UNLOCK(&type_rw_lock);
        iface->data->iface.vtable_init_base(vtable);
        X_WRITE_LOCK(&type_rw_lock);
    }

    return TRUE;
}

static void type_iface_vtable_iface_init_Wm(TypeNode *iface, TypeNode *node)
{
    xuint i;
    XTypeInterface *vtable = NULL;
    IFaceEntry *entry = type_lookup_iface_entry_L(node, iface);
    IFaceHolder *iholder = type_iface_peek_holder_L(iface, NODE_TYPE(node));

    x_assert(iface->data && entry && iholder && iholder->info);
    x_assert(entry->init_state == IFACE_INIT);

    entry->init_state = INITIALIZED;
    vtable = entry->vtable;

    if (iholder->info->interface_init) {
        X_WRITE_UNLOCK(&type_rw_lock);
        if (iholder->info->interface_init) {
            iholder->info->interface_init(vtable, iholder->info->interface_data);
        }
        X_WRITE_LOCK(&type_rw_lock);
    }

    for (i = 0; i < static_n_iface_check_funcs; i++) {
        XTypeInterfaceCheckFunc check_func = static_iface_check_funcs[i].check_func;
        xpointer check_data = static_iface_check_funcs[i].check_data;

        X_WRITE_UNLOCK(&type_rw_lock);
        check_func (check_data, (xpointer)vtable);
        X_WRITE_LOCK(&type_rw_lock);
    }
}

static xboolean type_iface_vtable_finalize_Wm(TypeNode *iface, TypeNode *node, XTypeInterface *vtable)
{
    IFaceHolder *iholder;
    IFaceEntry *entry = type_lookup_iface_entry_L(node, iface);

    iholder = type_iface_retrieve_holder_info_Wm(iface, NODE_TYPE(node), FALSE);
    if (!iholder) {
        return FALSE;
    }

    x_assert(entry && entry->vtable == vtable && iholder->info);

    entry->vtable = NULL;
    entry->init_state = UNINITIALIZED;
    if (iholder->info->interface_finalize || iface->data->iface.vtable_finalize_base) {
        X_WRITE_UNLOCK(&type_rw_lock);
        if (iholder->info->interface_finalize) {
            iholder->info->interface_finalize(vtable, iholder->info->interface_data);
        }

        if (iface->data->iface.vtable_finalize_base) {
            iface->data->iface.vtable_finalize_base(vtable);
        }
        X_WRITE_LOCK(&type_rw_lock);
    }

    vtable->x_type = 0;
    vtable->x_instance_type = 0;
    x_free(vtable);
    type_iface_blow_holder_info_Wm(iface, NODE_TYPE(node));

    return TRUE;
}

static void type_class_init_Wm(TypeNode *node, XTypeClass *pclass)
{
    xuint i;
    IFaceEntry *entry;
    XTypeClass *classt;
    IFaceEntries *entries;
    TypeNode *bnode, *pnode;
    XSList *slist, *init_slist = NULL;

    x_assert(node->is_classed && node->data && node->data->classt.class_size && !node->data->classt.classt && x_atomic_int_get(&node->data->classt.init_state) == UNINITIALIZED);

    if (node->data->classt.class_private_size) {
        classt = (XTypeClass *)x_malloc0(ALIGN_STRUCT(node->data->classt.class_size) + node->data->classt.class_private_size);
    } else {
        classt = (XTypeClass *)x_malloc0(node->data->classt.class_size);
    }

    node->data->classt.classt = classt;
    x_atomic_int_set(&node->data->classt.init_state, BASE_CLASS_INIT);

    if (pclass) {
        pnode = lookup_type_node_I(pclass->x_type);

        memcpy(classt, pclass, pnode->data->classt.class_size);
        memcpy(X_STRUCT_MEMBER_P(classt, ALIGN_STRUCT(node->data->classt.class_size)), X_STRUCT_MEMBER_P(pclass, ALIGN_STRUCT(pnode->data->classt.class_size)), pnode->data->classt.class_private_size);

        if (node->is_instantiatable) {
            node->data->instance.private_size = pnode->data->instance.private_size;
        }
    }
    classt->x_type = NODE_TYPE(node);

    X_WRITE_UNLOCK(&type_rw_lock);

    for (bnode = node; bnode; bnode = lookup_type_node_I(NODE_PARENT_TYPE(bnode))) {
        if (bnode->data->classt.class_init_base) {
            init_slist = x_slist_prepend(init_slist, (xpointer)bnode->data->classt.class_init_base);
        }
    }

    for (slist = init_slist; slist; slist = slist->next) {
        XBaseInitFunc class_init_base = (XBaseInitFunc)slist->data;
        class_init_base(classt);
    }

    x_slist_free(init_slist);
    X_WRITE_LOCK(&type_rw_lock);

    x_atomic_int_set(&node->data->classt.init_state, BASE_IFACE_INIT);

    pnode = lookup_type_node_I(NODE_PARENT_TYPE(node));
    i = 0;

    while ((entries = CLASSED_NODE_IFACES_ENTRIES_LOCKED(node)) != NULL && i < IFACE_ENTRIES_N_ENTRIES(entries)) {
        entry = &entries->entry[i];
        while (i < IFACE_ENTRIES_N_ENTRIES(entries) && entry->init_state == IFACE_INIT) {
            entry++;
            i++;
        }

        if (i == IFACE_ENTRIES_N_ENTRIES(entries)) {
            break;
        }

        if (!type_iface_vtable_base_init_Wm(lookup_type_node_I(entry->iface_type), node)) {
            xuint j;
            IFaceEntries *pentries = CLASSED_NODE_IFACES_ENTRIES_LOCKED(pnode);

            x_assert(pnode != NULL);

            if (pentries) {
                for (j = 0; j < IFACE_ENTRIES_N_ENTRIES(pentries); j++) {
                    IFaceEntry *pentry = &pentries->entry[j];
                    if (pentry->iface_type == entry->iface_type) {
                        entry->vtable = pentry->vtable;
                        entry->init_state = INITIALIZED;
                        break;
                    }
                }
            }

            x_assert(entry->vtable != NULL);
        }

        i++;
    }

    x_atomic_int_set(&node->data->classt.init_state, CLASS_INIT);
    X_WRITE_UNLOCK(&type_rw_lock);

    if (node->data->classt.class_init) {
        node->data->classt.class_init(classt, (xpointer)node->data->classt.class_data);
    }

    X_WRITE_LOCK(&type_rw_lock);
    x_atomic_int_set(&node->data->classt.init_state, IFACE_INIT);

    i = 0;
    while ((entries = CLASSED_NODE_IFACES_ENTRIES_LOCKED(node)) != NULL) {
        entry = &entries->entry[i];
        while (i < IFACE_ENTRIES_N_ENTRIES(entries) && entry->init_state == INITIALIZED) {
            entry++;
            i++;
        }

        if (i == IFACE_ENTRIES_N_ENTRIES(entries)) {
            break;
        }

        type_iface_vtable_iface_init_Wm(lookup_type_node_I(entry->iface_type), node);
        i++;
    }

    x_atomic_int_set(&node->data->classt.init_state, INITIALIZED);
}

static void type_data_finalize_class_ifaces_Wm(TypeNode *node)
{
    xuint i;
    IFaceEntries *entries;

    x_assert(node->is_instantiatable && node->data && node->data->classt.classt && NODE_REFCOUNT(node) == 0);

    reiterate:
    entries = CLASSED_NODE_IFACES_ENTRIES_LOCKED(node);
    for (i = 0; entries != NULL && i < IFACE_ENTRIES_N_ENTRIES(entries); i++) {
        IFaceEntry *entry = &entries->entry[i];
        if (entry->vtable) {
            if (type_iface_vtable_finalize_Wm(lookup_type_node_I(entry->iface_type), node, entry->vtable)) {
                goto reiterate;
            } else {
                entry->vtable = NULL;
                entry->init_state = UNINITIALIZED;
            }
        }
    }
}

static void type_data_finalize_class_U(TypeNode *node, ClassData *cdata)
{
    TypeNode *bnode;
    XTypeClass *classt = (XTypeClass *)cdata->classt;

    x_assert(cdata->classt && NODE_REFCOUNT (node) == 0);

    if (cdata->class_finalize) {
        cdata->class_finalize(classt, (xpointer)cdata->class_data);
    }

    if (cdata->class_finalize_base) {
        cdata->class_finalize_base(classt);
    }

    for (bnode = lookup_type_node_I(NODE_PARENT_TYPE(node)); bnode; bnode = lookup_type_node_I(NODE_PARENT_TYPE(bnode))) {
        if (bnode->data->classt.class_finalize_base) {
            bnode->data->classt.class_finalize_base(classt);
        }
    }

    x_free(cdata->classt);
}

static void type_data_last_unref_Wm(TypeNode *node, xboolean uncached)
{
    x_return_if_fail(node != NULL && node->plugin != NULL);

    if (!node->data || NODE_REFCOUNT(node) == 0) {
        x_critical("cannot drop last reference to unreferenced type '%s'", NODE_NAME(node));
        return;
    }

    if (node->is_classed && node->data && node->data->classt.classt && static_n_class_cache_funcs && !uncached) {
        xuint i;

        X_WRITE_UNLOCK(&type_rw_lock);
        X_READ_LOCK(&type_rw_lock);
        for (i = 0; i < static_n_class_cache_funcs; i++) {
            xboolean need_break;
            xpointer cache_data = static_class_cache_funcs[i].cache_data;
            XTypeClassCacheFunc cache_func = static_class_cache_funcs[i].cache_func;

            X_READ_UNLOCK(&type_rw_lock);
            need_break = cache_func(cache_data, (XTypeClass *)node->data->classt.classt);
            X_READ_LOCK(&type_rw_lock);

            if (!node->data || NODE_REFCOUNT(node) == 0) {
                INVALID_RECURSION("XType class cache function ", cache_func, NODE_NAME(node));
            }

            if (need_break) {
                break;
            }
        }

        X_READ_UNLOCK(&type_rw_lock);
        X_WRITE_LOCK(&type_rw_lock);
    }

    if (x_atomic_int_dec_and_test((int *)&node->ref_count)) {
        XType ptype = NODE_PARENT_TYPE (node);
        TypeData *tdata;

        if (node->is_instantiatable) {

        }

        tdata = node->data;
        if (node->is_classed && tdata->classt.classt) {
            if (CLASSED_NODE_IFACES_ENTRIES_LOCKED(node) != NULL) {
                type_data_finalize_class_ifaces_Wm(node);
            }

            node->mutatable_check_cache = FALSE;
            node->data = NULL;
            X_WRITE_UNLOCK(&type_rw_lock);
            type_data_finalize_class_U(node, &tdata->classt);
            X_WRITE_LOCK(&type_rw_lock);
        } else if (NODE_IS_IFACE(node) && tdata->iface.dflt_vtable) {
            node->mutatable_check_cache = FALSE;
            node->data = NULL;

            if (tdata->iface.dflt_finalize || tdata->iface.vtable_finalize_base) {
                X_WRITE_UNLOCK(&type_rw_lock);
                if (tdata->iface.dflt_finalize) {
                    tdata->iface.dflt_finalize(tdata->iface.dflt_vtable, (xpointer)tdata->iface.dflt_data);
                }

                if (tdata->iface.vtable_finalize_base) {
                    tdata->iface.vtable_finalize_base(tdata->iface.dflt_vtable);
                }
                X_WRITE_LOCK(&type_rw_lock);
            }

            x_free(tdata->iface.dflt_vtable);
        } else {
            node->mutatable_check_cache = FALSE;
            node->data = NULL;
        }

        x_free(tdata);

        X_WRITE_UNLOCK(&type_rw_lock);
        x_type_plugin_unuse(node->plugin);
        if (ptype) {
            type_data_unref_U(lookup_type_node_I(ptype), FALSE);
        }
        X_WRITE_LOCK(&type_rw_lock);
    }
}

static inline void type_data_unref_U(TypeNode *node, xboolean uncached)
{
    xuint current;

    do {
        current = NODE_REFCOUNT(node);

        if (current <= 1) {
            if (!node->plugin) {
                x_critical("static type '%s' unreferenced too often", NODE_NAME(node));
                return;
            } else {
                return;
            }

            x_assert(current > 0);

            x_rec_mutex_lock(&class_init_rec_mutex);
            X_WRITE_LOCK(&type_rw_lock);
            type_data_last_unref_Wm(node, uncached);
            X_WRITE_UNLOCK(&type_rw_lock);
            x_rec_mutex_unlock(&class_init_rec_mutex);
            return;
        }
    } while (!x_atomic_int_compare_and_exchange((int *)&node->ref_count, current, current - 1));
}

void x_type_add_class_cache_func(xpointer cache_data, XTypeClassCacheFunc cache_func)
{
    xuint i;

    x_return_if_fail(cache_func != NULL);

    X_WRITE_LOCK(&type_rw_lock);
    i = static_n_class_cache_funcs++;
    static_class_cache_funcs = x_renew(ClassCacheFunc, static_class_cache_funcs, static_n_class_cache_funcs);
    static_class_cache_funcs[i].cache_data = cache_data;
    static_class_cache_funcs[i].cache_func = cache_func;
    X_WRITE_UNLOCK(&type_rw_lock);
}

void x_type_remove_class_cache_func(xpointer cache_data, XTypeClassCacheFunc cache_func)
{
    xuint i;
    xboolean found_it = FALSE;

    x_return_if_fail(cache_func != NULL);

    X_WRITE_LOCK(&type_rw_lock);
    for (i = 0; i < static_n_class_cache_funcs; i++) {
        if (static_class_cache_funcs[i].cache_data == cache_data && static_class_cache_funcs[i].cache_func == cache_func) {
            static_n_class_cache_funcs--;
            memmove(static_class_cache_funcs + i, static_class_cache_funcs + i + 1, sizeof(static_class_cache_funcs[0]) * (static_n_class_cache_funcs - i));
            static_class_cache_funcs = x_renew(ClassCacheFunc, static_class_cache_funcs, static_n_class_cache_funcs);
            found_it = TRUE;
            break;
        }
    }
    X_WRITE_UNLOCK(&type_rw_lock);

    if (!found_it) {
        x_critical(X_STRLOC ": cannot remove unregistered class cache func %p with data %p", cache_func, cache_data);
    }
}

void x_type_add_interface_check(xpointer check_data, XTypeInterfaceCheckFunc check_func)
{
    xuint i;

    x_return_if_fail(check_func != NULL);

    X_WRITE_LOCK(&type_rw_lock);
    i = static_n_iface_check_funcs++;
    static_iface_check_funcs = x_renew(IFaceCheckFunc, static_iface_check_funcs, static_n_iface_check_funcs);
    static_iface_check_funcs[i].check_data = check_data;
    static_iface_check_funcs[i].check_func = check_func;
    X_WRITE_UNLOCK(&type_rw_lock);
}

void x_type_remove_interface_check(xpointer check_data, XTypeInterfaceCheckFunc check_func)
{
    xuint i;
    xboolean found_it = FALSE;

    x_return_if_fail(check_func != NULL);

    X_WRITE_LOCK(&type_rw_lock);
    for (i = 0; i < static_n_iface_check_funcs; i++) {
        if (static_iface_check_funcs[i].check_data == check_data && static_iface_check_funcs[i].check_func == check_func) {
            static_n_iface_check_funcs--;
            memmove(static_iface_check_funcs + i, static_iface_check_funcs + i + 1, sizeof(static_iface_check_funcs[0]) * (static_n_iface_check_funcs - i));
            static_iface_check_funcs = x_renew(IFaceCheckFunc, static_iface_check_funcs, static_n_iface_check_funcs);
            found_it = TRUE;
            break;
        }
    }
    X_WRITE_UNLOCK(&type_rw_lock);

    if (!found_it) {
        x_critical(X_STRLOC ": cannot remove unregistered class check func %p with data %p", check_func, check_data);
    }
}

XType x_type_register_fundamental(XType type_id, const xchar *type_name, const XTypeInfo *info, const XTypeFundamentalInfo *finfo, XTypeFlags flags)
{
    TypeNode *node;

    x_assert_type_system_initialized ();
    x_return_val_if_fail(type_id > 0, 0);
    x_return_val_if_fail(type_name != NULL, 0);
    x_return_val_if_fail(info != NULL, 0);
    x_return_val_if_fail(finfo != NULL, 0);
    
    if (!check_type_name_I(type_name)) {
        return 0;
    }

    if ((type_id & TYPE_ID_MASK) || type_id > X_TYPE_FUNDAMENTAL_MAX) {
        x_critical("attempt to register fundamental type '%s' with invalid type id (%" X_XUINTPTR_FORMAT ")", type_name, type_id);
        return 0;
    }

    if ((finfo->type_flags & X_TYPE_FLAG_INSTANTIATABLE) && !(finfo->type_flags & X_TYPE_FLAG_CLASSED)) {
        x_critical("cannot register instantiatable fundamental type '%s' as non-classed", type_name);
        return 0;
    }

    if (lookup_type_node_I(type_id)) {
        x_critical("cannot register existing fundamental type '%s' (as '%s')", type_descriptive_name_I(type_id), type_name);
        return 0;
    }

    X_WRITE_LOCK(&type_rw_lock);
    node = type_node_fundamental_new_W(type_id, type_name, finfo->type_flags);
    type_add_flags_W(node, flags);
    
    if (check_type_info_I(NULL, NODE_FUNDAMENTAL_TYPE(node), type_name, info)) {
        type_data_make_W(node, info, check_value_table_I(type_name, info->value_table) ? info->value_table : NULL);
    }
    X_WRITE_UNLOCK(&type_rw_lock);

    return NODE_TYPE(node);
}

XType x_type_register_static_simple(XType parent_type, const xchar *type_name, xuint class_size, XClassInitFunc class_init, xuint instance_size, XInstanceInitFunc instance_init, XTypeFlags flags)
{
    XTypeInfo info;

    x_return_val_if_fail(class_size <= X_MAXUINT16, X_TYPE_INVALID);
    x_return_val_if_fail(instance_size <= X_MAXUINT16, X_TYPE_INVALID);

    info.class_size = class_size;
    info.base_init = NULL;
    info.base_finalize = NULL;
    info.class_init = class_init;
    info.class_finalize = NULL;
    info.class_data = NULL;
    info.instance_size = instance_size;
    info.n_preallocs = 0;
    info.instance_init = instance_init;
    info.value_table = NULL;

    return x_type_register_static(parent_type, type_name, &info, flags);
}

XType x_type_register_static(XType parent_type, const xchar *type_name, const XTypeInfo *info, XTypeFlags flags)
{
    XType type = 0;
    TypeNode *pnode, *node;

    x_assert_type_system_initialized();
    x_return_val_if_fail(parent_type > 0, 0);
    x_return_val_if_fail(type_name != NULL, 0);
    x_return_val_if_fail(info != NULL, 0);

    if (!check_type_name_I(type_name) || !check_derivation_I(parent_type, type_name)) {
        return 0;
    }

    if (info->class_finalize) {
        x_critical("class finalizer specified for static type '%s'", type_name);
        return 0;
    }

    pnode = lookup_type_node_I(parent_type);
    X_WRITE_LOCK(&type_rw_lock);
    type_data_ref_Wm(pnode);

    if (check_type_info_I(pnode, NODE_FUNDAMENTAL_TYPE(pnode), type_name, info)) {
        node = type_node_new_W(pnode, type_name, NULL);
        type_add_flags_W(node, flags);
        type = NODE_TYPE(node);
        type_data_make_W(node, info, check_value_table_I(type_name, info->value_table) ? info->value_table : NULL);
    }
    X_WRITE_UNLOCK(&type_rw_lock);

    return type;
}

XType x_type_register_dynamic(XType parent_type, const xchar *type_name, XTypePlugin *plugin, XTypeFlags flags)
{
    XType type;
    TypeNode *pnode, *node;

    x_assert_type_system_initialized();
    x_return_val_if_fail(parent_type > 0, 0);
    x_return_val_if_fail(type_name != NULL, 0);
    x_return_val_if_fail(plugin != NULL, 0);

    if (!check_type_name_I(type_name) || !check_derivation_I(parent_type, type_name) || !check_plugin_U(plugin, TRUE, FALSE, type_name)) {
        return 0;
    }

    X_WRITE_LOCK(&type_rw_lock);
    pnode = lookup_type_node_I(parent_type);
    node = type_node_new_W(pnode, type_name, plugin);
    type_add_flags_W(node, flags);
    type = NODE_TYPE(node);
    X_WRITE_UNLOCK(&type_rw_lock);

    return type;
}

void x_type_add_interface_static(XType instance_type, XType interface_type, const XInterfaceInfo *info)
{
    x_return_if_fail(X_TYPE_IS_INSTANTIATABLE(instance_type));
    x_return_if_fail(x_type_parent(interface_type) == X_TYPE_INTERFACE);

    x_rec_mutex_lock(&class_init_rec_mutex);
    X_WRITE_LOCK(&type_rw_lock);
    if (check_add_interface_L(instance_type, interface_type)) {
        TypeNode *node = lookup_type_node_I(instance_type);
        TypeNode *iface = lookup_type_node_I(interface_type);
        if (check_interface_info_I(iface, NODE_TYPE(node), info)) {
            type_add_interface_Wm(node, iface, info, NULL);
        }
    }
    X_WRITE_UNLOCK(&type_rw_lock);
    x_rec_mutex_unlock(&class_init_rec_mutex);
}

void x_type_add_interface_dynamic(XType instance_type, XType interface_type, XTypePlugin *plugin)
{
    TypeNode *node;

    x_return_if_fail(X_TYPE_IS_INSTANTIATABLE(instance_type));
    x_return_if_fail(x_type_parent(interface_type) == X_TYPE_INTERFACE);

    node = lookup_type_node_I(instance_type);
    if (!check_plugin_U(plugin, FALSE, TRUE, NODE_NAME(node))) {
        return;
    }

    x_rec_mutex_lock(&class_init_rec_mutex);
    X_WRITE_LOCK(&type_rw_lock);
    if (check_add_interface_L(instance_type, interface_type)) {
        TypeNode *iface = lookup_type_node_I(interface_type);
        type_add_interface_Wm(node, iface, NULL, plugin);
    }
    X_WRITE_UNLOCK(&type_rw_lock);
    x_rec_mutex_unlock(&class_init_rec_mutex);
}

xpointer x_type_class_ref(XType type)
{
    XType ptype;
    TypeNode *node;
    xboolean holds_ref;
    XTypeClass *pclass;

    node = lookup_type_node_I(type);
    if (!node || !node->is_classed) {
        x_critical("cannot retrieve class for invalid (unclassed) type '%s'", type_descriptive_name_I(type));
        return NULL;
    }

    if (X_LIKELY(type_data_ref_U (node))) {
        if (X_LIKELY(x_atomic_int_get(&node->data->classt.init_state) == INITIALIZED)) {
            return node->data->classt.classt;
        }

        holds_ref = TRUE;
    } else {
        holds_ref = FALSE;
    }

    x_rec_mutex_lock(&class_init_rec_mutex);

    ptype = NODE_PARENT_TYPE(node);
    pclass = ptype ? (XTypeClass *)x_type_class_ref(ptype) : NULL;

    X_WRITE_LOCK(&type_rw_lock);
    if (!holds_ref) {
        type_data_ref_Wm(node);
    }

    if (!node->data->classt.classt) {
        type_class_init_Wm(node, pclass);
    }
    X_WRITE_UNLOCK(&type_rw_lock);

    if (pclass) {
        x_type_class_unref(pclass);
    }
    x_rec_mutex_unlock(&class_init_rec_mutex);

    return node->data->classt.classt;
}

void x_type_class_unref(xpointer x_class)
{
    TypeNode *node;
    XTypeClass *classt = (XTypeClass *)x_class;

    x_return_if_fail(x_class != NULL);

    node = lookup_type_node_I(classt->x_type);
    if (node && node->is_classed && NODE_REFCOUNT(node)) {
        type_data_unref_U(node, FALSE);
    } else {
        x_critical("cannot unreference class of invalid (unclassed) type '%s'", type_descriptive_name_I(classt->x_type));
    }
}

void x_type_class_unref_uncached(xpointer x_class)
{
    TypeNode *node;
    XTypeClass *classt = (XTypeClass *)x_class;

    x_return_if_fail(x_class != NULL);

    node = lookup_type_node_I(classt->x_type);
    if (node && node->is_classed && NODE_REFCOUNT(node)) {
        type_data_unref_U(node, TRUE);
    } else {
        x_critical("cannot unreference class of invalid (unclassed) type '%s'", type_descriptive_name_I(classt->x_type));
    }
}

xpointer x_type_class_peek(XType type)
{
    TypeNode *node;
    xpointer classt;

    node = lookup_type_node_I(type);
    if (node && node->is_classed && NODE_REFCOUNT(node) && x_atomic_int_get(&node->data->classt.init_state) == INITIALIZED) {
        classt = node->data->classt.classt;
    } else {
        classt = NULL;
    }

    return classt;
}

xpointer x_type_class_peek_static(XType type)
{
    TypeNode *node;
    xpointer classt;

    node = lookup_type_node_I(type);
    if (node && node->is_classed && NODE_REFCOUNT(node) && node->plugin == NULL && x_atomic_int_get(&node->data->classt.init_state) == INITIALIZED) {
        classt = node->data->classt.classt;
    } else {
        classt = NULL;
    }

    return classt;
}

xpointer x_type_class_peek_parent(xpointer x_class)
{
    TypeNode *node;
    xpointer classt = NULL;

    x_return_val_if_fail(x_class != NULL, NULL);

    node = lookup_type_node_I(X_TYPE_FROM_CLASS(x_class));
    x_return_val_if_fail(node != NULL, NULL);

    if (node->is_classed && node->data && NODE_PARENT_TYPE(node)) {
        node = lookup_type_node_I(NODE_PARENT_TYPE(node));
        classt = node->data->classt.classt;
    } else if (NODE_PARENT_TYPE(node)) {
        x_critical(X_STRLOC ": invalid class pointer '%p'", x_class);
    }

    return classt;
}

xpointer x_type_interface_peek(xpointer instance_class, XType iface_type)
{
    TypeNode *node;
    TypeNode *iface;
    xpointer vtable = NULL;
    XTypeClass *classt = (XTypeClass *)instance_class;

    x_return_val_if_fail(instance_class != NULL, NULL);

    node = lookup_type_node_I(classt->x_type);
    iface = lookup_type_node_I(iface_type);
    if (node && node->is_instantiatable && iface) {
        type_lookup_iface_vtable_I(node, iface, &vtable);
    } else {
        x_critical(X_STRLOC ": invalid class pointer '%p'", classt);
    }

    return vtable;
}

xpointer x_type_interface_peek_parent(xpointer x_iface)
{
    TypeNode *node;
    TypeNode *iface;
    xpointer vtable = NULL;
    XTypeInterface *iface_class = (XTypeInterface *)x_iface;

    x_return_val_if_fail(x_iface != NULL, NULL);

    iface = lookup_type_node_I(iface_class->x_type);
    node = lookup_type_node_I(iface_class->x_instance_type);
    if (node) {
        node = lookup_type_node_I(NODE_PARENT_TYPE(node));
    }

    if (node && node->is_instantiatable && iface) {
        type_lookup_iface_vtable_I(node, iface, &vtable);
    } else if (node) {
        x_critical(X_STRLOC ": invalid interface pointer '%p'", x_iface);
    }

    return vtable;
}

xpointer x_type_default_interface_ref(XType x_type)
{
    TypeNode *node;
    xpointer dflt_vtable;

    X_WRITE_LOCK(&type_rw_lock);

    node = lookup_type_node_I(x_type);
    if (!node || !NODE_IS_IFACE(node) || (node->data && NODE_REFCOUNT(node) == 0)) {
        X_WRITE_UNLOCK(&type_rw_lock);
        x_critical("cannot retrieve default vtable for invalid or non-interface type '%s'", type_descriptive_name_I(x_type));
        return NULL;
    }

    if (!node->data || !node->data->iface.dflt_vtable) {
        X_WRITE_UNLOCK(&type_rw_lock);
        x_rec_mutex_lock(&class_init_rec_mutex);
        X_WRITE_LOCK(&type_rw_lock);
        node = lookup_type_node_I(x_type);
        type_data_ref_Wm(node);
        type_iface_ensure_dflt_vtable_Wm(node);
        x_rec_mutex_unlock(&class_init_rec_mutex);
    } else {
        type_data_ref_Wm(node);
    }

    dflt_vtable = node->data->iface.dflt_vtable;
    X_WRITE_UNLOCK(&type_rw_lock);

    return dflt_vtable;
}

xpointer x_type_default_interface_peek(XType x_type)
{
    TypeNode *node;
    xpointer vtable;

    node = lookup_type_node_I(x_type);
    if (node && NODE_IS_IFACE(node) && NODE_REFCOUNT(node)) {
        vtable = node->data->iface.dflt_vtable;
    } else {
        vtable = NULL;
    }

    return vtable;
}

void x_type_default_interface_unref(xpointer x_iface)
{
    TypeNode *node;
    XTypeInterface *vtable = (XTypeInterface *)x_iface;

    x_return_if_fail(x_iface != NULL);

    node = lookup_type_node_I(vtable->x_type);
    if (node && NODE_IS_IFACE(node)) {
        type_data_unref_U(node, FALSE);
    } else {
        x_critical("cannot unreference invalid interface default vtable for '%s'", type_descriptive_name_I(vtable->x_type));
    }
}

const xchar *x_type_name(XType type)
{
    TypeNode *node;

    x_assert_type_system_initialized();
    node = lookup_type_node_I(type);

    return node ? NODE_NAME(node) : NULL;
}

XQuark x_type_qname(XType type)
{
    TypeNode *node;

    node = lookup_type_node_I(type);
    return node ? node->qname : 0;
}

XType x_type_from_name(const xchar *name)
{
    XType type = 0;

    x_return_val_if_fail(name != NULL, 0);

    X_READ_LOCK(&type_rw_lock);
    type = XPOINTER_TO_TYPE(x_hash_table_lookup(static_type_nodes_ht, name));
    X_READ_UNLOCK(&type_rw_lock);

    return type;
}

XType x_type_parent(XType type)
{
    TypeNode *node;

    node = lookup_type_node_I(type);
    return node ? NODE_PARENT_TYPE(node) : 0;
}

xuint x_type_depth(XType type)
{
    TypeNode *node;

    node = lookup_type_node_I(type);
    return node ? node->n_supers + 1 : 0;
}

XType x_type_next_base(XType type, XType base_type)
{
    TypeNode *node;
    XType atype = 0;

    node = lookup_type_node_I(type);
    if (node) {
        TypeNode *base_node = lookup_type_node_I(base_type);
        if (base_node && base_node->n_supers < node->n_supers) {
            xuint n = node->n_supers - base_node->n_supers;

            if (node->supers[n] == base_type) {
                atype = node->supers[n - 1];
            }
        }
    }

    return atype;
}

static inline xboolean type_node_check_conformities_UorL(TypeNode *node, TypeNode *iface_node, xboolean support_interfaces, xboolean support_prerequisites, xboolean have_lock)
{
    xboolean match;

    if (NODE_IS_ANCESTOR(iface_node, node)) {
        return TRUE;
    }

    support_interfaces = support_interfaces && node->is_instantiatable && NODE_IS_IFACE(iface_node);
    support_prerequisites = support_prerequisites && NODE_IS_IFACE(node);
    match = FALSE;

    if (support_interfaces) {
        if (have_lock) {
            if (type_lookup_iface_entry_L(node, iface_node)) {
                match = TRUE;
            }
        } else {
            if (type_lookup_iface_vtable_I(node, iface_node, NULL)) {
                match = TRUE;
            }
        }
    }

    if (!match && support_prerequisites) {
        if (!have_lock) {
            X_READ_LOCK(&type_rw_lock);
        }

        if (support_prerequisites && type_lookup_prerequisite_L(node, NODE_TYPE(iface_node))) {
            match = TRUE;
        }

        if (!have_lock) {
            X_READ_UNLOCK(&type_rw_lock);
        }
    }

    return match;
}

static xboolean type_node_is_a_L(TypeNode *node, TypeNode *iface_node)
{
    return type_node_check_conformities_UorL(node, iface_node, TRUE, TRUE, TRUE);
}

static inline xboolean type_node_conforms_to_U(TypeNode *node, TypeNode *iface_node, xboolean support_interfaces, xboolean support_prerequisites)
{
    return type_node_check_conformities_UorL(node, iface_node, support_interfaces, support_prerequisites, FALSE);
}

xboolean (x_type_is_a)(XType type, XType iface_type)
{
    xboolean is_a;
    TypeNode *node, *iface_node;

    if (type == iface_type) {
        return TRUE;
    }

    node = lookup_type_node_I(type);
    iface_node = lookup_type_node_I(iface_type);
    is_a = node && iface_node && type_node_conforms_to_U(node, iface_node, TRUE, TRUE);

    return is_a;
}

XType *x_type_children(XType type, xuint *n_children)
{
    TypeNode *node;

    node = lookup_type_node_I(type);
    if (node) {
        XType *children;

        X_READ_LOCK(&type_rw_lock);
        children = x_new(XType, node->n_children + 1);
        if (node->n_children != 0) {
            memcpy(children, node->children, sizeof(XType) * node->n_children);
        }
        children[node->n_children] = 0;

        if (n_children) {
            *n_children = node->n_children;
        }
        X_READ_UNLOCK(&type_rw_lock);

        return children;
    } else {
        if (n_children) {
            *n_children = 0;
        }

        return NULL;
    }
}

XType *x_type_interfaces(XType type, xuint *n_interfaces)
{
    TypeNode *node;

    node = lookup_type_node_I(type);
    if (node && node->is_instantiatable) {
        xuint i;
        XType *ifaces;
        IFaceEntries *entries;

        X_READ_LOCK(&type_rw_lock);
        entries = CLASSED_NODE_IFACES_ENTRIES_LOCKED(node);
        if (entries) {
            ifaces = x_new(XType, IFACE_ENTRIES_N_ENTRIES(entries) + 1);
            for (i = 0; i < IFACE_ENTRIES_N_ENTRIES(entries); i++) {
                ifaces[i] = entries->entry[i].iface_type;
            }
        } else {
            ifaces = x_new(XType, 1);
            i = 0;
        }
        ifaces[i] = 0;

        if (n_interfaces) {
            *n_interfaces = i;
        }
        X_READ_UNLOCK(&type_rw_lock);

        return ifaces;
    } else {
        if (n_interfaces) {
            *n_interfaces = 0;
        }

        return NULL;
    }
}

typedef struct _QData QData;
struct _XData {
    xuint n_qdatas;
    QData *qdatas;
};
struct _QData {
    XQuark   quark;
    xpointer data;
};

static inline xpointer type_get_qdata_L(TypeNode *node, XQuark quark)
{
    XData *gdata = node->global_gdata;

    if (quark && gdata && gdata->n_qdatas) {
        QData *qdatas = gdata->qdatas - 1;
        xuint n_qdatas = gdata->n_qdatas;

        do {
            xuint i;
            QData *check;

            i = (n_qdatas + 1) / 2;
            check = qdatas + i;
            if (quark == check->quark) {
                return check->data;
            } else if (quark > check->quark) {
                n_qdatas -= i;
                qdatas = check;
            } else {
                n_qdatas = i - 1;
            }
        } while (n_qdatas);
    }

    return NULL;
}

xpointer x_type_get_qdata(XType type, XQuark quark)
{
    xpointer data;
   TypeNode *node;

    node = lookup_type_node_I(type);
    if (node) {
        X_READ_LOCK(&type_rw_lock);
        data = type_get_qdata_L(node, quark);
        X_READ_UNLOCK(&type_rw_lock);
    } else {
        x_return_val_if_fail(node != NULL, NULL);
        data = NULL;
    }

    return data;
}

static inline void type_set_qdata_W(TypeNode *node, XQuark quark, xpointer data)
{
    xuint i;
    XData *gdata;
    QData *qdata;

    if (!node->global_gdata) {
        node->global_gdata = x_new0(XData, 1);
    }
    gdata = node->global_gdata;

    qdata = gdata->qdatas;
    for (i = 0; i < gdata->n_qdatas; i++) {
        if (qdata[i].quark == quark) {
            qdata[i].data = data;
            return;
        }
    }

    gdata->n_qdatas++;
    gdata->qdatas = x_renew(QData, gdata->qdatas, gdata->n_qdatas);
    qdata = gdata->qdatas;
    for (i = 0; i < gdata->n_qdatas - 1; i++) {
        if (qdata[i].quark > quark) {
            break;
        }
    }

    memmove(qdata + i + 1, qdata + i, sizeof(qdata[0]) * (gdata->n_qdatas - i - 1));
    qdata[i].quark = quark;
    qdata[i].data = data;
}

void x_type_set_qdata(XType type, XQuark quark, xpointer data)
{
    TypeNode *node;

    x_return_if_fail(quark != 0);

    node = lookup_type_node_I(type);
    if (node) {
        X_WRITE_LOCK(&type_rw_lock);
        type_set_qdata_W(node, quark, data);
        X_WRITE_UNLOCK(&type_rw_lock);
    } else {
        x_return_if_fail(node != NULL);
    }
}

static void type_add_flags_W(TypeNode *node, XTypeFlags flags)
{
    xuint dflags;

    x_return_if_fail((flags & ~TYPE_FLAG_MASK) == 0);
    x_return_if_fail(node != NULL);

    if ((flags & TYPE_FLAG_MASK) && node->is_classed && node->data && node->data->classt.classt) {
        x_critical("tagging type '%s' as abstract after class initialization", NODE_NAME(node));
    }

    dflags = XPOINTER_TO_UINT(type_get_qdata_L(node, static_quark_type_flags));
    dflags |= flags;
    type_set_qdata_W(node, static_quark_type_flags, XUINT_TO_POINTER (dflags));

    node->is_abstract = (flags & X_TYPE_FLAG_ABSTRACT) != 0;
    node->is_deprecated |= (flags & X_TYPE_FLAG_DEPRECATED) != 0;
    node->is_final = (flags & X_TYPE_FLAG_FINAL) != 0;
}

void x_type_query(XType type, XTypeQuery *query)
{
    TypeNode *node;

    x_return_if_fail(query != NULL);

    query->type = 0;
    node = lookup_type_node_I(type);
    if (node && node->is_classed) {
        X_READ_LOCK(&type_rw_lock);
        if (node->data) {
            query->type = NODE_TYPE(node);
            query->type_name = NODE_NAME(node);
            query->class_size = node->data->classt.class_size;
            query->instance_size = node->is_instantiatable ? node->data->instance.instance_size : 0;
        }
        X_READ_UNLOCK(&type_rw_lock);
    }
}

int x_type_get_instance_count(XType type)
{
    return 0;
}

static inline xboolean _x_type_test_flags(XType type, xuint flags)
{
    TypeNode *node;
    xboolean result = FALSE;

    node = lookup_type_node_I(type);
    if (node) {
        if ((flags & ~NODE_FLAG_MASK) == 0) {
            if ((flags & X_TYPE_FLAG_CLASSED) && !node->is_classed) {
                return FALSE;
            }

            if ((flags & X_TYPE_FLAG_INSTANTIATABLE) && !node->is_instantiatable) {
                return FALSE;
            }

            if ((flags & X_TYPE_FLAG_FINAL) && !node->is_final) {
                return FALSE;
            }

            if ((flags & X_TYPE_FLAG_ABSTRACT) && !node->is_abstract) {
                return FALSE;
            }

            if ((flags & X_TYPE_FLAG_DEPRECATED) && !node->is_deprecated) {
                return FALSE;
            }

            return TRUE;
        }

        xuint fflags = flags & TYPE_FUNDAMENTAL_FLAG_MASK;
        xuint tflags = flags & TYPE_FLAG_MASK;

        if (fflags) {
            XTypeFundamentalInfo *finfo = type_node_fundamental_info_I(node);
            fflags = (finfo->type_flags & fflags) == fflags;
        } else {
            fflags = TRUE;
        }

        if (tflags) {
            X_READ_LOCK(&type_rw_lock);
            tflags = (tflags & XPOINTER_TO_UINT(type_get_qdata_L(node, static_quark_type_flags))) == tflags;
            X_READ_UNLOCK(&type_rw_lock);
        } else {
            tflags = TRUE;
        }

        result = tflags && fflags;
    }

    return result;
}

xboolean (x_type_test_flags)(XType type, xuint flags)
{
    return _x_type_test_flags(type, flags);
}

XTypePlugin *x_type_get_plugin(XType type)
{
    TypeNode *node;

    node = lookup_type_node_I(type);
    return node ? node->plugin : NULL;
}

XTypePlugin *x_type_interface_get_plugin(XType instance_type, XType interface_type)
{
    TypeNode *node;
    TypeNode *iface;

    x_return_val_if_fail(X_TYPE_IS_INTERFACE(interface_type), NULL);

    node = lookup_type_node_I(instance_type);  
    iface = lookup_type_node_I(interface_type);
    if (node && iface) {
        IFaceHolder *iholder;
        XTypePlugin *plugin;

        X_READ_LOCK(&type_rw_lock);

        iholder = iface_node_get_holders_L(iface);
        while (iholder && iholder->instance_type != instance_type) {
            iholder = iholder->next;
        }
        plugin = iholder ? iholder->plugin : NULL;

        X_READ_UNLOCK(&type_rw_lock);

        return plugin;
    }

    x_return_val_if_fail(node == NULL, NULL);
    x_return_val_if_fail(iface == NULL, NULL);
    x_critical(X_STRLOC ": attempt to look up plugin for invalid instance/interface type pair.");

    return NULL;
}

XType x_type_fundamental_next(void)
{
    XType type;

    X_READ_LOCK(&type_rw_lock);
    type = static_fundamental_next;
    X_READ_UNLOCK(&type_rw_lock);
    type = X_TYPE_MAKE_FUNDAMENTAL(type);

    return type <= X_TYPE_FUNDAMENTAL_MAX ? type : 0;
}

XType x_type_fundamental(XType type_id)
{
    TypeNode *node = lookup_type_node_I(type_id);
    return node ? NODE_FUNDAMENTAL_TYPE(node) : 0;
}

xboolean x_type_check_instance_is_a(XTypeInstance *type_instance, XType iface_type)
{
    xboolean check;
    TypeNode *node, *iface;

    if (!type_instance || !type_instance->x_class) {
        return FALSE;
    }

    iface = lookup_type_node_I(iface_type);
    if (iface && iface->is_final) {
        return type_instance->x_class->x_type == iface_type;
    }

    node = lookup_type_node_I(type_instance->x_class->x_type);
    check = node && node->is_instantiatable && iface && type_node_conforms_to_U(node, iface, TRUE, FALSE);

    return check;
}

xboolean x_type_check_instance_is_fundamentally_a(XTypeInstance *type_instance, XType fundamental_type)
{
    TypeNode *node;
    if (!type_instance || !type_instance->x_class) {
        return FALSE;
    }

    node = lookup_type_node_I(type_instance->x_class->x_type);
    return node && (NODE_FUNDAMENTAL_TYPE(node) == fundamental_type);
}

xboolean x_type_check_class_is_a(XTypeClass *type_class, XType is_a_type)
{
    xboolean check;
    TypeNode *node, *iface;

    if (!type_class) {
        return FALSE;
    }

    node = lookup_type_node_I(type_class->x_type);
    iface = lookup_type_node_I(is_a_type);
    check = node && node->is_classed && iface && type_node_conforms_to_U(node, iface, FALSE, FALSE);

    return check;
}

XTypeInstance *x_type_check_instance_cast(XTypeInstance *type_instance, XType iface_type)
{
    if (type_instance) {
        if (type_instance->x_class) {
            TypeNode *node, *iface;
            xboolean is_instantiatable, check;

            node = lookup_type_node_I(type_instance->x_class->x_type);
            is_instantiatable = node && node->is_instantiatable;
            iface = lookup_type_node_I(iface_type);
            check = is_instantiatable && iface && type_node_conforms_to_U(node, iface, TRUE, FALSE);
            if (check) {
                return type_instance;
            }

            if (is_instantiatable) {
                x_critical("invalid cast from '%s' to '%s'", type_descriptive_name_I(type_instance->x_class->x_type), type_descriptive_name_I(iface_type));
            } else {
                x_critical("invalid uninstantiatable type '%s' in cast to '%s'", type_descriptive_name_I(type_instance->x_class->x_type), type_descriptive_name_I(iface_type));
            }
        } else {
            x_critical("invalid unclassed pointer in cast to '%s'", type_descriptive_name_I(iface_type));
        }
    }

    return type_instance;
}

XTypeClass *x_type_check_class_cast(XTypeClass *type_class, XType is_a_type)
{
    if (type_class) {
        TypeNode *node, *iface;
        xboolean is_classed, check;
        
        node = lookup_type_node_I(type_class->x_type);
        is_classed = node && node->is_classed;
        iface = lookup_type_node_I(is_a_type);

        check = is_classed && iface && type_node_conforms_to_U(node, iface, FALSE, FALSE);
        if (check) {
            return type_class;
        }

        if (is_classed)
            x_critical("invalid class cast from '%s' to '%s'", type_descriptive_name_I(type_class->x_type), type_descriptive_name_I(is_a_type));
        else
            x_critical("invalid unclassed type '%s' in class cast to '%s'", type_descriptive_name_I(type_class->x_type), type_descriptive_name_I(is_a_type));
    } else {
        x_critical("invalid class cast from (NULL) pointer to '%s'", type_descriptive_name_I(is_a_type));
    }

    return type_class;
}

xboolean x_type_check_instance(XTypeInstance *type_instance)
{
    if (type_instance) {
        if (type_instance->x_class) {
            TypeNode *node = lookup_type_node_I(type_instance->x_class->x_type);
            if (node && node->is_instantiatable) {
                return TRUE;
            }

            x_critical("instance of invalid non-instantiatable type '%s'", type_descriptive_name_I(type_instance->x_class->x_type));
        } else {
            x_critical("instance with invalid (NULL) class pointer");
        }
    } else {
        x_critical("invalid (NULL) pointer instance");
    }

    return FALSE;
}

static inline xboolean type_check_is_value_type_U(XType type)
{
    TypeNode *node;
    XTypeFlags tflags = X_TYPE_FLAG_VALUE_ABSTRACT;

    node = lookup_type_node_I(type);
    if (node && node->mutatable_check_cache) {
        return TRUE;
    }

    X_READ_LOCK(&type_rw_lock);
    restart_check:
    if (node) {
        if (node->data && NODE_REFCOUNT(node) > 0 && node->data->common.value_table->value_init) {
            tflags = (XTypeFlags)XPOINTER_TO_UINT(type_get_qdata_L(node, static_quark_type_flags));
        } else if (NODE_IS_IFACE(node)) {
            xuint i;

            for (i = 0; i < IFACE_NODE_N_PREREQUISITES(node); i++) {
                XType prtype = IFACE_NODE_PREREQUISITES(node)[i];
                TypeNode *prnode = lookup_type_node_I(prtype);

                if (prnode->is_instantiatable) {
                    type = prtype;
                    node = lookup_type_node_I(type);
                    goto restart_check;
                }
            }
        }
    }
    X_READ_UNLOCK(&type_rw_lock);

    return !(tflags & X_TYPE_FLAG_VALUE_ABSTRACT);
}

xboolean x_type_check_is_value_type(XType type)
{
    return type_check_is_value_type_U(type);
}

xboolean x_type_check_value(const XValue *value)
{
    return value && type_check_is_value_type_U(value->x_type);
}

xboolean x_type_check_value_holds(const XValue *value, XType type)
{
    return value && type_check_is_value_type_U (value->x_type) && x_type_is_a(value->x_type, type);
}

XTypeValueTable *x_type_value_table_peek(XType type)
{
    XTypeValueTable *vtable = NULL;
    xboolean has_refed_data, has_table;
    TypeNode *node = lookup_type_node_I(type);

    if (node && NODE_REFCOUNT(node) && node->mutatable_check_cache) {
        return node->data->common.value_table;
    }

    X_READ_LOCK(&type_rw_lock);

restart_table_peek:
    has_refed_data = node && node->data && NODE_REFCOUNT(node) > 0;
    has_table = has_refed_data && node->data->common.value_table->value_init;
    if (has_refed_data) {
        if (has_table) {
            vtable = node->data->common.value_table;
        } else if (NODE_IS_IFACE(node)) {
            xuint i;

            for (i = 0; i < IFACE_NODE_N_PREREQUISITES(node); i++) {
                XType prtype = IFACE_NODE_PREREQUISITES(node)[i];
                TypeNode *prnode = lookup_type_node_I(prtype);

                if (prnode->is_instantiatable) {
                    type = prtype;
                    node = lookup_type_node_I(type);
                    goto restart_table_peek;
                }
            }
        }
    }

    X_READ_UNLOCK(&type_rw_lock);

    if (vtable) {
        return vtable;
    }

    if (!node) {
        x_critical(X_STRLOC ": type id '%" X_XUINTPTR_FORMAT "' is invalid", (xuintptr)type);
    }

    if (!has_refed_data) {
        x_critical("can't peek value table for type '%s' which is not currently referenced", type_descriptive_name_I(type));
    }

    return NULL;
}

const xchar *x_type_name_from_instance(XTypeInstance *instance)
{
    if (!instance) {
        return "<NULL-instance>";
    } else {
        return x_type_name_from_class(instance->x_class);
    }
}

const xchar *x_type_name_from_class(XTypeClass *x_class)
{
    if (!x_class) {
        return "<NULL-class>";
    } else {
        return x_type_name(x_class->x_type);
    }
}

xpointer _x_type_boxed_copy(XType type, xpointer value)
{
    TypeNode *node = lookup_type_node_I(type);
    return node->data->boxed.copy_func(value);
}

void _x_type_boxed_free(XType type, xpointer value)
{
    TypeNode *node = lookup_type_node_I(type);
    node->data->boxed.free_func(value);
}

void _x_type_boxed_init(XType type, XBoxedCopyFunc copy_func, XBoxedFreeFunc free_func)
{
    TypeNode *node = lookup_type_node_I(type);

    node->data->boxed.copy_func = copy_func;
    node->data->boxed.free_func = free_func;
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
void x_type_init_with_debug_flags(XTypeDebugFlags debug_flags)
{
    x_assert_type_system_initialized();

    if (debug_flags) {
        x_message("x_type_init_with_debug_flags() is no longer supported.  Use the XOBJECT_DEBUG environment variable.");
    }
}
X_GNUC_END_IGNORE_DEPRECATIONS

void x_type_init(void)
{
    x_assert_type_system_initialized();
}

static void xobject_init(void)
{
    XTypeInfo info;
    TypeNode *node;
    const xchar *env_string;
    XType type X_GNUC_UNUSED;

    XLIB_PRIVATE_CALL(xlib_init)();

    X_WRITE_LOCK(&type_rw_lock);

    env_string = x_getenv("XOBJECT_DEBUG");
    if (env_string != NULL) {
        XDebugKey debug_keys[] = {
            { "objects", X_TYPE_DEBUG_OBJECTS },
            { "instance-count", X_TYPE_DEBUG_INSTANCE_COUNT },
            { "signals", X_TYPE_DEBUG_SIGNALS },
        };

        _x_type_debug_flags = (XTypeDebugFlags)x_parse_debug_string(env_string, debug_keys, X_N_ELEMENTS(debug_keys));
    }

    static_quark_type_flags = x_quark_from_static_string("-x-type-private--XTypeFlags");
    static_quark_iface_holder = x_quark_from_static_string("-x-type-private--IFaceHolder");
    static_quark_dependants_array = x_quark_from_static_string("-x-type-private--dependants-array");

    static_type_nodes_ht = x_hash_table_new(x_str_hash, x_str_equal);
    static_fundamental_type_nodes[0] = NULL;

    node = type_node_fundamental_new_W(X_TYPE_NONE, x_intern_static_string("void"), (XTypeFundamentalFlags)0);
    type = NODE_TYPE(node);

    x_assert(type == X_TYPE_NONE);

    memset(&info, 0, sizeof(info));
    node = type_node_fundamental_new_W(X_TYPE_INTERFACE, x_intern_static_string("XInterface"), (XTypeFundamentalFlags)X_TYPE_FLAG_DERIVABLE);
    type = NODE_TYPE(node);
    type_data_make_W(node, &info, NULL);
    x_assert(type == X_TYPE_INTERFACE);

    X_WRITE_UNLOCK(&type_rw_lock);

    _x_value_c_init();
    x_type_ensure(x_type_plugin_get_type());

    _x_value_types_init();
    _x_enum_types_init();
    _x_boxed_type_init();
    _x_param_type_init();
    _x_object_type_init();
    _x_param_spec_types_init();
    _x_value_transforms_init();
    _x_signal_init();
}

#ifdef X_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma X_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(xobject_init_ctor)
#endif

X_DEFINE_CONSTRUCTOR(xobject_init_ctor)

static void xobject_init_ctor(void)
{
    xobject_init();
}

void x_type_class_add_private(xpointer x_class, xsize private_size)
{
    XType instance_type = ((XTypeClass *)x_class)->x_type;
    TypeNode *node = lookup_type_node_I(instance_type);

    x_return_if_fail(private_size > 0);
    x_return_if_fail(private_size <= 0xffff);

    if (!node || !node->is_instantiatable || !node->data || node->data->classt.classt != x_class) {
        x_critical("cannot add private field to invalid (non-instantiatable) type '%s'", type_descriptive_name_I(instance_type));
        return;
    }

    if (NODE_PARENT_TYPE(node)) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE(node));
        if (node->data->instance.private_size != pnode->data->instance.private_size) {
            x_critical("g_type_class_add_private() called multiple times for the same type");
            return;
        }
    }
    
    X_WRITE_LOCK(&type_rw_lock);

    private_size = ALIGN_STRUCT(node->data->instance.private_size + private_size);
    x_assert(private_size <= 0xffff);
    node->data->instance.private_size = private_size;

    X_WRITE_UNLOCK(&type_rw_lock);
}

xint x_type_add_instance_private(XType class_gtype, xsize private_size)
{
    TypeNode *node = lookup_type_node_I(class_gtype);

    x_return_val_if_fail(private_size > 0, 0);
    x_return_val_if_fail(private_size <= 0xffff, 0);

    if (!node || !node->is_classed || !node->is_instantiatable || !node->data) {
        x_critical("cannot add private field to invalid (non-instantiatable) type '%s'", type_descriptive_name_I(class_gtype));
        return 0;
    }

    if (node->plugin != NULL) {
        x_critical("cannot use x_type_add_instance_private() with dynamic type '%s'", type_descriptive_name_I(class_gtype));
        return 0;
    }

    return private_size;
}

void x_type_class_adjust_private_offset(xpointer x_class, xint *private_size_or_offset)
{
    xssize private_size;
    XType class_gtype = ((XTypeClass *)x_class)->x_type;
    TypeNode *node = lookup_type_node_I(class_gtype);

    x_return_if_fail(private_size_or_offset != NULL);

    if (*private_size_or_offset > 0) {
        x_return_if_fail(*private_size_or_offset <= 0xffff);
    } else {
        return;
    }

    if (!node || !node->is_classed || !node->is_instantiatable || !node->data) {
        x_critical("cannot add private field to invalid (non-instantiatable) type '%s'", type_descriptive_name_I(class_gtype));
        *private_size_or_offset = 0;
        return;
    }

    if (NODE_PARENT_TYPE(node)) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE (node));
        if (node->data->instance.private_size != pnode->data->instance.private_size) {
            x_critical("g_type_add_instance_private() called multiple times for the same type");
            *private_size_or_offset = 0;
            return;
        }
    }

    X_WRITE_LOCK(&type_rw_lock);

    private_size = ALIGN_STRUCT(node->data->instance.private_size + *private_size_or_offset);
    x_assert(private_size <= 0xffff);
    node->data->instance.private_size = private_size;

    *private_size_or_offset = -(xint) node->data->instance.private_size;

    X_WRITE_UNLOCK(&type_rw_lock);
}

xpointer x_type_instance_get_private(XTypeInstance *instance, XType private_type)
{
    TypeNode *node;

    x_return_val_if_fail(instance != NULL && instance->x_class != NULL, NULL);

    node = lookup_type_node_I(private_type);
    if (X_UNLIKELY(!node || !node->is_instantiatable)) {
        x_critical("instance of invalid non-instantiatable type '%s'", type_descriptive_name_I(instance->x_class->x_type));
        return NULL;
    }

    return ((xchar *)instance) - node->data->instance.private_size;
}

xint x_type_class_get_instance_private_offset(xpointer x_class)
{
    TypeNode *node;
    XType instance_type;
    xuint16 parent_size;

    x_assert(x_class != NULL);

    instance_type = ((XTypeClass *)x_class)->x_type;
    node = lookup_type_node_I(instance_type);

    x_assert(node != NULL);
    x_assert(node->is_instantiatable);

    if (NODE_PARENT_TYPE(node)) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE(node));
        parent_size = pnode->data->instance.private_size;
    } else {
        parent_size = 0;
    }

    if (node->data->instance.private_size == parent_size) {
        x_error("g_type_class_get_instance_private_offset() called on class %s but it has no private data", x_type_name(instance_type));
    }

    return -(xint)node->data->instance.private_size;
}

void x_type_add_class_private(XType class_type, xsize private_size)
{
    xsize offset;
    TypeNode *node = lookup_type_node_I(class_type);

    x_return_if_fail(private_size > 0);

    if (!node || !node->is_classed || !node->data) {
        x_critical("cannot add class private field to invalid type '%s'", type_descriptive_name_I(class_type));
        return;
    }

    if (NODE_PARENT_TYPE(node)) {
        TypeNode *pnode = lookup_type_node_I(NODE_PARENT_TYPE(node));
        if (node->data->classt.class_private_size != pnode->data->classt.class_private_size) {
            x_critical("g_type_add_class_private() called multiple times for the same type");
            return;
        }
    }

    X_WRITE_LOCK(&type_rw_lock);

    offset = ALIGN_STRUCT(node->data->classt.class_private_size);
    node->data->classt.class_private_size = offset + private_size;

    X_WRITE_UNLOCK(&type_rw_lock);
}

xpointer x_type_class_get_private(XTypeClass *klass, XType private_type)
{
    xsize offset;
    TypeNode *class_node;
    TypeNode *private_node;
    TypeNode *parent_node;

    x_return_val_if_fail(klass != NULL, NULL);

    class_node = lookup_type_node_I(klass->x_type);
    if (X_UNLIKELY(!class_node || !class_node->is_classed)) {
        x_critical("class of invalid type '%s'", type_descriptive_name_I(klass->x_type));
        return NULL;
    }

    private_node = lookup_type_node_I(private_type);
    if (X_UNLIKELY(!private_node || !NODE_IS_ANCESTOR(private_node, class_node))) {
        x_critical("attempt to retrieve private data for invalid type '%s'", type_descriptive_name_I(private_type));
        return NULL;
    }

    offset = ALIGN_STRUCT(class_node->data->classt.class_size);

    if (NODE_PARENT_TYPE(private_node)) {
        parent_node = lookup_type_node_I(NODE_PARENT_TYPE(private_node));
        x_assert(parent_node->data && NODE_REFCOUNT(parent_node) > 0);

        if (X_UNLIKELY(private_node->data->classt.class_private_size == parent_node->data->classt.class_private_size)) {
            x_critical("g_type_instance_get_class_private() requires a prior call to x_type_add_class_private()");
            return NULL;
        }

        offset += ALIGN_STRUCT(parent_node->data->classt.class_private_size);
    }

    return X_STRUCT_MEMBER_P(klass, offset);
}

void x_type_ensure(XType type)
{
    if (X_UNLIKELY(type == (XType)-1)) {
        x_error("can't happen");
    }
}
