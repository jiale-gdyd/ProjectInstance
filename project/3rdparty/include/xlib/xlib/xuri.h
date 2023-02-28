#ifndef __X_URI_H__
#define __X_URI_H__

#include "xtypes.h"

X_BEGIN_DECLS

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _XUri XUri;

XLIB_AVAILABLE_IN_2_66
XUri *x_uri_ref(XUri *uri);

XLIB_AVAILABLE_IN_2_66
void x_uri_unref (XUri *uri);

XLIB_AVAILABLE_TYPE_IN_2_66
typedef enum {
    X_URI_FLAGS_NONE            = 0,
    X_URI_FLAGS_PARSE_RELAXED   = 1 << 0,
    X_URI_FLAGS_HAS_PASSWORD    = 1 << 1,
    X_URI_FLAGS_HAS_AUTH_PARAMS = 1 << 2,
    X_URI_FLAGS_ENCODED         = 1 << 3,
    X_URI_FLAGS_NON_DNS         = 1 << 4,
    X_URI_FLAGS_ENCODED_QUERY   = 1 << 5,
    X_URI_FLAGS_ENCODED_PATH    = 1 << 6,
    X_URI_FLAGS_ENCODED_FRAGMENT = 1 << 7,
    X_URI_FLAGS_SCHEME_NORMALIZE XLIB_AVAILABLE_ENUMERATOR_IN_2_68 = 1 << 8,
} XUriFlags;

XLIB_AVAILABLE_IN_2_66
xboolean x_uri_split(const xchar *uri_ref, XUriFlags flags, xchar **scheme, xchar **userinfo, xchar **host, xint *port, xchar **path, xchar **query, xchar **fragment, XError **error);

XLIB_AVAILABLE_IN_2_66
xboolean x_uri_split_with_user(const xchar *uri_ref, XUriFlags flags, xchar **scheme, xchar **user, xchar **password, xchar **auth_params, xchar **host, xint *port, xchar **path, xchar **query, xchar **fragment, XError **error);

XLIB_AVAILABLE_IN_2_66
xboolean x_uri_split_network(const xchar *uri_string, XUriFlags flags, xchar **scheme, xchar **host, xint *port, XError **error);

XLIB_AVAILABLE_IN_2_66
xboolean x_uri_is_valid(const xchar *uri_string, XUriFlags flags, XError **error);

XLIB_AVAILABLE_IN_2_66
xchar *x_uri_join(XUriFlags flags, const xchar *scheme, const xchar *userinfo, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment);

XLIB_AVAILABLE_IN_2_66
xchar *x_uri_join_with_user(XUriFlags flags, const xchar *scheme, const xchar *user, const xchar *password, const xchar *auth_params, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment);

XLIB_AVAILABLE_IN_2_66
XUri *x_uri_parse(const xchar *uri_string, XUriFlags flags, XError **error);

XLIB_AVAILABLE_IN_2_66
XUri *x_uri_parse_relative(XUri *base_uri, const xchar *uri_ref, XUriFlags flags, XError **error);

XLIB_AVAILABLE_IN_2_66
xchar *x_uri_resolve_relative(const xchar *base_uri_string, const xchar *uri_ref, XUriFlags flags, XError **error);

XLIB_AVAILABLE_IN_2_66
XUri *x_uri_build(XUriFlags flags, const xchar *scheme, const xchar *userinfo, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment);

XLIB_AVAILABLE_IN_2_66
XUri *x_uri_build_with_user(XUriFlags flags, const xchar *scheme, const xchar *user, const xchar *password, const xchar *auth_params, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment);

XLIB_AVAILABLE_TYPE_IN_2_66
typedef enum {
    X_URI_HIDE_NONE        = 0,
    X_URI_HIDE_USERINFO    = 1 << 0,
    X_URI_HIDE_PASSWORD    = 1 << 1,
    X_URI_HIDE_AUTH_PARAMS = 1 << 2,
    X_URI_HIDE_QUERY       = 1 << 3,
    X_URI_HIDE_FRAGMENT    = 1 << 4,
} XUriHideFlags;

