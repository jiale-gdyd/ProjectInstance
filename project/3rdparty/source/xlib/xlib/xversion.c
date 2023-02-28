#include <xlib/xlib/config.h>
#include <xlib/xlib/xversion.h>

const xuint xlib_major_version = XLIB_MAJOR_VERSION;
const xuint xlib_minor_version = XLIB_MINOR_VERSION;
const xuint xlib_micro_version = XLIB_MICRO_VERSION;

const xuint xlib_binary_age = XLIB_BINARY_AGE;
const xuint xlib_interface_age = XLIB_INTERFACE_AGE;

const xchar *xlib_check_version (xuint required_major, xuint required_minor, xuint required_micro)
{
    xint required_effective_micro = 100 * required_minor + required_micro;
    xint xlib_effective_micro = 100 * XLIB_MINOR_VERSION + XLIB_MICRO_VERSION;

    if (required_major > XLIB_MAJOR_VERSION) {
        return "XLib version too old (major mismatch)";
    }

    if (required_major < XLIB_MAJOR_VERSION) {
        return "XLib version too new (major mismatch)";
    }

    if (required_effective_micro < (xlib_effective_micro - XLIB_BINARY_AGE)) {
        return "XLib version too new (micro mismatch)";
    }

    if (required_effective_micro > xlib_effective_micro) {
        return "XLib version too old (micro mismatch)";
    }

    return NULL;
}
