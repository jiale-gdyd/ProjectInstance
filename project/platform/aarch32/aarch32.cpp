#include <linux/kconfig.h>
#include "aarch32.h"

#if defined(CONFIG_NXP)
#include "nxp/nxp.h"
#endif

#if defined(CONFIG_AXERA)
#include "axera/axera.h"
#endif

#if defined(CONFIG_ROCKCHIP)
#include "rockchip/rockchip.h"
#endif

int platform_aarch32_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_NXP)
    ret = nxp_aarch32_platform_probe(argc, argv);
#elif defined(CONFIG_AXERA)
    ret = axera_aarch32_platform_probe(argc, argv);
#elif defined(CONFIG_ROCKCHIP)
    ret = rockchip_aarch32_platform_probe(argc, argv);
#endif

    return ret;
}

int platform_aarch32_remove(void)
{
    int ret = -1;

#if defined(CONFIG_NXP)
    ret = nxp_aarch32_platform_remove();
#elif defined(CONFIG_AXERA)
    ret = axera_aarch32_platform_remove();
#elif defined(CONFIG_ROCKCHIP)
    ret = rockchip_aarch32_platform_remove();
#endif

    return ret;
}
