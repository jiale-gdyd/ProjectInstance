#include <linux/kconfig.h>
#include "axera.h"

#if defined(CONFIG_AX620A)
#include "ax620a/ax620a.h"
#endif

#if defined(CONFIG_AX630A)
#include "ax630a/ax630a.h"
#endif

int axera_aarch32_platform_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_AX620A)
    ret = axera_ax620a_app_probe(argc, argv);
#elif defined(CONFIG_AX630A)
    ret = axera_ax630a_app_probe(argc, argv);
#endif

    return ret;
}

int axera_aarch32_platform_remove(void)
{
    int ret = -1;

#if defined(CONFIG_AX620A)
    ret = axera_ax620a_app_remove();
#elif defined(CONFIG_AX630A)
    ret = axera_ax630a_app_remove();
#endif

    return ret;
}
