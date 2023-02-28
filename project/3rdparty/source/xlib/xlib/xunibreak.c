#include <stdlib.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xunibreak.h>

#define TPROP_PART1(Page, Char) \
    ((break_property_table_part1[Page] >= X_UNICODE_MAX_TABLE_INDEXB) \
        ? (break_property_table_part1[Page] - X_UNICODE_MAX_TABLE_INDEXB) \
        : (break_property_data[break_property_table_part1[Page]][Char]))

#define TPROP_PART2(Page, Char) \
    ((break_property_table_part2[Page] >= X_UNICODE_MAX_TABLE_INDEXB) \
        ? (break_property_table_part2[Page] - X_UNICODE_MAX_TABLE_INDEXB) \
        : (break_property_data[break_property_table_part2[Page]][Char]))

#define PROP(Char) \
    (((Char) <= X_UNICODE_LAST_CHAR_PART1) \
        ? TPROP_PART1((Char) >> 8, (Char) & 0xff) \
        : (((Char) >= 0xe0000 && (Char) <= X_UNICODE_LAST_CHAR) ? TPROP_PART2(((Char) - 0xe0000) >> 8, (Char) & 0xff) : X_UNICODE_BREAK_UNKNOWN))

XUnicodeBreakType x_unichar_break_type(xunichar c)
{
    return (XUnicodeBreakType)PROP(c);
}
