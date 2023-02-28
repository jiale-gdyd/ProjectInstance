#include <stdlib.h>
#include <xlib/xlib/config.h>
#include <xlib/xobj/xtypeplugin.h>
#include <xlib/xobj/xtypemodule.h>

typedef struct _ModuleTypeInfo ModuleTypeInfo;
typedef struct _ModuleInterfaceInfo ModuleInterfaceInfo;

struct _ModuleTypeInfo  {
    xboolean  loaded;
    XType     type;
    XType     parent_type;
    XTypeInfo info;
};

struct _ModuleInterfaceInfo  {
    xboolean       loaded;
    XType          instance_type;
    XType          interface_type;
    XInterfaceInfo info;
};

static xpointer parent_class = NULL;

static void x_type_module_use_plugin(XTypePlugin *plugin);
static void x_type_module_complete_type_info(XTypePlugin *plugin, XType x_type, XTypeInfo *info, XTypeValueTable *value_table);
static void x_type_module_complete_interface_info(XTypePlugin *plugin, XType instance_type, XType interface_type, XInterfaceInfo *info);

static void x_type_module_dispose(XObject *object)
{
    XTypeModule *module = X_TYPE_MODULE(object);

    if (module->type_infos || module->interface_infos) {
        x_critical(X_STRLOC ": unsolicitated invocation of x_object_run_dispose() on XTypeModule");
        x_object_ref(object);
    }

    X_OBJECT_CLASS(parent_class)->dispose(object);
}

static void x_type_module_finalize(XObject *object)
{
    XTypeModule *module = X_TYPE_MODULE(object);

    x_free(module->name);
    X_OBJECT_CLASS(parent_class)->finalize(object);
}

static void x_type_module_class_init(XTypeModuleClass *classt)
{
    XObjectClass *gobject_class = X_OBJECT_CLASS(classt);

    parent_class = X_OBJECT_CLASS(x_type_class_peek_parent(classt));
    gobject_class->dispose = x_type_module_dispose;
    gobject_class->finalize = x_type_module_finalize;
}

static void x_type_module_iface_init(XTypePluginClass *iface)
{
    iface->use_plugin = x_type_module_use_plugin;
    iface->unuse_plugin = (void (*)(XTypePlugin *))x_type_module_unuse;
    iface->complete_type_info = x_type_module_complete_type_info;
    iface->complete_interface_info = x_type_module_complete_interface_info;
}

XType x_type_module_get_type(void)
{
    static XType type_module_type = 0;

    if (!type_module_type) {
        const XTypeInfo type_module_info = {
            sizeof(XTypeModuleClass),
            NULL,
            NULL,
            (XClassInitFunc)x_type_module_class_init,
            NULL,
            NULL,
            sizeof (XTypeModule),
            0,
            NULL,
            NULL,
        };

        const XInterfaceInfo iface_info = {
            (XInterfaceInitFunc)x_type_module_iface_init,
            NULL,
            NULL,
        };

        type_module_type = x_type_register_static(X_TYPE_OBJECT, x_intern_static_string("XTypeModule"), &type_module_info, X_TYPE_FLAG_ABSTRACT);
        x_type_add_interface_static(type_module_type, X_TYPE_TYPE_PLUGIN, &iface_info);
    }

    return type_module_type;
}

void x_type_module_set_name(XTypeModule *module, const xchar  *name)
{
    x_return_if_fail(X_IS_TYPE_MODULE(module));

    x_free(module->name);
    module->name = x_strdup(name);
}

static ModuleTypeInfo *x_type_module_find_type_info(XTypeModule *module, XType type)
{
    XSList *tmp_list = module->type_infos;
    while (tmp_list) {
        ModuleTypeInfo *type_info = (ModuleTypeInfo *)tmp_list->data;
        if (type_info->type == type) {
            return type_info;
        }

        tmp_list = tmp_list->next;
    }

    return NULL;
}

