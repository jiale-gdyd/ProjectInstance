#include "ax620a.h"

#include <linux/kconfig.h>
#include <mpi/axmpi/mediaBase.hpp>

#if defined(CONFIG_AX620A_EMS)
#include "FuncImpl/EMS/EMSDemo.hpp"
#elif defined(CONFIG_AX620A_EDGE_BOX)
#include "FuncImpl/EDGE/EdgeBoxDemo.hpp"
#endif

axpi::MediaBase *gApi = nullptr;

int axera_ax620a_app_probe(int argc, char *argv[])
{
    int ret = -1;

#if defined(CONFIG_AX620A_EMS)
    gApi = new ems::EMSDemo();
#elif defined(CONFIG_AX620A_EDGE_BOX)
    gApi = new edge::EdgeBoxDemo();
#endif

    if (gApi) {
        ret = gApi->init();
    }

    return ret;
}

int axera_ax620a_app_remove(void)
{
    if (gApi) {
        delete gApi;
        gApi = nullptr;
    }

    return 0;
}