XLIB_AVAILABLE_IN_2_66
char *x_uri_to_string(XUri *uri);

XLIB_AVAILABLE_IN_2_66
char *x_uri_to_string_partial(XUri *uri, XUriHideFlags flags);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_scheme(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_userinfo(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_user(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_password(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_auth_params(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_host(XUri *uri);

XLIB_AVAILABLE_IN_2_66
xint x_uri_get_port(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_path(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_query(XUri *uri);

XLIB_AVAILABLE_IN_2_66
const xchar *x_uri_get_fragment(XUri *uri);

XLIB_AVAILABLE_IN_2_66
XUriFlags x_uri_get_flags(XUri *uri);

XLIB_AVAILABLE_TYPE_IN_2_66
typedef enum {
    X_URI_PARAMS_NONE             = 0,
    X_URI_PARAMS_CASE_INSENSITIVE = 1 << 0,
    X_URI_PARAMS_WWW_FORM         = 1 << 1,
    X_URI_PARAMS_PARSE_RELAXED    = 1 << 2,
} XUriParamsFlags;

XLIB_AVAILABLE_IN_2_66
XHashTable *x_uri_parse_params(const xchar *params, xssize length, const xchar *separators, XUriParamsFlags flags, XError **error);

typedef struct _XUriParamsIter XUriParamsIter;

struct _XUriParamsIter {
    xint     dummy0;
    xpointer dummy1;
    xpointer dummy2;
    xuint8   dummy3[256];
};

XLIB_AVAILABLE_IN_2_66
void x_uri_params_iter_init(XUriParamsIter *iter, const xchar *params, xssize length, const xchar *separators, XUriParamsFlags flags);

XLIB_AVAILABLE_IN_2_66
xboolean x_uri_params_iter_next(XUriParamsIter *iter, xchar **attribute, xchar **value, XError **error);

#define X_URI_ERROR                     (x_uri_error_quark()) XLIB_AVAILABLE_MACRO_IN_2_66

XLIB_AVAILABLE_IN_2_66
XQuark x_uri_error_quark(void);

typedef enum {
    X_URI_ERROR_FAILED,
    X_URI_ERROR_BAD_SCHEME,
    X_URI_ERROR_BAD_USER,
    X_URI_ERROR_BAD_PASSWORD,
    X_URI_ERROR_BAD_AUTH_PARAMS,
    X_URI_ERROR_BAD_HOST,
    X_URI_ERROR_BAD_PORT,
    X_URI_ERROR_BAD_PATH,
    X_URI_ERROR_BAD_QUERY,
    X_URI_ERROR_BAD_FRAGMENT,
} XUriError;

#define X_URI_RESERVED_CHARS_GENERIC_DELIMITERS             ":/?#[]@"
#define X_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS        "!$&'()*+,;="
#define X_URI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT        X_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS ":@"
#define X_URI_RESERVED_CHARS_ALLOWED_IN_PATH                X_URI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT "/"
#define X_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO            X_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS ":"

XLIB_AVAILABLE_IN_ALL
char *x_uri_unescape_string(const char *escaped_string, const char *illegal_characters);

XLIB_AVAILABLE_IN_ALL
char *x_uri_unescape_segment(const char *escaped_string, const char *escaped_string_end, const char *illegal_characters);

XLIB_AVAILABLE_IN_ALL
char *x_uri_parse_scheme(const char *uri);

XLIB_AVAILABLE_IN_2_66
const char *x_uri_peek_scheme(const char *uri);

XLIB_AVAILABLE_IN_ALL
char *x_uri_escape_string(const char *unescaped, const char *reserved_chars_allowed, xboolean allow_utf8);

XLIB_AVAILABLE_IN_2_66
XBytes *x_uri_unescape_bytes(const char *escaped_string, xssize length, const char *illegal_characters, XError **error);

XLIB_AVAILABLE_IN_2_66
char *x_uri_escape_bytes(const xuint8 *unescaped, xsize length, const char *reserved_chars_allowed);

X_GNUC_END_IGNORE_DEPRECATIONS

X_END_DECLS

#endif
