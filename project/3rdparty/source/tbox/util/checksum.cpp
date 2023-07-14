#include <tbox/util/checksum.h>

namespace tbox {
namespace util {

uint8_t CalcCheckSum8(const void *data_ptr, size_t data_size)
{
    const uint8_t *byte_ptr = (const uint8_t *)data_ptr;
    uint16_t acc = 0;

    for (size_t i = 0; i < data_size; ++i) {
        acc += byte_ptr[i];
        while ((acc & 0xff00) != 0)
            acc = (acc & 0xff) + (acc >> 8);
    }

    return ~((uint8_t)acc);
}

uint16_t CalcCheckSum16(const void *data_ptr, size_t data_size)
{
    uint32_t acc = 0;

    if (data_size > 0) {
        uint8_t *bytes = (uint8_t*)data_ptr;

        while (data_size > 1) {
            acc += (bytes[0] << 8 | bytes[1]);
            while ((acc & 0xffff0000UL) != 0)
                acc = (acc >> 16) + (acc & 0x0000ffffUL);

            bytes += 2;
            data_size -= 2;
        }

        if (data_size > 0) {
            acc += (bytes[0] << 8);
            while ((acc & 0xffff0000UL) != 0)
                acc = (acc >> 16) + (acc & 0x0000ffffUL);
        }
    }

    uint16_t ret = (uint16_t)acc;
    return ~ret;
}

}
}
