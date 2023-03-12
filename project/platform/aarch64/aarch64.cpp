#include <linux/kconfig.h>
#include "aarch64.h"

#if defined(CONFIG_NVIDIA)
#include "nvidia/nvidia.h"
#endif

int platform_aarch64_probe(int argc, char *argv[])
{
#if defined(CONFIG_NVIDIA)
    return nvidia_aarch64_platform_probe(argc, argv);
#endif

    return -1;
}

int platform_aarch64_remove(void)
{
#if defined(CONFIG_NVIDIA)
    return nvidia_aarch64_platform_remove();
#endif

    return -1;
}
