#include <linux/kconfig.h>
#include "ax620a.h"

#if defined(CONFIG_AX620A_EMS)
#include "FuncImpl/EMS/EMSDemo.hpp"

axpi::MediaBase *gApi = nullptr;
#endif

int axera_ax620a_app_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_AX620A_EMS)
    gApi = new EMS::EMSDemo();
    if (gApi == nullptr) {
        return -1;
    }

    ret = gApi->init();
#endif

    return ret;
}

int axera_ax620a_app_remove(void)
{
#if defined(CONFIG_AX620A_EMS)
    if (gApi) {
        delete gApi;
        gApi = nullptr;
    }
#endif

    return -1;
}
