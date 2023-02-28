#include <xlib/xlib/config.h>
#include <xlib/xobj/xtypeplugin.h>

XType x_type_plugin_get_type(void)
{
    static XType type_plugin_type = 0;

    if (!type_plugin_type) {
        const XTypeInfo type_plugin_info = {
            sizeof(XTypePluginClass),
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            0,
            0,
            NULL,
            NULL,
        };

        type_plugin_type = x_type_register_static(X_TYPE_INTERFACE, x_intern_static_string("XTypePlugin"), &type_plugin_info, (XTypeFlags)0);
    }

    return type_plugin_type;
}

void x_type_plugin_use(XTypePlugin *plugin)
{
    XTypePluginClass *iface;

    x_return_if_fail(X_IS_TYPE_PLUGIN(plugin));

    iface = X_TYPE_PLUGIN_GET_CLASS(plugin);
    iface->use_plugin(plugin);
}

void x_type_plugin_unuse(XTypePlugin *plugin)
{
    XTypePluginClass *iface;

    x_return_if_fail(X_IS_TYPE_PLUGIN(plugin));

    iface = X_TYPE_PLUGIN_GET_CLASS(plugin);
    iface->unuse_plugin(plugin);
}

void x_type_plugin_complete_type_info(XTypePlugin *plugin, XType x_type, XTypeInfo *info, XTypeValueTable *value_table)
{
    XTypePluginClass *iface;

    x_return_if_fail(X_IS_TYPE_PLUGIN(plugin));
    x_return_if_fail(info != NULL);
    x_return_if_fail(value_table != NULL);

    iface = X_TYPE_PLUGIN_GET_CLASS(plugin);
    iface->complete_type_info(plugin, x_type, info, value_table);
}

void x_type_plugin_complete_interface_info(XTypePlugin *plugin, XType instance_type, XType interface_type, XInterfaceInfo *info)
{
    XTypePluginClass *iface;

    x_return_if_fail(X_IS_TYPE_PLUGIN(plugin));
    x_return_if_fail(info != NULL);

    iface = X_TYPE_PLUGIN_GET_CLASS(plugin);
    iface->complete_interface_info(plugin, instance_type, interface_type, info);
}
