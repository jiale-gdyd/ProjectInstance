#ifndef __X_MARKUP_H__
#define __X_MARKUP_H__

#include <stdarg.h>

#include "xerror.h"
#include "xslist.h"

X_BEGIN_DECLS

typedef enum {
    X_MARKUP_ERROR_BAD_UTF8,
    X_MARKUP_ERROR_EMPTY,
    X_MARKUP_ERROR_PARSE,
    X_MARKUP_ERROR_UNKNOWN_ELEMENT,
    X_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
    X_MARKUP_ERROR_INVALID_CONTENT,
    X_MARKUP_ERROR_MISSING_ATTRIBUTE
} XMarkupError;

#define X_MARKUP_ERROR          x_markup_error_quark()

XLIB_AVAILABLE_IN_ALL
XQuark x_markup_error_quark(void);

typedef enum {
    X_MARKUP_DEFAULT_FLAGS XLIB_AVAILABLE_ENUMERATOR_IN_2_74 = 0,
    X_MARKUP_DO_NOT_USE_THIS_UNSUPPORTED_FLAG = 1 << 0,
    X_MARKUP_TREAT_CDATA_AS_TEXT              = 1 << 1,
    X_MARKUP_PREFIX_ERROR_POSITION            = 1 << 2,
    X_MARKUP_IGNORE_QUALIFIED                 = 1 << 3
} XMarkupParseFlags;

typedef struct _XMarkupParser XMarkupParser;
typedef struct _XMarkupParseContext XMarkupParseContext;

struct _XMarkupParser {
    void (*start_element)(XMarkupParseContext *context, const xchar *element_name, const xchar **attribute_names, const xchar **attribute_values, xpointer user_data, XError **error);
    void (*end_element)(XMarkupParseContext *context, const xchar *element_name, xpointer user_data, XError **error);
    void (*text)(XMarkupParseContext *context, const xchar *text, xsize text_len, xpointer user_data, XError **error);
    void (*passthrough)(XMarkupParseContext *context, const xchar *passthrough_text, xsize text_len, xpointer user_data, XError **error);
    void (*error)(XMarkupParseContext *context, XError *error, xpointer user_data);
};

XLIB_AVAILABLE_IN_ALL
XMarkupParseContext *x_markup_parse_context_new(const XMarkupParser *parser, XMarkupParseFlags flags, xpointer user_data, XDestroyNotify user_data_dnotify);

XLIB_AVAILABLE_IN_2_36
XMarkupParseContext *x_markup_parse_context_ref(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_2_36
void x_markup_parse_context_unref(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_ALL
void x_markup_parse_context_free(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_ALL
xboolean x_markup_parse_context_parse(XMarkupParseContext *context, const xchar *text, xssize text_len, XError **error);

XLIB_AVAILABLE_IN_ALL
void x_markup_parse_context_push(XMarkupParseContext *context, const XMarkupParser *parser, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xpointer x_markup_parse_context_pop(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_ALL
xboolean x_markup_parse_context_end_parse(XMarkupParseContext *context, XError **error);

XLIB_AVAILABLE_IN_ALL
const xchar *x_markup_parse_context_get_element(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_ALL
const XSList *x_markup_parse_context_get_element_stack(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_ALL
void x_markup_parse_context_get_position(XMarkupParseContext *context, xint *line_number, xint *char_number);

XLIB_AVAILABLE_IN_ALL
xpointer x_markup_parse_context_get_user_data(XMarkupParseContext *context);

XLIB_AVAILABLE_IN_ALL
xchar *x_markup_escape_text(const xchar *text, xssize length);

XLIB_AVAILABLE_IN_ALL
xchar *x_markup_printf_escaped(const char *format, ...) X_GNUC_PRINTF(1, 2);

XLIB_AVAILABLE_IN_ALL
xchar *x_markup_vprintf_escaped(const char *format, va_list args) X_GNUC_PRINTF(1, 0);

typedef enum {
    X_MARKUP_COLLECT_INVALID,
    X_MARKUP_COLLECT_STRING,
    X_MARKUP_COLLECT_STRDUP,
    X_MARKUP_COLLECT_BOOLEAN,
    X_MARKUP_COLLECT_TRISTATE,

    X_MARKUP_COLLECT_OPTIONAL = (1 << 16)
} XMarkupCollectType;

XLIB_AVAILABLE_IN_ALL
xboolean x_markup_collect_attributes(const xchar *element_name, const xchar **attribute_names, const xchar **attribute_values, XError **error, XMarkupCollectType first_type, const xchar *first_attr, ...);

X_END_DECLS

#endif
