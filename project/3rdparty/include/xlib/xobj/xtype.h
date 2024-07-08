#ifndef __X_TYPE_H__
#define __X_TYPE_H__

#include "../xlib.h"
#include "xobject-visibility.h"

X_BEGIN_DECLS

#define X_TYPE_FUNDAMENTAL_SHIFT                        (2)
#define X_TYPE_MAKE_FUNDAMENTAL(x)                      ((XType)((x) << X_TYPE_FUNDAMENTAL_SHIFT))
#define X_TYPE_RESERVED_XLIB_FIRST                      (22)
#define X_TYPE_RESERVED_XLIB_LAST                       (31)
#define X_TYPE_RESERVED_BSE_FIRST                       (32)
#define X_TYPE_RESERVED_BSE_LAST                        (48)
#define X_TYPE_RESERVED_USER_FIRST                      (49)

#define X_TYPE_FUNDAMENTAL(type)                        (x_type_fundamental(type))
#define X_TYPE_FUNDAMENTAL_MAX                          (255 << X_TYPE_FUNDAMENTAL_SHIFT)
#define X_TYPE_INVALID                                  X_TYPE_MAKE_FUNDAMENTAL(0)
#define X_TYPE_NONE                                     X_TYPE_MAKE_FUNDAMENTAL(1)
#define X_TYPE_INTERFACE                                X_TYPE_MAKE_FUNDAMENTAL(2)
#define X_TYPE_CHAR                                     X_TYPE_MAKE_FUNDAMENTAL(3)
#define X_TYPE_UCHAR                                    X_TYPE_MAKE_FUNDAMENTAL(4)
#define X_TYPE_BOOLEAN                                  X_TYPE_MAKE_FUNDAMENTAL(5)
#define X_TYPE_INT                                      X_TYPE_MAKE_FUNDAMENTAL(6)
#define X_TYPE_UINT                                     X_TYPE_MAKE_FUNDAMENTAL(7)
#define X_TYPE_LONG                                     X_TYPE_MAKE_FUNDAMENTAL(8)
#define X_TYPE_ULONG                                    X_TYPE_MAKE_FUNDAMENTAL(9)
#define X_TYPE_INT64                                    X_TYPE_MAKE_FUNDAMENTAL(10)
#define X_TYPE_UINT64                                   X_TYPE_MAKE_FUNDAMENTAL(11)
#define X_TYPE_ENUM                                     X_TYPE_MAKE_FUNDAMENTAL(12)
#define X_TYPE_FLAGS                                    X_TYPE_MAKE_FUNDAMENTAL(13)
#define X_TYPE_FLOAT                                    X_TYPE_MAKE_FUNDAMENTAL(14)
#define X_TYPE_DOUBLE                                   X_TYPE_MAKE_FUNDAMENTAL(15)
#define X_TYPE_STRING                                   X_TYPE_MAKE_FUNDAMENTAL(16)
#define X_TYPE_POINTER                                  X_TYPE_MAKE_FUNDAMENTAL(17)
#define X_TYPE_BOXED                                    X_TYPE_MAKE_FUNDAMENTAL(18)
#define X_TYPE_PARAM                                    X_TYPE_MAKE_FUNDAMENTAL(19)
#define X_TYPE_OBJECT                                   X_TYPE_MAKE_FUNDAMENTAL(20)
#define X_TYPE_VARIANT                                  X_TYPE_MAKE_FUNDAMENTAL(21)

#define X_TYPE_IS_FUNDAMENTAL(type)                     ((type) <= X_TYPE_FUNDAMENTAL_MAX)
#define X_TYPE_IS_DERIVED(type)                         ((type) > X_TYPE_FUNDAMENTAL_MAX)
#define X_TYPE_IS_INTERFACE(type)                       (X_TYPE_FUNDAMENTAL(type) == X_TYPE_INTERFACE)
#define X_TYPE_IS_CLASSED(type)                         (x_type_test_flags((type), X_TYPE_FLAG_CLASSED))
#define X_TYPE_IS_INSTANTIATABLE(type)                  (x_type_test_flags((type), X_TYPE_FLAG_INSTANTIATABLE))
#define X_TYPE_IS_DERIVABLE(type)                       (x_type_test_flags((type), X_TYPE_FLAG_DERIVABLE))
#define X_TYPE_IS_DEEP_DERIVABLE(type)                  (x_type_test_flags((type), X_TYPE_FLAG_DEEP_DERIVABLE))
#define X_TYPE_IS_ABSTRACT(type)                        (x_type_test_flags((type), X_TYPE_FLAG_ABSTRACT))
#define X_TYPE_IS_VALUE_ABSTRACT(type)                  (x_type_test_flags((type), X_TYPE_FLAG_VALUE_ABSTRACT))
#define X_TYPE_IS_VALUE_TYPE(type)                      (x_type_check_is_value_type(type))
#define X_TYPE_HAS_VALUE_TABLE(type)                    (x_type_value_table_peek(type) != NULL)
#define X_TYPE_IS_FINAL(type)                           (x_type_test_flags((type), X_TYPE_FLAG_FINAL)) XLIB_AVAILABLE_MACRO_IN_2_70
#define X_TYPE_IS_DEPRECATED(type)                      (x_type_test_flags((type), X_TYPE_FLAG_DEPRECATED)) XLIB_AVAILABLE_MACRO_IN_2_76

