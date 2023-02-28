#include <linux/kconfig.h>
#include "rockchip.h"

#if defined(CONFIG_RV11XX)
#include "rv11xx/rv11xx.h"
#endif

int rockchip_aarch32_platform_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_RV11XX)
    ret = rockchip_rv11xx_app_probe(argc, argv);
#endif

    return ret;
}

int rockchip_aarch32_platform_remove(void)
{
    int ret = -1;

#if defined(CONFIG_RV11XX)
    ret = rockchip_rv11xx_app_remove();
#endif

    return ret;
}
