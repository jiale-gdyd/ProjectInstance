#include <linux/kconfig.h>
#include "platform.h"

#if defined(CONFIG_X86_64)
#include "x86_64/x86_64.h"
#endif

#if defined(CONFIG_AARCH32)
#include "aarch32/aarch32.h"
#endif

#if defined(CONFIG_AARCH64)
#include "aarch64/aarch64.h"
#endif

int platform_probe(int argc, char *argv[])
{
    int ret = 0;

#if defined(CONFIG_X86_64)
    ret = platform_x86_64_probe(argc, argv);
#elif defined(CONFIG_AARCH32)
    ret = platform_aarch32_probe(argc, argv);
#elif defined(CONFIG_AARCH64)
    ret = platform_aarch64_probe(argc, argv);
#endif

    return ret;
}

int platform_remove(void)
{
    int ret = 0;

#if defined(CONFIG_X86_64)
    ret = platform_x86_64_remove();
#elif defined(CONFIG_AARCH32)
    ret = platform_aarch32_remove();
#elif defined(CONFIG_AARCH64)
    ret = platform_aarch64_remove();
#endif

    return ret;
}
