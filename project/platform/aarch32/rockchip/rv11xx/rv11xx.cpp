#include <linux/kconfig.h>

#include "rv11xx.h"
#include "private.hpp"

#if defined(CONFIG_RV11XX_EMS)
#include "FuncImpl/EMS/EMSDemo.hpp"

static media::RV1126 gIMedia;
static media::MediaBase *gApp = nullptr;
#endif

int rockchip_rv11xx_app_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_RV11XX_EMS)
    ret = gIMedia.init();
    if (ret != 0) {
        printf("Application System init failed, return:[%d]\n", ret);
        return ret;
    }

    gApp = new EMS::EMSDemoImpl(&gIMedia);
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

    gIMedia.deinit();
#endif

    return -1;
}
