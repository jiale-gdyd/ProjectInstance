
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XClosure, x_closure_unref)
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XEnumClass, x_type_class_unref)
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XFlagsClass, x_type_class_unref)
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XObject, x_object_unref)
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XInitiallyUnowned, x_object_unref)
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XParamSpec, x_param_spec_unref)
X_DEFINE_AUTOPTR_CLEANUP_FUNC(XTypeClass, x_type_class_unref)
X_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(XValue, x_value_unset)
