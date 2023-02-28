#ifndef __X_TYPE_MODULE_H__
#define __X_TYPE_MODULE_H__

#include "xobject.h"
#include "xenums.h"

X_BEGIN_DECLS

typedef struct _XTypeModule XTypeModule;
typedef struct _XTypeModuleClass XTypeModuleClass;

#define X_TYPE_TYPE_MODULE                      (x_type_module_get_type())
#define X_TYPE_MODULE(module)                   (X_TYPE_CHECK_INSTANCE_CAST((module), X_TYPE_TYPE_MODULE, XTypeModule))
#define X_TYPE_MODULE_CLASS(classt)             (X_TYPE_CHECK_CLASS_CAST((classt), X_TYPE_TYPE_MODULE, XTypeModuleClass))
#define X_IS_TYPE_MODULE(module)                (X_TYPE_CHECK_INSTANCE_TYPE((module), X_TYPE_TYPE_MODULE))
#define X_IS_TYPE_MODULE_CLASS(classt)          (X_TYPE_CHECK_CLASS_TYPE((classt), X_TYPE_TYPE_MODULE))
#define X_TYPE_MODULE_GET_CLASS(module)         (X_TYPE_INSTANCE_GET_CLASS((module), X_TYPE_TYPE_MODULE, XTypeModuleClass))

X_DEFINE_AUTOPTR_CLEANUP_FUNC(XTypeModule, x_object_unref)

struct _XTypeModule  {
    XObject parent_instance;
    xuint   use_count;
    XSList  *type_infos;
    XSList  *interface_infos;
    xchar   *name;
};

struct _XTypeModuleClass {
    XObjectClass parent_class;

    xboolean (*load)(XTypeModule *module);
    void (*unload)(XTypeModule *module);

    void (*reserved1)(void);
    void (*reserved2)(void);
    void (*reserved3)(void);
    void (*reserved4)(void);
};

#define X_DEFINE_DYNAMIC_TYPE(TN, t_n, T_P)     X_DEFINE_DYNAMIC_TYPE_EXTENDED(TN, t_n, T_P, 0, {})

#define X_DEFINE_DYNAMIC_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE)                                                   \
    static void type_name##_init(TypeName *self);                                                                                       \
    static void type_name##_class_init(TypeName##Class *klass);                                                                         \
    static void type_name##_class_finalize(TypeName##Class *klass);                                                                     \
    static xpointer type_name##_parent_class = NULL;                                                                                    \
    static XType type_name##_type_id = 0;                                                                                               \
    static xint TypeName##_private_offset;                                                                                              \
                                                                                                                                        \
    _X_DEFINE_TYPE_EXTENDED_CLASS_INIT(TypeName, type_name)                                                                             \
                                                                                                                                        \
    X_GNUC_UNUSED                                                                                                                       \
    static inline xpointer type_name##_get_instance_private(TypeName *self)                                                             \
    {                                                                                                                                   \
        return (X_STRUCT_MEMBER_P(self, TypeName##_private_offset));                                                                    \
    }                                                                                                                                   \
                                                                                                                                        \
    XType type_name##_get_type(void)                                                                                                    \
    {                                                                                                                                   \
        return type_name##_type_id;                                                                                                     \
    }                                                                                                                                   \
    static void type_name##_register_type(XTypeModule *type_module)                                                                     \
    {                                                                                                                                   \
        XType x_define_type_id X_GNUC_UNUSED;                                                                                           \
        const XTypeInfo x_define_type_info = {                                                                                          \
            sizeof (TypeName##Class),                                                                                                   \
            (XBaseInitFunc)NULL,                                                                                                        \
            (XBaseFinalizeFunc)NULL,                                                                                                    \
            (XClassInitFunc)(void (*)(void))type_name##_class_intern_init,                                                              \
            (XClassFinalizeFunc)(void (*)(void))type_name##_class_finalize,                                                             \
            NULL,                                                                                                                       \
            sizeof(TypeName),                                                                                                           \
            0,                                                                                                                          \
            (XInstanceInitFunc)(void (*)(void))type_name##_init,                                                                        \
            NULL                                                                                                                        \
        };                                                                                                                              \
        type_name##_type_id = x_type_module_register_type(type_module, TYPE_PARENT, #TypeName, &x_define_type_info, (XTypeFlags)flags); \
        x_define_type_id = type_name##_type_id;                                                                                         \
        { CODE ; }                                                                                                                      \
    }

#define X_IMPLEMENT_INTERFACE_DYNAMIC(TYPE_IFACE, iface_init)                                                                           \
    {                                                                                                                                   \
        const XInterfaceInfo x_implement_interface_info = {                                                                             \
            (XInterfaceInitFunc)(void (*)(void))iface_init, NULL, NULL                                                                  \
        };                                                                                                                              \
        x_type_module_add_interface(type_module, x_define_type_id, TYPE_IFACE, &x_implement_interface_info);                            \
    }

#define X_ADD_PRIVATE_DYNAMIC(TypeName)                                                                                                 \
    {                                                                                                                                   \
        TypeName##_private_offset = sizeof(TypeName##Private);                                                                          \
    }

XLIB_AVAILABLE_IN_ALL
XType x_type_module_get_type(void) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_type_module_use(XTypeModule *module);

XLIB_AVAILABLE_IN_ALL
void x_type_module_unuse(XTypeModule *module);

XLIB_AVAILABLE_IN_ALL
void x_type_module_set_name(XTypeModule *module, const xchar *name);

XLIB_AVAILABLE_IN_ALL
XType x_type_module_register_type(XTypeModule *module, XType parent_type, const xchar *type_name, const XTypeInfo *type_info, XTypeFlags flags);

XLIB_AVAILABLE_IN_ALL
void x_type_module_add_interface(XTypeModule *module, XType instance_type, XType interface_type, const XInterfaceInfo *interface_info);

XLIB_AVAILABLE_IN_ALL
XType x_type_module_register_enum(XTypeModule *module, const xchar *name, const XEnumValue *const_static_values);

XLIB_AVAILABLE_IN_ALL
XType x_type_module_register_flags(XTypeModule *module, const xchar *name, const XFlagsValue *const_static_values);

X_END_DECLS

#endif
