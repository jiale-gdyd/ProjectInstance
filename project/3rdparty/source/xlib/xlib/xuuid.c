#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xi18n.h>
#include <xlib/xlib/xuuid.h>
#include <xlib/xlib/xrand.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xmessages.h>
#include <xlib/xlib/xchecksum.h>

typedef struct {
    xuint8 bytes[16];
} XUuid;

static xchar *x_uuid_to_string(const XUuid *uuid)
{
    const xuint8 *bytes;

    x_return_val_if_fail(uuid != NULL, NULL);

    bytes = uuid->bytes;
    return x_strdup_printf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
}

static xboolean uuid_parse_string(const xchar *str, XUuid        *uuid)
{
    XUuid tmp;
    xint i, j, hi, lo;
    xuint expected_len = 36;
    xuint8 *bytes = tmp.bytes;

    if (strlen(str) != expected_len) {
        return FALSE;
    }

    for (i = 0, j = 0; i < 16;) {
        if (j == 8 || j == 13 || j == 18 || j == 23) {
            if (str[j++] != '-') {
                return FALSE;
            }

            continue;
        }

        hi = x_ascii_xdigit_value(str[j++]);
        lo = x_ascii_xdigit_value(str[j++]);
        if (hi == -1 || lo == -1) {
            return FALSE;
        }

        bytes[i++] = hi << 4 | lo;
    }

    if (uuid != NULL) {
        *uuid = tmp;
    }

    return TRUE;
}

xboolean x_uuid_string_is_valid(const xchar *str)
{
    x_return_val_if_fail(str != NULL, FALSE);
    return uuid_parse_string(str, NULL);
}

static void uuid_set_version(XUuid *uuid, xuint version)
{
    xuint8 *bytes = uuid->bytes;

    bytes[6] &= 0x0f;
    bytes[6] |= version << 4;
    bytes[8] &= 0x3f;
    bytes[8] |= 0x80;
}

static void x_uuid_generate_v4(XUuid *uuid)
{
    int i;
    xuint8 *bytes;
    xuint32 *ints;

    x_return_if_fail(uuid != NULL);

    bytes = uuid->bytes;
    ints = (xuint32 *) bytes;
    for (i = 0; i < 4; i++) {
        ints[i] = x_random_int();
    }

    uuid_set_version(uuid, 4);
}

xchar *x_uuid_string_random(void)
{
    XUuid uuid;

    x_uuid_generate_v4(&uuid);
    return x_uuid_to_string(&uuid);
}
