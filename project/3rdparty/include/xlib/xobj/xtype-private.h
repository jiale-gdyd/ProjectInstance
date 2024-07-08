#ifndef __X_TYPE_PRIVATE_H__
#define __X_TYPE_PRIVATE_H__

#include "xboxed.h"
#include "xobject.h"
#include "xclosure.h"

#define XOBJECT_IF_DEBUG(debug_type, code_block)

X_BEGIN_DECLS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
extern XTypeDebugFlags _x_type_debug_flags;
X_GNUC_END_IGNORE_DEPRECATIONS

typedef struct _XRealClosure XRealClosure;
struct _XRealClosure {
    XClosureMarshal   meta_marshal;
    xpointer          meta_marshal_data;
    XVaClosureMarshal va_meta_marshal;
    XVaClosureMarshal va_marshal;
    XClosure          closure;
};

#define X_REAL_CLOSURE(_c) \
    ((XRealClosure *)X_STRUCT_MEMBER_P((_c), -X_STRUCT_OFFSET(XRealClosure, closure)))

void _x_value_c_init(void);
void _x_value_types_init(void);
void _x_enum_types_init(void);
void _x_param_type_init(void);
void _x_boxed_type_init(void);
void _x_object_type_init(void);
void _x_param_spec_types_init(void);
void _x_value_transforms_init(void); 
void _x_signal_init(void);

xpointer _x_type_boxed_copy(XType type, xpointer value);

void _x_type_boxed_free(XType type, xpointer value);

void _x_type_boxed_init(XType type, XBoxedCopyFunc copy_func, XBoxedFreeFunc free_func);

xboolean _x_closure_is_void(XClosure *closure, xpointer instance);

xboolean _x_closure_supports_invoke_va(XClosure *closure);

void _x_closure_set_va_marshal(XClosure *closure, XVaClosureMarshal marshal);

void _x_closure_invoke_va(XClosure *closure, XValue *return_value, xpointer instance, va_list args, int n_params, XType *param_types);

xboolean _x_object_has_signal_handler(XObject *object);

void _x_object_set_has_signal_handler(XObject *object, xuint signal_id);

#define _X_DEFINE_TYPE_EXTENDED_WITH_PRELUDE(TN, t_n, T_P, _f_, _P_, _C_)   _X_DEFINE_TYPE_EXTENDED_BEGIN_PRE(TN, t_n) {_P_;} _X_DEFINE_TYPE_EXTENDED_BEGIN_REGISTER(TN, t_n, T_P, _f_){_C_;} _X_DEFINE_TYPE_EXTENDED_END()

X_END_DECLS

#endif
