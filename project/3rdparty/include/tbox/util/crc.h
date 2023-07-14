#ifndef TBOX_UTIL_CRC_H
#define TBOX_UTIL_CRC_H

#include <cstdlib>
#include <cstdint>

namespace tbox {
namespace util {

uint16_t CalcCrc16(const void *data_ptr, size_t data_size);

}
}

#endif //TBOX_UTIL_CRC_H_20230104