#if XLIB_SIZEOF_VOID_P > XLIB_SIZEOF_SIZE_T
typedef xuintptr XType;
#elif XLIB_SIZEOF_SIZE_T != XLIB_SIZEOF_LONG || !defined(X_CXX_STD_VERSION)
typedef xsize XType;
#else
typedef xulong XType;
#endif

typedef struct _XValue XValue;
typedef struct _XTypeInfo XTypeInfo;
typedef struct _XTypeQuery XTypeQuery;
typedef struct _XTypeClass XTypeClass;
typedef union _XTypeCValue XTypeCValue;
typedef struct _XTypePlugin XTypePlugin;
typedef struct _XTypeInstance XTypeInstance;
typedef struct _XTypeInterface XTypeInterface;
typedef struct _XInterfaceInfo XInterfaceInfo;
typedef struct _XTypeValueTable XTypeValueTable;
typedef struct _XTypeFundamentalInfo XTypeFundamentalInfo;

struct _XTypeClass {
    XType x_type;
};

struct _XTypeInstance {
    XTypeClass *x_class;
};

struct _XTypeInterface {
    XType x_type;
    XType x_instance_type;
};

struct _XTypeQuery
{
    XType       type;
    const xchar *type_name;
    xuint       class_size;
    xuint       instance_size;
};

typedef enum  {
    X_TYPE_DEBUG_NONE           = 0,
    X_TYPE_DEBUG_OBJECTS        = 1 << 0,
    X_TYPE_DEBUG_SIGNALS        = 1 << 1,
    X_TYPE_DEBUG_INSTANCE_COUNT = 1 << 2,
    X_TYPE_DEBUG_MASK           = 0x07
} XTypeDebugFlags XLIB_DEPRECATED_TYPE_IN_2_36;

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
XLIB_DEPRECATED_IN_2_36
void x_type_init(void);

XLIB_DEPRECATED_IN_2_36
void x_type_init_with_debug_flags(XTypeDebugFlags debug_flags);
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
const xchar *x_type_name(XType type);

XLIB_AVAILABLE_IN_ALL
XQuark x_type_qname(XType type);

XLIB_AVAILABLE_IN_ALL
XType x_type_from_name(const xchar *name);

XLIB_AVAILABLE_IN_ALL
XType x_type_parent(XType type);

XLIB_AVAILABLE_IN_ALL
xuint x_type_depth(XType type);

XLIB_AVAILABLE_IN_ALL
XType x_type_next_base(XType leaf_type, XType root_type);

XLIB_AVAILABLE_IN_ALL
xboolean x_type_is_a(XType type, XType is_a_type);

#define x_type_is_a(a, b)               ((a) == (b) || (x_type_is_a)((a), (b)))

XLIB_AVAILABLE_IN_ALL
xpointer x_type_class_ref(XType type);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_class_peek(XType type);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_class_peek_static(XType type);

XLIB_AVAILABLE_IN_ALL
void x_type_class_unref(xpointer x_class);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_class_peek_parent(xpointer x_class);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_interface_peek(xpointer instance_class, XType iface_type);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_interface_peek_parent(xpointer x_iface);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_default_interface_ref(XType x_type);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_default_interface_peek(XType x_type);

XLIB_AVAILABLE_IN_ALL
void x_type_default_interface_unref(xpointer x_iface);

XLIB_AVAILABLE_IN_ALL
XType *x_type_children(XType type, xuint *n_children);

XLIB_AVAILABLE_IN_ALL
XType *x_type_interfaces(XType type, xuint *n_interfaces);

XLIB_AVAILABLE_IN_ALL
void x_type_set_qdata(XType type, XQuark quark, xpointer data);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_get_qdata(XType type, XQuark quark);

XLIB_AVAILABLE_IN_ALL
void x_type_query(XType type, XTypeQuery *query);

XLIB_AVAILABLE_IN_2_44
int x_type_get_instance_count(XType type);

typedef void (*XBaseInitFunc)(xpointer x_class);
typedef void (*XBaseFinalizeFunc)(xpointer x_class);

typedef void (*XClassInitFunc)(xpointer x_class, xpointer class_data);
typedef void (*XClassFinalizeFunc)(xpointer x_class, xpointer class_data);

typedef void (*XInstanceInitFunc)(XTypeInstance *instance, xpointer x_class);
typedef void (*XInterfaceInitFunc)(xpointer x_iface, xpointer iface_data);
typedef void (*XInterfaceFinalizeFunc)(xpointer x_iface, xpointer iface_data);

typedef xboolean (*XTypeClassCacheFunc)(xpointer cache_data, XTypeClass *x_class);
typedef void (*XTypeInterfaceCheckFunc)(xpointer check_data, xpointer x_iface);

typedef enum {
    X_TYPE_FLAG_CLASSED        = (1 << 0),
    X_TYPE_FLAG_INSTANTIATABLE = (1 << 1),
    X_TYPE_FLAG_DERIVABLE      = (1 << 2),
    X_TYPE_FLAG_DEEP_DERIVABLE = (1 << 3)
} XTypeFundamentalFlags;

typedef enum {
    X_TYPE_FLAG_NONE XLIB_AVAILABLE_ENUMERATOR_IN_2_74       = 0,
    X_TYPE_FLAG_ABSTRACT                                     = (1 << 4),
    X_TYPE_FLAG_VALUE_ABSTRACT                               = (1 << 5),
    X_TYPE_FLAG_FINAL XLIB_AVAILABLE_ENUMERATOR_IN_2_70      = (1 << 6),
    X_TYPE_FLAG_DEPRECATED XLIB_AVAILABLE_ENUMERATOR_IN_2_76 = (1 << 7)
} XTypeFlags;

