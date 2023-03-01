#include <linux/kconfig.h>

#include "rv11xx.h"

#if defined(CONFIG_RV11XX_EMS)
#include "FuncImpl/EMS/EMSDemo.hpp"

static media::MediaBase *gApp = nullptr;
#endif

int rockchip_rv11xx_app_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_RV11XX_EMS)
    gApp = new EMS::EMSDemoImpl();
    if (gApp == nullptr) {
        printf("new EMS::EMSDemoImpl failed\n");
        return -1;
    }

    ret = gApp->init();
#endif

    return ret;
}

int rockchip_rv11xx_app_remove(void)
{
#if defined(CONFIG_RV11XX_EMS)
    if (gApp) {
        delete gApp;
        gApp = nullptr;
    }
#endif

    return -1;
}