static ModuleInterfaceInfo *x_type_module_find_interface_info(XTypeModule *module, XType instance_type, XType interface_type)
{
    XSList *tmp_list = module->interface_infos;
    while (tmp_list) {
        ModuleInterfaceInfo *interface_info = (ModuleInterfaceInfo *)tmp_list->data;
        if (interface_info->instance_type == instance_type && interface_info->interface_type == interface_type) {
            return interface_info;
        }

        tmp_list = tmp_list->next;
    }

    return NULL;
}

xboolean x_type_module_use(XTypeModule *module)
{
    x_return_val_if_fail(X_IS_TYPE_MODULE(module), FALSE);

    module->use_count++;
    if (module->use_count == 1) {
        XSList *tmp_list;

        if (!X_TYPE_MODULE_GET_CLASS(module)->load(module)) {
            module->use_count--;
            return FALSE;
        }

        tmp_list = module->type_infos;
        while (tmp_list) {
            ModuleTypeInfo *type_info = (ModuleTypeInfo *)tmp_list->data;
            if (!type_info->loaded) {
                x_critical("plugin '%s' failed to register type '%s'", module->name ? module->name : "(unknown)", x_type_name(type_info->type));
                module->use_count--;
                return FALSE;
            }

            tmp_list = tmp_list->next;
        }
    }

    return TRUE;
}

void x_type_module_unuse(XTypeModule *module)
{
    x_return_if_fail(X_IS_TYPE_MODULE(module));
    x_return_if_fail(module->use_count > 0);

    module->use_count--;

    if (module->use_count == 0) {
        XSList *tmp_list;

        X_TYPE_MODULE_GET_CLASS(module)->unload(module);

        tmp_list = module->type_infos;
        while (tmp_list) {
            ModuleTypeInfo *type_info = (ModuleTypeInfo *)tmp_list->data;
            type_info->loaded = FALSE;
            tmp_list = tmp_list->next;
        }
    }
}

static void x_type_module_use_plugin(XTypePlugin *plugin)
{
    XTypeModule *module = X_TYPE_MODULE(plugin);

    if (!x_type_module_use(module)) {
        x_error("Fatal error - Could not reload previously loaded plugin '%s'", module->name ? module->name : "(unknown)");
    }
}

static void x_type_module_complete_type_info(XTypePlugin *plugin, XType x_type, XTypeInfo *info, XTypeValueTable *value_table)
{
    XTypeModule *module = X_TYPE_MODULE(plugin);
    ModuleTypeInfo *module_type_info = x_type_module_find_type_info(module, x_type);

    *info = module_type_info->info;
    if (module_type_info->info.value_table) {
        *value_table = *module_type_info->info.value_table;
    }
}

static void x_type_module_complete_interface_info(XTypePlugin *plugin, XType instance_type, XType interface_type, XInterfaceInfo *info)
{
    XTypeModule *module = X_TYPE_MODULE(plugin);
    ModuleInterfaceInfo *module_interface_info = x_type_module_find_interface_info(module, instance_type, interface_type);

    *info = module_interface_info->info;
}

