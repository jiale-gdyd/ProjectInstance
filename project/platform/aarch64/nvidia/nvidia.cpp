#include <linux/kconfig.h>
#include "nvidia.h"

#if defined(CONFIG_JETSON)
#include "jetson/jetson.h"
#endif

int nvidia_aarch64_platform_probe(int argc, char *argv[])
{
#if defined(CONFIG_JETSON)
    return nvidia_jetson_platform_probe(argc, argv);
#endif

    return -1;
}

int nvidia_aarch64_platform_remove(void)
{
#if defined(CONFIG_JETSON)
    return nvidia_jetson_platform_remove();
#endif

    return -1;
}