struct _XTypeInfo {
    xuint16               class_size;

    XBaseInitFunc         base_init;
    XBaseFinalizeFunc     base_finalize;

    XClassInitFunc        class_init;
    XClassFinalizeFunc    class_finalize;
    xconstpointer         class_data;

    xuint16               instance_size;
    xuint16               n_preallocs;
    XInstanceInitFunc     instance_init;

    const XTypeValueTable *value_table;
};

struct _XTypeFundamentalInfo {
    XTypeFundamentalFlags type_flags;
};

struct _XInterfaceInfo {
    XInterfaceInitFunc     interface_init;
    XInterfaceFinalizeFunc interface_finalize;
    xpointer               interface_data;
};

XLIB_AVAILABLE_TYPE_IN_2_78
typedef void (*XTypeValueInitFunc)(XValue *value);

XLIB_AVAILABLE_TYPE_IN_2_78
typedef void (*XTypeValueFreeFunc)(XValue *value);

XLIB_AVAILABLE_TYPE_IN_2_78
typedef void (*XTypeValueCopyFunc)(const XValue *src_value, XValue *dest_value);

XLIB_AVAILABLE_TYPE_IN_2_78
typedef xpointer (*XTypeValuePeekPointerFunc)(const XValue *value);

XLIB_AVAILABLE_TYPE_IN_2_78
typedef xchar *(*XTypeValueCollectFunc)(XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);

XLIB_AVAILABLE_TYPE_IN_2_78
typedef xchar *(*XTypeValueLCopyFunc)(const XValue *value, xuint n_collect_values, XTypeCValue *collect_values, xuint collect_flags);

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
struct _XTypeValueTable {
    XTypeValueInitFunc        value_init;
    XTypeValueFreeFunc        value_free;
    XTypeValueCopyFunc        value_copy;
    XTypeValuePeekPointerFunc value_peek_pointer;

    const xchar               *collect_format;
    XTypeValueCollectFunc     collect_value;
    const xchar               *lcopy_format;
    XTypeValueLCopyFunc       lcopy_value;
};
X_GNUC_END_IGNORE_DEPRECATIONS

XLIB_AVAILABLE_IN_ALL
XType x_type_register_static(XType parent_type, const xchar *type_name, const XTypeInfo *info, XTypeFlags flags);

XLIB_AVAILABLE_IN_ALL
XType x_type_register_static_simple(XType parent_type, const xchar *type_name, xuint class_size, XClassInitFunc class_init, xuint instance_size, XInstanceInitFunc instance_init, XTypeFlags flags);
  
XLIB_AVAILABLE_IN_ALL
XType x_type_register_dynamic(XType parent_type, const xchar *type_name, XTypePlugin *plugin, XTypeFlags flags);

XLIB_AVAILABLE_IN_ALL
XType x_type_register_fundamental(XType type_id, const xchar *type_name, const XTypeInfo *info, const XTypeFundamentalInfo *finfo, XTypeFlags flags);

XLIB_AVAILABLE_IN_ALL
void x_type_add_interface_static(XType instance_type, XType interface_type, const XInterfaceInfo *info);

XLIB_AVAILABLE_IN_ALL
void x_type_add_interface_dynamic(XType instance_type, XType interface_type, XTypePlugin *plugin);

XLIB_AVAILABLE_IN_ALL
void x_type_interface_add_prerequisite(XType interface_type, XType prerequisite_type);

XLIB_AVAILABLE_IN_ALL
XType *x_type_interface_prerequisites(XType interface_type, xuint *n_prerequisites);

XLIB_AVAILABLE_IN_2_68
XType x_type_interface_instantiatable_prerequisite(XType interface_type);

XLIB_DEPRECATED_IN_2_58
void x_type_class_add_private(xpointer x_class, xsize private_size);

XLIB_AVAILABLE_IN_2_38
xint x_type_add_instance_private(XType class_type, xsize private_size);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_instance_get_private(XTypeInstance *instance, XType private_type);

XLIB_AVAILABLE_IN_2_38
void x_type_class_adjust_private_offset(xpointer x_class, xint *private_size_or_offset);

XLIB_AVAILABLE_IN_ALL
void x_type_add_class_private(XType class_type, xsize private_size);

XLIB_AVAILABLE_IN_ALL
xpointer x_type_class_get_private(XTypeClass *klass, XType private_type);

XLIB_AVAILABLE_IN_2_38
xint x_type_class_get_instance_private_offset(xpointer x_class);

XLIB_AVAILABLE_IN_2_34
void x_type_ensure(XType type);

XLIB_AVAILABLE_IN_2_36
xuint x_type_get_type_registration_serial(void);