XType x_type_module_register_type(XTypeModule *module, XType parent_type, const xchar *type_name, const XTypeInfo *type_info, XTypeFlags flags)
{
    XType type;
    ModuleTypeInfo *module_type_info = NULL;

    x_return_val_if_fail(type_name != NULL, 0);
    x_return_val_if_fail(type_info != NULL, 0);

    if (module == NULL) {
        return x_type_register_static_simple(parent_type, type_name, type_info->class_size, type_info->class_init, type_info->instance_size, type_info->instance_init, flags);
    }

    type = x_type_from_name(type_name);
    if (type) {
        XTypePlugin *old_plugin = x_type_get_plugin(type);

        if (old_plugin != X_TYPE_PLUGIN(module)) {
            x_critical("Two different plugins tried to register '%s'.", type_name);
            return 0;
        }
    }

    if (type) {
        module_type_info = x_type_module_find_type_info(module, type);

        if (module_type_info->parent_type != parent_type) {
            const xchar *parent_type_name = x_type_name(parent_type);
      
            x_critical("Type '%s' recreated with different parent type.(was '%s', now '%s')", type_name, x_type_name(module_type_info->parent_type), parent_type_name ? parent_type_name : "(unknown)");
            return 0;
        }

        if (module_type_info->info.value_table) {
            x_free((XTypeValueTable *)module_type_info->info.value_table);
        }
    } else {
        module_type_info = x_new(ModuleTypeInfo, 1);

        module_type_info->parent_type = parent_type;
        module_type_info->type = x_type_register_dynamic(parent_type, type_name, X_TYPE_PLUGIN(module), flags);
        module->type_infos = x_slist_prepend(module->type_infos, module_type_info);
    }

    module_type_info->loaded = TRUE;
    module_type_info->info = *type_info;
    if (type_info->value_table) {
        module_type_info->info.value_table = (const XTypeValueTable *)x_memdup2(type_info->value_table, sizeof(XTypeValueTable));
    }

    return module_type_info->type;
}

void x_type_module_add_interface(XTypeModule *module, XType instance_type, XType interface_type, const XInterfaceInfo *interface_info)
{
    ModuleInterfaceInfo *module_interface_info = NULL;

    x_return_if_fail(interface_info != NULL);

    if (module == NULL) {
        x_type_add_interface_static(instance_type, interface_type, interface_info);
        return;
    }

    if (x_type_is_a(instance_type, interface_type)) {
        XTypePlugin *old_plugin = x_type_interface_get_plugin(instance_type, interface_type);

        if (!old_plugin) {
            x_critical("Interface '%s' for '%s' was previously registered statically or for a parent type.", x_type_name(interface_type), x_type_name(instance_type));
            return;
        } else if (old_plugin != X_TYPE_PLUGIN(module)) {
            x_critical("Two different plugins tried to register interface '%s' for '%s'.", x_type_name(interface_type), x_type_name(instance_type));
            return;
        }

        module_interface_info = x_type_module_find_interface_info(module, instance_type, interface_type);
        x_assert(module_interface_info);
    } else {
        module_interface_info = x_new(ModuleInterfaceInfo, 1);

        module_interface_info->instance_type = instance_type;
        module_interface_info->interface_type = interface_type;

        x_type_add_interface_dynamic(instance_type, interface_type, X_TYPE_PLUGIN(module));
        module->interface_infos = x_slist_prepend(module->interface_infos, module_interface_info);
    }

    module_interface_info->loaded = TRUE;
    module_interface_info->info = *interface_info;
}

XType x_type_module_register_enum(XTypeModule *module, const xchar *name, const XEnumValue *const_static_values)
{
    XTypeInfo enum_type_info = { 0, };

    x_return_val_if_fail(module == NULL || X_IS_TYPE_MODULE(module), 0);
    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(const_static_values != NULL, 0);

    x_enum_complete_type_info(X_TYPE_ENUM, &enum_type_info, const_static_values);

    return x_type_module_register_type(X_TYPE_MODULE(module), X_TYPE_ENUM, name, &enum_type_info, (XTypeFlags)0);
}

XType x_type_module_register_flags(XTypeModule *module, const xchar *name, const XFlagsValue *const_static_values)
{
    XTypeInfo flags_type_info = { 0, };

    x_return_val_if_fail(module == NULL || X_IS_TYPE_MODULE(module), 0);
    x_return_val_if_fail(name != NULL, 0);
    x_return_val_if_fail(const_static_values != NULL, 0);

    x_flags_complete_type_info(X_TYPE_FLAGS, &flags_type_info, const_static_values);

    return x_type_module_register_type(X_TYPE_MODULE(module), X_TYPE_FLAGS, name, &flags_type_info, (XTypeFlags)0);
}
