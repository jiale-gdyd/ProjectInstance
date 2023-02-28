#ifndef __X_MARSHAL_H__
#define __X_MARSHAL_H__

#include "xclosure.h"

X_BEGIN_DECLS

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__VOID(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__VOIDv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__BOOLEAN(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__BOOLEANv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__CHAR(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__CHARv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__UCHAR(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__UCHARv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__INT(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__INTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__UINT(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__UINTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__LONG(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__LONGv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__ULONG(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__ULONGv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__ENUM(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__ENUMv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__FLAGS(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__FLAGSv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__FLOAT(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__FLOATv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__DOUBLE(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__DOUBLEv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__STRING(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__STRINGv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__PARAM(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__PARAMv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__BOXED(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__BOXEDv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__POINTER(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__POINTERv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__OBJECT(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__OBJECTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__VARIANT(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__VARIANTv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__UINT_POINTER(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_VOID__UINT_POINTERv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_BOOLEAN__FLAGS(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_BOOLEAN__FLAGSv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

#define x_cclosure_marshal_BOOL__FLAGS          x_cclosure_marshal_BOOLEAN__FLAGS

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_STRING__OBJECT_POINTER(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_STRING__OBJECT_POINTERv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_BOOLEAN__BOXED_BOXED(XClosure *closure, XValue *return_value, xuint n_param_values, const XValue *param_values, xpointer invocation_hint, xpointer marshal_data);

XLIB_AVAILABLE_IN_ALL
void x_cclosure_marshal_BOOLEAN__BOXED_BOXEDv(XClosure *closure, XValue *return_value, xpointer instance, va_list args, xpointer marshal_data, int n_params, XType *param_types);

#define x_cclosure_marshal_BOOL__BOXED_BOXED    x_cclosure_marshal_BOOLEAN__BOXED_BOXED

X_END_DECLS

#endif