#define X_DECLARE_FINAL_TYPE(ModuleObjName, module_obj_name, MODULE, OBJ_NAME, ParentName)                  \
    XType module_obj_name##_get_type(void);                                                                 \
    X_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                        \
    typedef struct _##ModuleObjName ModuleObjName;                                                          \
    typedef struct { ParentName##Class parent_class; } ModuleObjName##Class;                                \
                                                                                                            \
    _XLIB_DEFINE_AUTOPTR_CHAINUP(ModuleObjName, ParentName)                                                 \
    X_DEFINE_AUTOPTR_CLEANUP_FUNC(ModuleObjName##Class, x_type_class_unref)                                 \
                                                                                                            \
    X_GNUC_UNUSED static inline ModuleObjName *MODULE##_##OBJ_NAME(xpointer ptr)                            \
    {                                                                                                       \
        return X_TYPE_CHECK_INSTANCE_CAST(ptr, module_obj_name##_get_type(), ModuleObjName);                \
    }                                                                                                       \
    X_GNUC_UNUSED static inline xboolean MODULE##_IS_##OBJ_NAME(xpointer ptr)                               \
    {                                                                                                       \
        return X_TYPE_CHECK_INSTANCE_TYPE(ptr, module_obj_name##_get_type());                               \
    }                                                                                                       \
    X_GNUC_END_IGNORE_DEPRECATIONS

#define X_DECLARE_DERIVABLE_TYPE(ModuleObjName, module_obj_name, MODULE, OBJ_NAME, ParentName)              \
    XType module_obj_name##_get_type(void);                                                                 \
    X_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                        \
    typedef struct _##ModuleObjName ModuleObjName;                                                          \
    typedef struct _##ModuleObjName##Class ModuleObjName##Class;                                            \
    struct _##ModuleObjName { ParentName parent_instance; };                                                \
                                                                                                            \
    _XLIB_DEFINE_AUTOPTR_CHAINUP(ModuleObjName, ParentName)                                                 \
    X_DEFINE_AUTOPTR_CLEANUP_FUNC(ModuleObjName##Class, x_type_class_unref)                                 \
                                                                                                            \
    X_GNUC_UNUSED static inline ModuleObjName *MODULE##_##OBJ_NAME(xpointer ptr)                            \
    {                                                                                                       \
        return X_TYPE_CHECK_INSTANCE_CAST(ptr, module_obj_name##_get_type(), ModuleObjName);                \
    }                                                                                                       \
    X_GNUC_UNUSED static inline ModuleObjName##Class *MODULE##_##OBJ_NAME##_CLASS(xpointer ptr)             \
    {                                                                                                       \
        return X_TYPE_CHECK_CLASS_CAST(ptr, module_obj_name##_get_type(), ModuleObjName##Class);            \
    }                                                                                                       \
    X_GNUC_UNUSED static inline xboolean MODULE##_IS_##OBJ_NAME(xpointer ptr)                               \
    {                                                                                                       \
        return X_TYPE_CHECK_INSTANCE_TYPE(ptr, module_obj_name##_get_type());                               \
    }                                                                                                       \
    X_GNUC_UNUSED static inline xboolean MODULE##_IS_##OBJ_NAME##_CLASS(xpointer ptr)                       \
    {                                                                                                       \
        return X_TYPE_CHECK_CLASS_TYPE(ptr, module_obj_name##_get_type());                                  \
    }                                                                                                       \
    X_GNUC_UNUSED static inline ModuleObjName##Class *MODULE##_##OBJ_NAME##_GET_CLASS(xpointer ptr)         \
    {                                                                                                       \
        return X_TYPE_INSTANCE_GET_CLASS(ptr, module_obj_name##_get_type(), ModuleObjName##Class);          \
    }                                                                                                       \
    X_GNUC_END_IGNORE_DEPRECATIONS

#define X_DECLARE_INTERFACE(ModuleObjName, module_obj_name, MODULE, OBJ_NAME, PrerequisiteName)             \
    XType module_obj_name##_get_type(void);                                                                 \
    X_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                        \
    typedef struct _##ModuleObjName ModuleObjName;                                                          \
    typedef struct _##ModuleObjName##Interface ModuleObjName##Interface;                                    \
                                                                                                            \
    _XLIB_DEFINE_AUTOPTR_CHAINUP(ModuleObjName, PrerequisiteName)                                           \
                                                                                                            \
    X_GNUC_UNUSED static inline ModuleObjName *MODULE##_##OBJ_NAME(xpointer ptr)                            \
    {                                                                                                       \
        return X_TYPE_CHECK_INSTANCE_CAST(ptr, module_obj_name##_get_type(), ModuleObjName);                \
    }                                                                                                       \
    X_GNUC_UNUSED static inline xboolean MODULE##_IS_##OBJ_NAME(xpointer ptr)                               \
    {                                                                                                       \
        return X_TYPE_CHECK_INSTANCE_TYPE(ptr, module_obj_name##_get_type());                               \
    }                                                                                                       \
    X_GNUC_UNUSED static inline ModuleObjName##Interface *MODULE##_##OBJ_NAME##_GET_IFACE(xpointer ptr)     \
    {                                                                                                       \
        return X_TYPE_INSTANCE_GET_INTERFACE(ptr, module_obj_name##_get_type(), ModuleObjName##Interface);  \
    }                                                                                                       \
    X_GNUC_END_IGNORE_DEPRECATIONS

#define X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, _f_, _C_)          _X_DEFINE_TYPE_EXTENDED_BEGIN(TN, t_n, T_P, _f_) {_C_;} _X_DEFINE_TYPE_EXTENDED_END()
#define X_DEFINE_INTERFACE_WITH_CODE(TN, t_n, T_P, _C_)         _X_DEFINE_INTERFACE_EXTENDED_BEGIN(TN, t_n, T_P) {_C_;} _X_DEFINE_INTERFACE_EXTENDED_END()
#define X_DEFINE_INTERFACE(TN, t_n, T_P)                        X_DEFINE_INTERFACE_WITH_CODE(TN, t_n, T_P, ;)

#define X_DEFINE_TYPE(TN, t_n, T_P)                             X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, 0, {})
#define X_DEFINE_TYPE_WITH_CODE(TN, t_n, T_P, _C_)              _X_DEFINE_TYPE_EXTENDED_BEGIN(TN, t_n, T_P, 0) {_C_;} _X_DEFINE_TYPE_EXTENDED_END()
#define X_DEFINE_TYPE_WITH_PRIVATE(TN, t_n, T_P)                X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, 0, X_ADD_PRIVATE(TN))

#define X_DEFINE_ABSTRACT_TYPE(TN, t_n, T_P)                    X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, X_TYPE_FLAG_ABSTRACT, {})
#define X_DEFINE_ABSTRACT_TYPE_WITH_CODE(TN, t_n, T_P, _C_)     _X_DEFINE_TYPE_EXTENDED_BEGIN(TN, t_n, T_P, X_TYPE_FLAG_ABSTRACT) {_C_;} _X_DEFINE_TYPE_EXTENDED_END()
#define X_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(TN, t_n, T_P)       X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, X_TYPE_FLAG_ABSTRACT, X_ADD_PRIVATE(TN))

#define X_DEFINE_FINAL_TYPE(TN, t_n, T_P)                       X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, X_TYPE_FLAG_FINAL, {}) XLIB_AVAILABLE_MACRO_IN_2_70
#define X_DEFINE_FINAL_TYPE_WITH_CODE(TN, t_n, T_P, _C_)        _X_DEFINE_TYPE_EXTENDED_BEGIN(TN, t_n, T_P, X_TYPE_FLAG_FINAL) {_C_;} _X_DEFINE_TYPE_EXTENDED_END() XLIB_AVAILABLE_MACRO_IN_2_70
#define X_DEFINE_FINAL_TYPE_WITH_PRIVATE(TN, t_n, T_P)          X_DEFINE_TYPE_EXTENDED(TN, t_n, T_P, X_TYPE_FLAG_FINAL, X_ADD_PRIVATE(TN)) XLIB_AVAILABLE_MACRO_IN_2_70

#define X_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init)                                           \
    {                                                                                           \
        const XInterfaceInfo x_implement_interface_info = {                                     \
            (XInterfaceInitFunc)(void (*)(void))iface_init, NULL, NULL                          \
        };                                                                                      \
        x_type_add_interface_static(x_define_type_id, TYPE_IFACE, &x_implement_interface_info); \
    }

#define X_ADD_PRIVATE(TypeName)                                                                               \
    {                                                                                                         \
        TypeName##_private_offset = x_type_add_instance_private(x_define_type_id, sizeof(TypeName##Private)); \
    }

#define X_PRIVATE_OFFSET(TypeName, field)                           \
    (TypeName##_private_offset + (X_STRUCT_OFFSET(TypeName##Private, field)))

#define X_PRIVATE_FIELD_P(TypeName, inst, field_name)               \
    X_STRUCT_MEMBER_P(inst, X_PRIVATE_OFFSET(TypeName, field_name))

#define X_PRIVATE_FIELD(TypeName, inst, field_type, field_name)     \
    X_STRUCT_MEMBER(field_type, inst, X_PRIVATE_OFFSET(TypeName, field_name))

#if XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_38
#define _X_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name)                     \
    static void type_name##_class_intern_init(xpointer klass)                       \
    {                                                                               \
        type_name##_parent_class = x_type_class_peek_parent(klass);                 \
        if (TypeName##_private_offset != 0) {                                       \
            x_type_class_adjust_private_offset(klass, &TypeName##_private_offset);  \
        }                                                                           \
        type_name##_class_init((TypeName##Class *)klass);                           \
    }

#else

#define _X_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name)                     \
    static void type_name##_class_intern_init(xpointer klass)                       \
    {                                                                               \
        type_name##_parent_class = x_type_class_peek_parent(klass);                 \
        type_name##_class_init((TypeName##Class *)klass);                           \
    }
#endif

#if XLIB_VERSION_MAX_ALLOWED >= XLIB_VERSION_2_80
#define _x_type_once_init_type      XType
#define _x_type_once_init_enter     x_once_init_enter_pointer
#define _x_type_once_init_leave     x_once_init_leave_pointer
#else
#define _x_type_once_init_type      xsize
#define _x_type_once_init_enter     x_once_init_enter
#define _x_type_once_init_leave     x_once_init_leave
#endif

#define _X_DEFINE_TYPE_EXTENDED_BEGIN_PRE(TypeName, type_name)                          \
                                                                                        \
    static void type_name##_init(TypeName *self);                                       \
    static void type_name##_class_init(TypeName##Class *klass);                         \
    static XType type_name##_get_type_once(void);                                       \
    static xpointer type_name##_parent_class = NULL;                                    \
    static xint TypeName##_private_offset;                                              \
                                                                                        \
    _X_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name)                             \
                                                                                        \
    X_GNUC_UNUSED                                                                       \
    static inline xpointer type_name##_get_instance_private(TypeName *self)             \
    {                                                                                   \
        return (X_STRUCT_MEMBER_P(self, TypeName##_private_offset));                    \
    }                                                                                   \
                                                                                        \
    XType type_name##_get_type(void)                                                    \
    {                                                                                   \
        static _x_type_once_init_type static_x_define_type_id = 0;

#define _X_DEFINE_TYPE_EXTENDED_BEGIN_REGISTER(TypeName, type_name, TYPE_PARENT, flags)             \
    if (_x_type_once_init_enter(&static_x_define_type_id)) {                                        \
        XType x_define_type_id = type_name##_get_type_once();                                       \
        _x_type_once_init_leave(&static_x_define_type_id, x_define_type_id);                        \
    }                                                                                               \
    return static_x_define_type_id;                                                                 \
    }                                                                                               \
                                                                                                    \
    X_NO_INLINE                                                                                     \
    static XType type_name##_get_type_once(void)                                                    \
    {                                                                                               \
        XType x_define_type_id =                                                                    \
                x_type_register_static_simple(TYPE_PARENT,                                          \
                                            x_intern_static_string(#TypeName),                      \
                                            sizeof(TypeName##Class),                                \
                                            (XClassInitFunc)(void (*)(void))type_name##_class_intern_init, \
                                            sizeof(TypeName),                                       \
                                            (XInstanceInitFunc)(void (*)(void))type_name##_init,    \
                                            (XTypeFlags)flags);                                      \
            {

#define _X_DEFINE_TYPE_EXTENDED_END()   \
    }                                   \
    return x_define_type_id;            \
}

#define _X_DEFINE_TYPE_EXTENDED_BEGIN(TypeName, type_name, TYPE_PARENT, flags)      \
    _X_DEFINE_TYPE_EXTENDED_BEGIN_PRE(TypeName, type_name)                          \
    _X_DEFINE_TYPE_EXTENDED_BEGIN_REGISTER(TypeName, type_name, TYPE_PARENT, flags) \

#define _X_DEFINE_INTERFACE_EXTENDED_BEGIN(TypeName, type_name, TYPE_PREREQ)                            \
                                                                                                        \
    static void type_name##_default_init(TypeName##Interface *klass);                                   \
                                                                                                        \
    XType type_name##_get_type(void)                                                                    \
    {                                                                                                   \
        static _x_type_once_init_type static_x_define_type_id = 0;                                      \
        if (_x_type_once_init_enter(&static_x_define_type_id)) {                                        \
            XType x_define_type_id =                                                                    \
                x_type_register_static_simple(X_TYPE_INTERFACE,                                         \
                                            x_intern_static_string(#TypeName),                          \
                                            sizeof(TypeName##Interface),                                \
                                            (XClassInitFunc)(void (*)(void))type_name##_default_init,   \
                                            0,                                                          \
                                            (XInstanceInitFunc)NULL,                                    \
                                            (XTypeFlags)0);                                             \
            if (TYPE_PREREQ != X_TYPE_INVALID)                                                          \
                x_type_interface_add_prerequisite(x_define_type_id, TYPE_PREREQ);                       \
            {

#define _X_DEFINE_INTERFACE_EXTENDED_END()                              \
    }                                                                   \
      _x_type_once_init_leave(&static_x_define_type_id, x_define_type_id);    \
    }                                                                   \
    return static_x_define_type_id;                                     \
}

#define X_DEFINE_BOXED_TYPE(TypeName, type_name, copy_func, free_func)                  X_DEFINE_BOXED_TYPE_WITH_CODE(TypeName, type_name, copy_func, free_func, {})
#define X_DEFINE_BOXED_TYPE_WITH_CODE(TypeName, type_name, copy_func, free_func, _C_)   _X_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func) {_C_;} _X_DEFINE_TYPE_EXTENDED_END()

#if !defined (X_CXX_STD_VERSION) && (X_GNUC_CHECK_VERSION(2, 7)) && !(defined (__APPLE__) && defined (__ppc64__))
#define _X_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func)                                                                           \
    static XType type_name##_get_type_once(void);                                                                                                       \
                                                                                                                                                        \
    XType type_name##_get_type(void)                                                                                                                    \
    {                                                                                                                                                   \
        static _x_type_once_init_type static_x_define_type_id = 0;                                                                                      \
        if (_x_type_once_init_enter(&static_x_define_type_id)) {                                                                                        \
            XType x_define_type_id = type_name##_get_type_once ();                                                                                      \
            _x_type_once_init_leave(&static_x_define_type_id, x_define_type_id);                                                                        \
        }                                                                                                                                               \
        return static_x_define_type_id;                                                                                                                 \
    }                                                                                                                                                   \
                                                                                                                                                        \
    X_NO_INLINE static XType type_name##_get_type_once(void)                                                                                            \
    {                                                                                                                                                   \
        XType (* _x_register_boxed)                                                                                                                     \
            (const xchar *,                                                                                                                             \
            union                                                                                                                                       \
            {                                                                                                                                           \
                TypeName * (*do_copy_type)(TypeName *);                                                                                                 \
                TypeName * (*do_const_copy_type)(const TypeName *);                                                                                     \
                XBoxedCopyFunc do_copy_boxed;                                                                                                           \
            } __attribute__((__transparent_union__)),                                                                                                   \
            union                                                                                                                                       \
            {                                                                                                                                           \
                void (* do_free_type)(TypeName *);                                                                                                      \
                XBoxedFreeFunc do_free_boxed;                                                                                                           \
            } __attribute__((__transparent_union__))                                                                                                    \
            ) = x_boxed_type_register_static;                                                                                                           \
        XType x_define_type_id = _x_register_boxed(x_intern_static_string(#TypeName), copy_func, free_func);                                            \
        {

#else

#define _X_DEFINE_BOXED_TYPE_BEGIN(TypeName, type_name, copy_func, free_func)                                                                           \
    static XType type_name##_get_type_once(void);                                                                                                       \
                                                                                                                                                        \
    XType type_name##_get_type(void)                                                                                                                    \
    {                                                                                                                                                   \
        static _x_type_once_init_type static_x_define_type_id = 0;                                                                                      \
        if (_x_type_once_init_enter(&static_x_define_type_id)) {                                                                                        \
            XType x_define_type_id = type_name##_get_type_once();                                                                                       \
            _x_type_once_init_leave(&static_x_define_type_id, x_define_type_id);                                                                        \
        }                                                                                                                                               \
        return static_x_define_type_id;                                                                                                                 \
    }                                                                                                                                                   \
                                                                                                                                                        \
    X_NO_INLINE static XType type_name##_get_type_once(void)                                                                                            \
    {                                                                                                                                                   \
        XType x_define_type_id = x_boxed_type_register_static(x_intern_static_string(#TypeName), (XBoxedCopyFunc)copy_func, (XBoxedFreeFunc)free_func); \
        {
#endif

#define _X_DEFINE_POINTER_TYPE_BEGIN(TypeName, type_name)                                                       \
    static XType type_name##_get_type_once(void);                                                               \
                                                                                                                \
    XType type_name##_get_type(void)                                                                            \
    {                                                                                                           \
        static _x_type_once_init_type static_x_define_type_id = 0;                                              \
        if (_x_type_once_init_enter(&static_x_define_type_id)) {                                                \
            XType x_define_type_id = type_name##_get_type_once();                                               \
            _x_type_once_init_leave(&static_x_define_type_id, x_define_type_id);                                \
        }                                                                                                       \
        return static_x_define_type_id;                                                                         \
    }                                                                                                           \
                                                                                                                \
    X_NO_INLINE static XType type_name##_get_type_once(void)                                                    \
    {                                                                                                           \
        XType x_define_type_id = x_pointer_type_register_static(x_intern_static_string(#TypeName));             \
        {

#define X_DEFINE_POINTER_TYPE_WITH_CODE(TypeName, type_name, _C_)   _X_DEFINE_POINTER_TYPE_BEGIN(TypeName, type_name) {_C_;} _X_DEFINE_TYPE_EXTENDED_END()
#define X_DEFINE_POINTER_TYPE(TypeName, type_name)                  X_DEFINE_POINTER_TYPE_WITH_CODE(TypeName, type_name, {})

XLIB_AVAILABLE_IN_ALL
XTypePlugin *x_type_get_plugin(XType type);

XLIB_AVAILABLE_IN_ALL
XTypePlugin *x_type_interface_get_plugin(XType instance_type, XType interface_type);

XLIB_AVAILABLE_IN_ALL
XType x_type_fundamental_next(void);

XLIB_AVAILABLE_IN_ALL
XType x_type_fundamental(XType type_id);

XLIB_AVAILABLE_IN_ALL
XTypeInstance *x_type_create_instance(XType type);

XLIB_AVAILABLE_IN_ALL
void x_type_free_instance(XTypeInstance *instance);

XLIB_AVAILABLE_IN_ALL
void x_type_add_class_cache_func(xpointer cache_data, XTypeClassCacheFunc cache_func);

XLIB_AVAILABLE_IN_ALL
void x_type_remove_class_cache_func(xpointer cache_data, XTypeClassCacheFunc cache_func);

XLIB_AVAILABLE_IN_ALL
void x_type_class_unref_uncached(xpointer x_class);

XLIB_AVAILABLE_IN_ALL
void x_type_add_interface_check(xpointer check_data, XTypeInterfaceCheckFunc check_func);

XLIB_AVAILABLE_IN_ALL
void x_type_remove_interface_check(xpointer check_data, XTypeInterfaceCheckFunc check_func);

XLIB_AVAILABLE_IN_ALL
XTypeValueTable *x_type_value_table_peek(XType type);

XLIB_AVAILABLE_IN_ALL
xboolean x_type_check_instance(XTypeInstance *instance) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
XTypeInstance *x_type_check_instance_cast(XTypeInstance *instance, XType iface_type);

XLIB_AVAILABLE_IN_ALL
xboolean x_type_check_instance_is_a(XTypeInstance *instance, XType iface_type) X_GNUC_PURE;

XLIB_AVAILABLE_IN_2_42
xboolean x_type_check_instance_is_fundamentally_a(XTypeInstance *instance, XType fundamental_type) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
XTypeClass *x_type_check_class_cast(XTypeClass *x_class, XType is_a_type);

XLIB_AVAILABLE_IN_ALL
xboolean x_type_check_class_is_a(XTypeClass *x_class, XType is_a_type) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xboolean x_type_check_is_value_type(XType type) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_type_check_value(const XValue *value) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xboolean x_type_check_value_holds(const XValue *value, XType type) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xboolean x_type_test_flags(XType type, xuint flags) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
const xchar *x_type_name_from_instance(XTypeInstance *instance);

XLIB_AVAILABLE_IN_ALL
const xchar *x_type_name_from_class(XTypeClass *x_class);

#define _X_TYPE_CIC(ip, gt, ct) \
    ((ct *)(void *)x_type_check_instance_cast((XTypeInstance *)ip, gt))

#define _X_TYPE_CCC(cp, gt, ct) \
    ((ct *)(void *)x_type_check_class_cast((XTypeClass *)cp, gt))

#define _X_TYPE_CHI(ip)                 (x_type_check_instance((XTypeInstance *)ip))
#define _X_TYPE_CHV(vl)                 (x_type_check_value((XValue *)vl))
#define _X_TYPE_IGC(ip, gt, ct)         ((ct *)(((XTypeInstance *)ip)->x_class))
#define _X_TYPE_IGI(ip, gt, ct)         ((ct *)x_type_interface_peek(((XTypeInstance *)ip)->x_class, gt))
#define _X_TYPE_CIFT(ip, ft)            (x_type_check_instance_is_fundamentally_a((XTypeInstance *)ip, ft))

#define _X_TYPE_CIT(ip, gt)                                                             \
    (X_GNUC_EXTENSION ({                                                                \
        XTypeInstance *__inst = (XTypeInstance *)ip; XType __t = gt; xboolean __r;      \
        if (!__inst) {                                                                  \
            __r = FALSE;                                                                \
        } else if (__inst->x_class && __inst->x_class->x_type == __t) {                 \
            __r = TRUE;                                                                 \
        } else {                                                                        \
            __r = x_type_check_instance_is_a(__inst, __t);                              \
        }                                                                               \
        __r;                                                                            \
    }))

#define _X_TYPE_CCT(cp, gt)                                                             \
    (X_GNUC_EXTENSION ({                                                                \
    XTypeClass *__class = (XTypeClass *)cp; XType __t = gt; xboolean __r;               \
        if (!__class) {                                                                 \
            __r = FALSE;                                                                \
        } else if (__class->x_type == __t) {                                            \
            __r = TRUE;                                                                 \
        } else {                                                                        \
            __r = x_type_check_class_is_a(__class, __t);                                \
        }                                                                               \
        __r;                                                                            \
    }))

#define _X_TYPE_CVH(vl, gt)                                                             \
    (X_GNUC_EXTENSION ({                                                                \
        const XValue *__val = (const XValue *)vl; XType __t = gt; xboolean __r;         \
        if (!__val) {                                                                   \
            __r = FALSE;                                                                \
        } else if (__val->x_type == __t) {                                              \
            __r = TRUE;                                                                 \
        } else {                                                                        \
            __r = x_type_check_value_holds(__val, __t);                                 \
        }                                                                               \
        __r;                                                                            \
    }))

#define X_TYPE_FLAG_RESERVED_ID_BIT                                 (((XType)(1 << 0)))

#define X_TYPE_CHECK_INSTANCE(instance)                             (_X_TYPE_CHI((XTypeInstance *)(instance)))
#define X_TYPE_CHECK_INSTANCE_CAST(instance, x_type, c_type)        (_X_TYPE_CIC((instance), (x_type), c_type))
#define X_TYPE_CHECK_INSTANCE_TYPE(instance, x_type)                (_X_TYPE_CIT((instance), (x_type)))
#define X_TYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE(instance, x_type)    (_X_TYPE_CIFT((instance), (x_type)))
#define X_TYPE_INSTANCE_GET_CLASS(instance, x_type, c_type)         (_X_TYPE_IGC((instance), (x_type), c_type))
#define X_TYPE_INSTANCE_GET_INTERFACE(instance, x_type, c_type)     (_X_TYPE_IGI((instance), (x_type), c_type))
#define X_TYPE_CHECK_CLASS_CAST(x_class, x_type, c_type)            (_X_TYPE_CCC((x_class), (x_type), c_type))
#define X_TYPE_CHECK_CLASS_TYPE(x_class, x_type)                    (_X_TYPE_CCT((x_class), (x_type)))
#define X_TYPE_CHECK_VALUE(value)                                   (_X_TYPE_CHV((value)))
#define X_TYPE_CHECK_VALUE_TYPE(value, x_type)                      (_X_TYPE_CVH((value), (x_type)))
#define X_TYPE_FROM_INSTANCE(instance)                              (X_TYPE_FROM_CLASS(((XTypeInstance *)(instance))->x_class))
#define X_TYPE_FROM_CLASS(x_class)                                  (((XTypeClass *)(x_class))->x_type)
#define X_TYPE_FROM_INTERFACE(x_iface)                              (((XTypeInterface *)(x_iface))->x_type)

#define X_TYPE_INSTANCE_GET_PRIVATE(instance, x_type, c_type)       ((c_type *)x_type_instance_get_private((XTypeInstance *)(instance), (x_type))) XLIB_DEPRECATED_MACRO_IN_2_58_FOR(X_ADD_PRIVATE)
#define X_TYPE_CLASS_GET_PRIVATE(klass, x_type, c_type)             ((c_type *)x_type_class_get_private((XTypeClass *)(klass), (x_type)))

#define XPOINTER_TO_TYPE(p)                                         ((XType)(xuintptr)(p)) XLIB_AVAILABLE_MACRO_IN_2_80
#define XTYPE_TO_POINTER(t)                                         ((xpointer)(xuintptr)(t)) XLIB_AVAILABLE_MACRO_IN_2_80

X_END_DECLS

#endif
