#include <linux/kconfig.h>
#include "ax620a.h"

#if defined(CONFIG_AX620A_EMS)
#include "FuncImpl/EMS/EMSDemo.hpp"

static media::MediaBase *gApp = nullptr;
#endif

int axera_ax620a_app_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_AX620A_EMS)
    gApp = new EMS::EMSDemoImpl();
    if (gApp == nullptr) {
        printf("new EMS::EMSDemoImpl failed\n");
        return -1;
    }

    ret = gApp->init();
#endif

    return ret;
}

int axera_ax620a_app_remove(void)
{
#if defined(CONFIG_AX620A_EMS)
    if (gApp) {
        delete gApp;
        gApp = nullptr;
    }
#endif

    return -1;
}
