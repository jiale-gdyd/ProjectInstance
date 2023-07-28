#include <tbox/base/version.h>

namespace tbox {

#define TBOX_VERSION_MAJOR          1
#define TBOX_VERSION_MINOR          5
#define TBOX_VERSION_REVISION       14

void GetTboxVersion(int &major, int &minor, int &rev)
{
    major = TBOX_VERSION_MAJOR;
    minor = TBOX_VERSION_MINOR;
    rev   = TBOX_VERSION_REVISION;
}
}
