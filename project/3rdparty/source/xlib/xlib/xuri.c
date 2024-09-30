#include <stdlib.h>
#include <string.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib.h>
#include <xlib/xlib/xlibintl.h>
#include <xlib/xlib/xuriprivate.h>
#include <xlib/xlib/xlib-private.h>

struct _XUri {
    xchar     *scheme;
    xchar     *userinfo;
    xchar     *host;
    xint      port;
    xchar     *path;
    xchar     *query;
    xchar     *fragment;
    xchar     *user;
    xchar     *password;
    xchar     *auth_params;
    XUriFlags flags;
};

XUri *x_uri_ref(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return x_atomic_rc_box_acquire(uri);
}

static void x_uri_clear(XUri *uri)
{
    x_free(uri->scheme);
    x_free(uri->userinfo);
    x_free(uri->host);
    x_free(uri->path);
    x_free(uri->query);
    x_free(uri->fragment);
    x_free(uri->user);
    x_free(uri->password);
    x_free(uri->auth_params);
}

void x_uri_unref(XUri *uri)
{
    x_return_if_fail(uri != NULL);
    x_atomic_rc_box_release_full(uri, (XDestroyNotify)x_uri_clear);
}

static xboolean x_uri_char_is_unreserved(xchar ch)
{
    if (x_ascii_isalnum(ch)) {
        return TRUE;
    }

    return ch == '-' || ch == '.' || ch == '_' || ch == '~';
}

#define XDIGIT(c)           ((c) <= '9' ? (c) - '0' : ((c) & 0x4F) - 'A' + 10)
#define HEXCHAR(s)          ((XDIGIT(s[1]) << 4) + XDIGIT(s[2]))

static xssize uri_decoder(xchar **out, const xchar *illegal_chars, const xchar *start, xsize length, xboolean just_normalize, xboolean www_form, XUriFlags flags, XUriError parse_error, XError **error)
{
    xchar c;
    xssize len;
    XString *decoded;
    const xchar *invalid, *s, *end;

    if (!(flags & X_URI_FLAGS_ENCODED)) {
        just_normalize = FALSE;
    }

    decoded = x_string_sized_new(length + 1);
    for (s = start, end = s + length; s < end; s++) {
        if (*s == '%') {
            if (s + 2 >= end || !x_ascii_isxdigit(s[1]) || !x_ascii_isxdigit(s[2])) {
                if (!(flags & X_URI_FLAGS_PARSE_RELAXED)) {
                    x_set_error_literal(error, X_URI_ERROR, parse_error, _("Invalid %-encoding in URI"));
                    x_string_free(decoded, TRUE);
                    return -1;
                }

                x_string_append_c(decoded, *s);
                continue;
            }

            c = HEXCHAR(s);
            if (illegal_chars && strchr(illegal_chars, c)) {
                x_set_error_literal(error, X_URI_ERROR, parse_error, _("Illegal character in URI"));
                x_string_free(decoded, TRUE);
                return -1;
            }

            if (just_normalize && !x_uri_char_is_unreserved(c)) {
                x_string_append_c(decoded, *s);
                x_string_append_c(decoded, x_ascii_toupper(s[1]));
                x_string_append_c(decoded, x_ascii_toupper(s[2]));
                s += 2;
            } else {
                x_string_append_c(decoded, c);
                s += 2;
            }
        } else if (www_form && *s == '+') {
            x_string_append_c(decoded, ' ');
        } else if (just_normalize && (!x_ascii_isgraph(*s))) {
            x_string_append_printf(decoded, "%%%02X", (xuchar)*s);
        } else {
            x_string_append_c(decoded, *s);
        }
    }

    len = decoded->len;
    x_assert(len >= 0);

    if (!(flags & X_URI_FLAGS_ENCODED) && !x_utf8_validate(decoded->str, len, &invalid)) {
        x_set_error_literal(error, X_URI_ERROR, parse_error, _("Non-UTF-8 characters in URI"));
        x_string_free(decoded, TRUE);
        return -1;
    }

    if (out) {
        *out = x_string_free(decoded, FALSE);
    } else {
        x_string_free(decoded, TRUE);
    }

    return len;
}

static xboolean uri_decode(xchar **out, const xchar *illegal_chars, const xchar *start, xsize length, xboolean www_form, XUriFlags flags, XUriError parse_error, XError **error)
{
    return uri_decoder(out, illegal_chars, start, length, FALSE, www_form, flags, parse_error, error) != -1;
}

static xboolean uri_normalize(xchar **out, const xchar *start, xsize length, XUriFlags flags, XUriError parse_error, XError **error)
{
    return uri_decoder(out, NULL, start, length, TRUE, FALSE, flags, parse_error, error) != -1;
}

static xboolean is_valid(xuchar c, const xchar *reserved_chars_allowed)
{
    if (x_uri_char_is_unreserved(c)) {
        return TRUE;
    }

    if (reserved_chars_allowed && strchr(reserved_chars_allowed, c)) {
        return TRUE;
    }

    return FALSE;
}

void _uri_encoder(XString *out, const xuchar *start, xsize length, const xchar *reserved_chars_allowed, xboolean allow_utf8)
{
    const xuchar *p = start;
    const xuchar *end = p + length;
    static const xchar hex[] = "0123456789ABCDEF";

    while (p < end) {
        xunichar multibyte_utf8_char = 0;

        if (allow_utf8 && *p >= 0x80) {
            multibyte_utf8_char = x_utf8_get_char_validated((xchar *)p, end - p);
        }

        if (multibyte_utf8_char > 0 && multibyte_utf8_char != (xunichar) -1 && multibyte_utf8_char != (xunichar) -2) {
            xint len = x_utf8_skip[*p];
            x_string_append_len(out, (xchar *)p, len);
            p += len;
        } else if (is_valid(*p, reserved_chars_allowed)) {
            x_string_append_c(out, *p);
            p++;
        } else {
            x_string_append_c(out, '%');
            x_string_append_c(out, hex[*p >> 4]);
            x_string_append_c(out, hex[*p & 0xf]);
            p++;
        }
    }
}

static xboolean parse_ip_literal(const xchar *start, xsize length, XUriFlags flags, xchar **out, XError **error)
{
    xchar *addr = NULL;
    xsize addr_length = 0;
    xsize zone_id_length = 0;
    xchar *pct, *zone_id = NULL;
    xchar *decoded_zone_id = NULL;

    if (start[length - 1] != ']') {
        goto bad_ipv6_literal;
    }

    addr = x_strndup(start + 1, length - 2);
    addr_length = length - 2;

    pct = strchr(addr, '%');
    if (pct != NULL) {
        *pct = '\0';

        if (addr_length - (pct - addr) >= 4 && *(pct + 1) == '2' && *(pct + 2) == '5') {
            zone_id = pct + 3;
            zone_id_length = addr_length - (zone_id - addr);
        } else if (flags & X_URI_FLAGS_PARSE_RELAXED && addr_length - (pct - addr) >= 2) {
            zone_id = pct + 1;
            zone_id_length = addr_length - (zone_id - addr);
        } else {
            goto bad_ipv6_literal;
        }

        x_assert(zone_id_length >= 1);
    }

    if (!x_hostname_is_ip_address(addr) || !strchr(addr, ':')) {
        goto bad_ipv6_literal;
    }

    if (zone_id != NULL && !uri_decode(&decoded_zone_id, NULL, zone_id, zone_id_length, FALSE, flags, X_URI_ERROR_BAD_HOST, NULL)) {
        goto bad_ipv6_literal;
    }

    if (out != NULL && decoded_zone_id != NULL) {
        *out = x_strconcat(addr, "%", decoded_zone_id, NULL);
    } else if (out != NULL) {
        *out = x_steal_pointer(&addr);
    }

    x_free(addr);
    x_free(decoded_zone_id);
    return TRUE;

bad_ipv6_literal:
    x_free(addr);
    x_free(decoded_zone_id);
    x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_HOST, _("Invalid IPv6 address ‘%.*s’ in URI"), (xint)length, start);

    return FALSE;
}

static xboolean parse_host(const xchar *start, xsize length, XUriFlags flags, xchar **out, XError **error)
{
    xchar *addr = NULL;
    xchar *decoded = NULL, *host;

    if (*start == '[') {
        if (!parse_ip_literal(start, length, flags, &host, error)) {
            return FALSE;
        }

        goto ok;
    }

    if (x_ascii_isdigit(*start)) {
        addr = x_strndup(start, length);
        if (x_hostname_is_ip_address(addr)) {
            host = addr;
            goto ok;
        }

        x_free(addr);
    }

    if (flags & X_URI_FLAGS_NON_DNS) {
        if (!uri_normalize(&decoded, start, length, flags, X_URI_ERROR_BAD_HOST, error)) {
            return FALSE;
        }

        host = x_steal_pointer(&decoded);
        goto ok;
    }

    flags = (XUriFlags)(flags & ~X_URI_FLAGS_ENCODED);
    if (!uri_decode(&decoded, NULL, start, length, FALSE, flags, X_URI_ERROR_BAD_HOST, error)) {
        return FALSE;
    }

    if (x_hostname_is_ip_address(decoded)) {
        x_free(decoded);
        x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_HOST, _("Illegal encoded IP address ‘%.*s’ in URI"), (xint)length, start);
        return FALSE;
    }

    if (x_hostname_is_non_ascii(decoded)) {
        host = x_hostname_to_ascii(decoded);
        if (host == NULL) {
            x_free(decoded);
            x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_HOST, _("Illegal internationalized hostname ‘%.*s’ in URI"), (xint) length, start);
            return FALSE;
        }
    } else {
        host = x_steal_pointer(&decoded);
    }

ok:
    if (out) {
        *out = x_steal_pointer(&host);
    }
    x_free(host);
    x_free(decoded);

    return TRUE;
}

static xboolean parse_port(const xchar *start, xsize length, xint *out, XError **error)
{
    xchar *end;
    xulong parsed_port;

    if (!x_ascii_isdigit(*start)) {
        x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_PORT, _("Could not parse port ‘%.*s’ in URI"), (xint)length, start);
        return FALSE;
    }

    parsed_port = strtoul(start, &end, 10);
    if (end != start + length) {
        x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_PORT, _("Could not parse port ‘%.*s’ in URI"), (xint)length, start);
        return FALSE;
    } else if (parsed_port > 65535) {
        x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_PORT, _("Port ‘%.*s’ in URI is out of range"), (xint)length, start);
        return FALSE;
    }

    if (out) {
        *out = parsed_port;
    }

    return TRUE;
}

static xboolean parse_userinfo(const xchar *start, xsize length, XUriFlags flags, xchar **user, xchar **password, xchar **auth_params, XError **error)
{
    const xchar *user_end = NULL, *password_end = NULL, *auth_params_end;

    auth_params_end = start + length;
    if (flags & X_URI_FLAGS_HAS_AUTH_PARAMS) {
        password_end = (const xchar *)memchr(start, ';', auth_params_end - start);
    }

    if (!password_end) {
        password_end = auth_params_end;
    }

    if (flags & X_URI_FLAGS_HAS_PASSWORD) {
        user_end = (const xchar *)memchr(start, ':', password_end - start);
    }

    if (!user_end) {
        user_end = password_end;
    }

    if (!uri_normalize(user, start, user_end - start, flags, X_URI_ERROR_BAD_USER, error)) {
        return FALSE;
    }

    if (*user_end == ':') {
        start = user_end + 1;
        if (!uri_normalize(password, start, password_end - start, flags, X_URI_ERROR_BAD_PASSWORD, error)) {
            if (user) {
                x_clear_pointer(user, x_free);
            }

            return FALSE;
        }
    } else if (password) {
        *password = NULL;
    }

    if (*password_end == ';') {
        start = password_end + 1;
        if (!uri_normalize(auth_params, start, auth_params_end - start, flags, X_URI_ERROR_BAD_AUTH_PARAMS, error)) {
            if (user) {
                x_clear_pointer(user, x_free);
            }

            if (password) {
                x_clear_pointer(password, x_free);
            }

            return FALSE;
        }
    } else if (auth_params) {
        *auth_params = NULL;
    }

    return TRUE;
}

static xchar *uri_cleanup(const xchar *uri_string)
{
    XString *copy;
    const xchar *end;

    while (x_ascii_isspace(*uri_string)) {
        uri_string++;
    }

    end = uri_string + strlen(uri_string);
    while (end > uri_string && x_ascii_isspace(*(end - 1))) {
        end--;
    }

    copy = x_string_sized_new(end - uri_string);
    while (uri_string < end) {
        if (*uri_string == ' ') {
            x_string_append(copy, "%20");
        } else if (x_ascii_isspace(*uri_string)) {
            ;
        } else {
            x_string_append_c(copy, *uri_string);
        }

        uri_string++;
    }

    return x_string_free(copy, FALSE);
}

static xboolean should_normalize_empty_path(const char *scheme)
{
    xsize i;
    const char *const schemes[] = { "https", "http", "wss", "ws" };

    for (i = 0; i < X_N_ELEMENTS(schemes); ++i) {
        if (!strcmp(schemes[i], scheme)) {
            return TRUE;
        }
    }

    return FALSE;
}

static int normalize_port(const char *scheme, int port)
{
    int i;
    const char *default_schemes[3] = { NULL };

    switch (port) {
        case 21:
            default_schemes[0] = "ftp";
            break;

        case 80:
            default_schemes[0] = "http";
            default_schemes[1] = "ws";
            break;

        case 443:
            default_schemes[0] = "https";
            default_schemes[1] = "wss";
            break;

        default:
            break;
    }

    for (i = 0; default_schemes[i]; ++i) {
        if (!strcmp(scheme, default_schemes[i])) {
            return -1;
        }
    }

    return port;
}

int x_uri_get_default_scheme_port(const char *scheme)
{
    if (strcmp(scheme, "http") == 0 || strcmp(scheme, "ws") == 0) {
        return 80;
    }

    if (strcmp(scheme, "https") == 0 || strcmp(scheme, "wss") == 0) {
        return 443;
    }

    if (strcmp(scheme, "ftp") == 0) {
        return 21;
    }

    if (strstr(scheme, "socks") == scheme) {
        return 1080;
    }

    return -1;
}

static xboolean x_uri_split_internal(const xchar *uri_string, XUriFlags flags, xchar **scheme, xchar **userinfo, xchar **user, xchar **password, xchar **auth_params, xchar **host, xint *port, xchar **path, xchar **query, xchar **fragment, XError **error)
{
    xchar *normalized_scheme = NULL;
    xchar *cleaned_uri_string = NULL;
    const xchar *p, *bracket, *hostend;
    const xchar *end, *colon, *at, *path_start, *semi, *question;

    if (scheme) {
        *scheme = NULL;
    }

    if (userinfo) {
        *userinfo = NULL;
    }

    if (user) {
        *user = NULL;
    }

    if (password) {
        *password = NULL;
    }

    if (auth_params) {
        *auth_params = NULL;
    }

    if (host) {
        *host = NULL;
    }

    if (port) {
        *port = -1;
    }

    if (path) {
        *path = NULL;
    }

    if (query) {
        *query = NULL;
    }

    if (fragment) {
        *fragment = NULL;
    }

    if ((flags & X_URI_FLAGS_PARSE_RELAXED) && strpbrk(uri_string, " \t\n\r")) {
        cleaned_uri_string = uri_cleanup(uri_string);
        uri_string = cleaned_uri_string;
    }

    p = uri_string;
    while (*p && (x_ascii_isalpha(*p) || (p > uri_string && (x_ascii_isdigit(*p) || *p == '.' || *p == '+' || *p == '-')))) {
        p++;
    }

    if (p > uri_string && *p == ':') {
        normalized_scheme = x_ascii_strdown(uri_string, p - uri_string);
        if (scheme) {
            *scheme = x_steal_pointer(&normalized_scheme);
        }
        p++;
    } else {
        if (scheme) {
            *scheme = NULL;
        }
        p = uri_string;
    }

    if (strncmp(p, "//", 2) == 0) {
        p += 2;

        path_start = p + strcspn(p, "/?#");
        at = (const xchar *)memchr(p, '@', path_start - p);
        if (at) {
            if (flags & X_URI_FLAGS_PARSE_RELAXED) {
                xchar *next_at;

                do {
                    next_at = (xchar *)memchr(at + 1, '@', path_start - (at + 1));
                    if (next_at) {
                        at = next_at;
                    }
                } while (next_at);
            }

            if (user || password || auth_params || (flags & (X_URI_FLAGS_HAS_PASSWORD|X_URI_FLAGS_HAS_AUTH_PARAMS))) {
                if (!parse_userinfo(p, at - p, flags, user, password, auth_params, error)) {
                    goto fail;
                }
            }

            if (!uri_normalize(userinfo, p, at - p, flags, X_URI_ERROR_BAD_USER, error)) {
                goto fail;
            }

            p = at + 1;
        }

        if (flags & X_URI_FLAGS_PARSE_RELAXED) {
            semi = strchr(p, ';');
            if (semi && semi < path_start) {
                path_start = semi;
            }
        }

        if (*p == '[') {
            bracket = (const xchar *)memchr(p, ']', path_start - p);
            if (bracket && *(bracket + 1) == ':') {
                colon = bracket + 1;
            } else {
                colon = NULL;
            }
        } else {
            colon = (const xchar *)memchr(p, ':', path_start - p);
        }

        hostend = colon ? colon : path_start;
        if (!parse_host(p, hostend - p, flags, host, error)) {
            goto fail;
        }

        if (colon && colon != path_start - 1) {
            p = colon + 1;
            if (!parse_port(p, path_start - p, port, error)) {
                goto fail;
            }
        }

        p = path_start;
    }

    end = p + strcspn(p, "#");
    if (*end == '#') {
        if (!uri_normalize(fragment, end + 1, strlen(end + 1), (XUriFlags)(flags | (flags & X_URI_FLAGS_ENCODED_FRAGMENT ? X_URI_FLAGS_ENCODED : 0)), X_URI_ERROR_BAD_FRAGMENT, error)) {
            goto fail;
        }
    }

    question = (const xchar *)memchr(p, '?', end - p);
    if (question) {
        if (!uri_normalize(query, question + 1, end - (question + 1), (XUriFlags)(flags | (flags & X_URI_FLAGS_ENCODED_QUERY ? X_URI_FLAGS_ENCODED : 0)), X_URI_ERROR_BAD_QUERY, error)) {
            goto fail;
        }

        end = question;
    }

    if (!uri_normalize(path, p, end - p, (XUriFlags)(flags | (flags & X_URI_FLAGS_ENCODED_PATH ? X_URI_FLAGS_ENCODED : 0)), X_URI_ERROR_BAD_PATH, error)) {
        goto fail;
    }

    if (flags & X_URI_FLAGS_SCHEME_NORMALIZE && ((scheme && *scheme) || normalized_scheme)) {
        const char *scheme_str = scheme && *scheme ? *scheme : normalized_scheme;

        if (should_normalize_empty_path(scheme_str) && path && !**path) {
            x_free(*path);
            *path = x_strdup("/");
        }

        if (port && *port == -1) {
            *port = x_uri_get_default_scheme_port(scheme_str);
        }
    }

    x_free(normalized_scheme);
    x_free(cleaned_uri_string);
    return TRUE;

fail:
    if (scheme) {
        x_clear_pointer(scheme, x_free);
    }

    if (userinfo) {
        x_clear_pointer(userinfo, x_free);
    }

    if (host) {
        x_clear_pointer(host, x_free);
    }

    if (port) {
        *port = -1;
    }

    if (path) {
        x_clear_pointer(path, x_free);
    }

    if (query) {
        x_clear_pointer(query, x_free);
    }

    if (fragment) {
        x_clear_pointer(fragment, x_free);
    }

    x_free(normalized_scheme);
    x_free(cleaned_uri_string);
    return FALSE;
}

xboolean x_uri_split(const xchar *uri_ref, XUriFlags flags, xchar **scheme, xchar **userinfo, xchar **host, xint *port, xchar **path, xchar **query, xchar **fragment, XError **error)
{
    x_return_val_if_fail(uri_ref != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    return x_uri_split_internal(uri_ref, flags, scheme, userinfo, NULL, NULL, NULL, host, port, path, query, fragment, error);
}

xboolean x_uri_split_with_user(const xchar *uri_ref, XUriFlags flags, xchar **scheme, xchar **user, xchar **password, xchar **auth_params, xchar **host, xint *port, xchar **path, xchar **query, xchar **fragment, XError **error)
{
    x_return_val_if_fail(uri_ref != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    return x_uri_split_internal(uri_ref, flags, scheme, NULL, user, password, auth_params, host, port, path, query, fragment, error);
}

xboolean x_uri_split_network(const xchar *uri_string, XUriFlags flags, xchar **scheme, xchar **host, xint *port, XError **error)
{
    xchar *my_scheme = NULL, *my_host = NULL;

    x_return_val_if_fail(uri_string != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!x_uri_split_internal(uri_string, flags, &my_scheme, NULL, NULL, NULL, NULL, &my_host, port, NULL, NULL, NULL, error)) {
        return FALSE;
    }

    if (!my_scheme || !my_host) {
        if (!my_scheme) {
            x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_SCHEME, _("URI ‘%s’ is not an absolute URI"), uri_string);
        } else {
                x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_HOST, _("URI ‘%s’ has no host component"), uri_string);
        }

        x_free(my_scheme);
        x_free(my_host);

        return FALSE;
    }

    if (scheme) {
        *scheme = x_steal_pointer(&my_scheme);
    }

    if (host) {
        *host = x_steal_pointer(&my_host);
    }

    x_free(my_scheme);
    x_free(my_host);

    return TRUE;
}

xboolean x_uri_is_valid(const xchar *uri_string, XUriFlags flags, XError **error)
{
    xchar *my_scheme = NULL;

    x_return_val_if_fail(uri_string != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!x_uri_split_internal (uri_string, flags, &my_scheme, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, error)) {
        return FALSE;
    }

    if (!my_scheme) {
        x_set_error(error, X_URI_ERROR, X_URI_ERROR_BAD_SCHEME, _("URI ‘%s’ is not an absolute URI"), uri_string);
        return FALSE;
    }

    x_free(my_scheme);
    return TRUE;
}

static void remove_dot_segments(xchar *path)
{
    xchar *input = path;
    xchar *output = path;

    if (!*path) {
        return;
    }

    while (*input) {
        if (strncmp(input, "../", 3) == 0) {
            input += 3;
        } else if (strncmp(input, "./", 2) == 0) {
            input += 2;
        } else if (strncmp(input, "/./", 3) == 0) {
            input += 2;
        } else if (strcmp(input, "/.") == 0) {
            input[1] = '\0';
        } else if (strncmp(input, "/../", 4) == 0) {
            input += 3;
            if (output > path) {
                do {
                    output--;
                } while (*output != '/' && output > path);
            }
        } else if (strcmp(input, "/..") == 0) {
            input[1] = '\0';
            if (output > path) {
                do {
                    output--;
                   } while (*output != '/' && output > path);
            }
        } else if (strcmp(input, "..") == 0 || strcmp(input, ".") == 0) {
            input[0] = '\0';
        } else {
            *output++ = *input++;
            while (*input && *input != '/') {
                *output++ = *input++;
            }
        }
    }

    *output = '\0';
}

XUri *x_uri_parse(const xchar *uri_string, XUriFlags flags, XError **error)
{
    x_return_val_if_fail(uri_string != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    return x_uri_parse_relative(NULL, uri_string, flags, error);
}

XUri *x_uri_parse_relative(XUri *base_uri, const xchar *uri_ref, XUriFlags flags, XError **error)
{
    XUri *uri = NULL;

    x_return_val_if_fail(uri_ref != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);
    x_return_val_if_fail(base_uri == NULL || base_uri->scheme != NULL, NULL);

    uri = x_atomic_rc_box_new0(XUri);
    uri->flags = flags;

    if (!x_uri_split_internal(uri_ref, flags, &uri->scheme, &uri->userinfo, &uri->user, &uri->password, &uri->auth_params, &uri->host, &uri->port, &uri->path, &uri->query, &uri->fragment, error)) {
        x_uri_unref(uri);
        return NULL;
    }

    if (!uri->scheme && !base_uri) {
        x_set_error_literal(error, X_URI_ERROR, X_URI_ERROR_FAILED, _("URI is not absolute, and no base URI was provided"));
        x_uri_unref(uri);
        return NULL;
    }

    if (base_uri) {
        if (uri->scheme) {
            remove_dot_segments(uri->path);
        } else {
            uri->scheme = x_strdup(base_uri->scheme);
            if (uri->host) {
                remove_dot_segments(uri->path);
            } else {
                if (!*uri->path) {
                    x_free(uri->path);
                    uri->path = x_strdup(base_uri->path);
                    if (!uri->query) {
                        uri->query = x_strdup(base_uri->query);
                    }
                } else {
                    if (*uri->path == '/') {
                        remove_dot_segments(uri->path);
                    } else {
                        xchar *newpath, *last;

                        last = strrchr(base_uri->path, '/');
                        if (last) {
                            newpath = x_strdup_printf("%.*s/%s", (xint)(last - base_uri->path), base_uri->path, uri->path);
                        } else {
                            newpath = x_strdup_printf("/%s", uri->path);
                        }

                        x_free(uri->path);
                        uri->path = x_steal_pointer(&newpath);

                        remove_dot_segments(uri->path);
                    }
                }

                uri->userinfo = x_strdup(base_uri->userinfo);
                uri->user = x_strdup(base_uri->user);
                uri->password = x_strdup(base_uri->password);
                uri->auth_params = x_strdup(base_uri->auth_params);
                uri->host = x_strdup(base_uri->host);
                uri->port = base_uri->port;
            }
        }

        if (flags & X_URI_FLAGS_SCHEME_NORMALIZE) {
            if (should_normalize_empty_path(uri->scheme) && !*uri->path) {
                x_free(uri->path);
                uri->path = x_strdup("/");
            }

            uri->port = normalize_port(uri->scheme, uri->port);
        }
    } else {
        remove_dot_segments(uri->path);
    }

    return x_steal_pointer(&uri);
}

xchar *x_uri_resolve_relative(const xchar *base_uri_string, const xchar *uri_ref, XUriFlags flags, XError **error)
{
    xchar *resolved_uri_string;
    XUri *base_uri, *resolved_uri;

    x_return_val_if_fail(uri_ref != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    flags = (XUriFlags)(flags | X_URI_FLAGS_ENCODED);

    if (base_uri_string) {
        base_uri = x_uri_parse(base_uri_string, flags, error);
        if (!base_uri) {
            return NULL;
        }
    } else {
        base_uri = NULL;
    }

    resolved_uri = x_uri_parse_relative(base_uri, uri_ref, flags, error);
    if (base_uri) {
        x_uri_unref(base_uri);
    }

    if (!resolved_uri) {
        return NULL;
    }

    resolved_uri_string = x_uri_to_string(resolved_uri);
    x_uri_unref(resolved_uri);

    return x_steal_pointer(&resolved_uri_string);
}

#define USERINFO_ALLOWED_CHARS              X_URI_RESERVED_CHARS_ALLOWED_IN_USERINFO
#define USER_ALLOWED_CHARS                  "!$&'()*+,="
#define PASSWORD_ALLOWED_CHARS              "!$&'()*+,=:"
#define AUTH_PARAMS_ALLOWED_CHARS           USERINFO_ALLOWED_CHARS
#define IP_ADDR_ALLOWED_CHARS               ":"
#define HOST_ALLOWED_CHARS                  X_URI_RESERVED_CHARS_SUBCOMPONENT_DELIMITERS
#define PATH_ALLOWED_CHARS                  X_URI_RESERVED_CHARS_ALLOWED_IN_PATH
#define QUERY_ALLOWED_CHARS                 X_URI_RESERVED_CHARS_ALLOWED_IN_PATH "?"
#define FRAGMENT_ALLOWED_CHARS              X_URI_RESERVED_CHARS_ALLOWED_IN_PATH "?"

static xchar *x_uri_join_internal(XUriFlags flags, const xchar *scheme, xboolean userinfo, const xchar *user, const xchar *password, const xchar *auth_params, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment)
{
    XString *str;
    char *normalized_scheme = NULL;
    xboolean encoded = (flags & X_URI_FLAGS_ENCODED);

    x_return_val_if_fail(path != NULL, NULL);
    x_return_val_if_fail(host == NULL || (path[0] == '\0' || path[0] == '/'), NULL);
    x_return_val_if_fail(host != NULL || (path[0] != '/' || path[1] != '/'), NULL);

    str = x_string_sized_new(127);
    if (scheme) {
        x_string_append(str, scheme);
        x_string_append_c(str, ':');
    }

    if (flags & X_URI_FLAGS_SCHEME_NORMALIZE && scheme && ((host && port != -1) || path[0] == '\0')) {
        normalized_scheme = x_ascii_strdown(scheme, -1);
    }

    if (host) {
        x_string_append(str, "//");

        if (user) {
            if (encoded) {
                x_string_append(str, user);
            } else {
                if (userinfo) {
                    x_string_append_uri_escaped(str, user, USERINFO_ALLOWED_CHARS, TRUE);
                } else {
                    x_string_append_uri_escaped(str, user, USER_ALLOWED_CHARS, TRUE);
                }
            }

            if (password) {
                x_string_append_c(str, ':');
                if (encoded) {
                    x_string_append(str, password);
                } else {
                    x_string_append_uri_escaped(str, password, PASSWORD_ALLOWED_CHARS, TRUE);
                }
            }

            if (auth_params) {
                x_string_append_c(str, ';');
                if (encoded) {
                    x_string_append(str, auth_params);
                } else {
                    x_string_append_uri_escaped(str, auth_params, AUTH_PARAMS_ALLOWED_CHARS, TRUE);
                }
            }

            x_string_append_c(str, '@');
        }

        if (strchr(host, ':') && x_hostname_is_ip_address(host)) {
            x_string_append_c(str, '[');
            if (encoded) {
                x_string_append(str, host);
            } else {
                x_string_append_uri_escaped(str, host, IP_ADDR_ALLOWED_CHARS, TRUE);
            }
            x_string_append_c(str, ']');
         } else {
            if (encoded) {
                x_string_append(str, host);
            } else {
                x_string_append_uri_escaped(str, host, HOST_ALLOWED_CHARS, TRUE);
            }
        }

        if (port != -1 && (!normalized_scheme || normalize_port(normalized_scheme, port) != -1)) {
            x_string_append_printf(str, ":%d", port);
        }
    }

    if (path[0] == '\0' && normalized_scheme && should_normalize_empty_path(normalized_scheme)) {
        x_string_append(str, "/");
    } else if (encoded || flags & X_URI_FLAGS_ENCODED_PATH) {
        x_string_append(str, path);
    } else {
        x_string_append_uri_escaped(str, path, PATH_ALLOWED_CHARS, TRUE);
    }

    x_free(normalized_scheme);

    if (query) {
        x_string_append_c(str, '?');
        if (encoded || flags & X_URI_FLAGS_ENCODED_QUERY) {
            x_string_append(str, query);
        } else {
            x_string_append_uri_escaped(str, query, QUERY_ALLOWED_CHARS, TRUE);
        }
    }

    if (fragment) {
        x_string_append_c(str, '#');
        if (encoded || flags & X_URI_FLAGS_ENCODED_FRAGMENT) {
            x_string_append(str, fragment);
        } else {
            x_string_append_uri_escaped(str, fragment, FRAGMENT_ALLOWED_CHARS, TRUE);
        }
    }

    return x_string_free(str, FALSE);
}

xchar *x_uri_join(XUriFlags flags, const xchar *scheme, const xchar *userinfo, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment)
{
    x_return_val_if_fail(port >= -1 && port <= 65535, NULL);
    x_return_val_if_fail(path != NULL, NULL);

    return x_uri_join_internal(flags, scheme, TRUE, userinfo, NULL, NULL, host, port, path, query, fragment);
}

xchar *x_uri_join_with_user(XUriFlags flags, const xchar *scheme, const xchar *user, const xchar *password, const xchar *auth_params, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment)
{
    x_return_val_if_fail(port >= -1 && port <= 65535, NULL);
    x_return_val_if_fail(path != NULL, NULL);

    return x_uri_join_internal(flags, scheme, FALSE, user, password, auth_params, host, port, path, query, fragment);
}

XUri *x_uri_build(XUriFlags flags, const xchar *scheme, const xchar *userinfo, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment)
{
    XUri *uri;

    x_return_val_if_fail(scheme != NULL, NULL);
    x_return_val_if_fail(port >= -1 && port <= 65535, NULL);
    x_return_val_if_fail(path != NULL, NULL);

    uri = x_atomic_rc_box_new0(XUri);
    uri->flags = flags;
    uri->scheme = x_ascii_strdown(scheme, -1);
    uri->userinfo = x_strdup(userinfo);
    uri->host = x_strdup(host);
    uri->port = port;
    uri->path = x_strdup(path);
    uri->query = x_strdup(query);
    uri->fragment = x_strdup(fragment);

    return x_steal_pointer(&uri);
}

XUri *x_uri_build_with_user(XUriFlags flags, const xchar *scheme, const xchar *user, const xchar *password, const xchar *auth_params, const xchar *host, xint port, const xchar *path, const xchar *query, const xchar *fragment)
{
    XUri *uri;
    XString *userinfo;

    x_return_val_if_fail(scheme != NULL, NULL);
    x_return_val_if_fail(password == NULL || user != NULL, NULL);
    x_return_val_if_fail(auth_params == NULL || user != NULL, NULL);
    x_return_val_if_fail(port >= -1 && port <= 65535, NULL);
    x_return_val_if_fail(path != NULL, NULL);

    uri = x_atomic_rc_box_new0(XUri);
    uri->flags = (XUriFlags)(flags | X_URI_FLAGS_HAS_PASSWORD);
    uri->scheme = x_ascii_strdown(scheme, -1);
    uri->user = x_strdup(user);
    uri->password = x_strdup(password);
    uri->auth_params = x_strdup(auth_params);
    uri->host = x_strdup(host);
    uri->port = port;
    uri->path = x_strdup(path);
    uri->query = x_strdup(query);
    uri->fragment = x_strdup(fragment);

    if (user) {
        userinfo = x_string_new(user);
        if (password) {
            x_string_append_c(userinfo, ':');
            x_string_append(userinfo, uri->password);
         }

        if (auth_params) {
            x_string_append_c(userinfo, ';');
            x_string_append(userinfo, uri->auth_params);
        }

        uri->userinfo = x_string_free(userinfo, FALSE);
    }

    return x_steal_pointer(&uri);
}

xchar *x_uri_to_string (XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return x_uri_to_string_partial(uri, X_URI_HIDE_NONE);
}

xchar *x_uri_to_string_partial(XUri *uri, XUriHideFlags flags)
{
    xboolean hide_user = (flags & X_URI_HIDE_USERINFO);
    xboolean hide_password = (flags & (X_URI_HIDE_USERINFO | X_URI_HIDE_PASSWORD));
    xboolean hide_auth_params = (flags & (X_URI_HIDE_USERINFO | X_URI_HIDE_AUTH_PARAMS));
    xboolean hide_query = (flags & X_URI_HIDE_QUERY);
    xboolean hide_fragment = (flags & X_URI_HIDE_FRAGMENT);

    x_return_val_if_fail(uri != NULL, NULL);

    if (uri->flags & (X_URI_FLAGS_HAS_PASSWORD | X_URI_FLAGS_HAS_AUTH_PARAMS)) {
        return x_uri_join_with_user(uri->flags, uri->scheme, hide_user ? NULL : uri->user, hide_password ? NULL : uri->password, hide_auth_params ? NULL : uri->auth_params, uri->host, uri->port, uri->path, hide_query ? NULL : uri->query, hide_fragment ? NULL : uri->fragment);
    }

    return x_uri_join(uri->flags, uri->scheme, hide_user ? NULL : uri->userinfo, uri->host, uri->port, uri->path, hide_query ? NULL : uri->query, hide_fragment ? NULL : uri->fragment);
}

static xuint str_ascii_case_hash(xconstpointer v)
{
    xuint32 h = 5381;
    const signed char *p;

    for (p = (const signed char *)v; *p != '\0'; p++) {
        h = (h << 5) + h + x_ascii_toupper(*p);
    }

    return h;
}

static xboolean str_ascii_case_equal(xconstpointer v1, xconstpointer v2)
{
    const xchar *string1 = (const xchar *)v1;
    const xchar *string2 = (const xchar *)v2;

    return x_ascii_strcasecmp(string1, string2) == 0;
}

typedef struct {
    XUriParamsFlags flags;
    const xchar     *attr;
    const xchar     *end;
    xuint8          sep_table[256];
} RealIter;

typedef struct {
    char     a;
    RealIter b;
} RealIterAlign;

typedef struct {
    char           a;
    XUriParamsIter b;
} XUriParamsIterAlign;

X_STATIC_ASSERT(sizeof(XUriParamsIter) == sizeof(RealIter));
// X_STATIC_ASSERT(X_ALIGNOF(XUriParamsIter) >= X_ALIGNOF(RealIter));
X_STATIC_ASSERT(X_STRUCT_OFFSET(XUriParamsIterAlign, b) >= X_STRUCT_OFFSET(RealIterAlign, b));

void x_uri_params_iter_init(XUriParamsIter *iter, const xchar *params, xssize length, const xchar *separators, XUriParamsFlags flags)
{
    const xchar *s;
    RealIter *ri = (RealIter *)iter;

    x_return_if_fail(iter != NULL);
    x_return_if_fail(length == 0 || params != NULL);
    x_return_if_fail(length >= -1);
    x_return_if_fail(separators != NULL);

    ri->flags = flags;

    if (length == -1) {
        ri->end = params + strlen(params);
    } else {
        ri->end = params + length;
    }

    memset(ri->sep_table, FALSE, sizeof (ri->sep_table));
    for (s = separators; *s != '\0'; ++s) {
        ri->sep_table[*(xuchar *)s] = TRUE;
    }

    ri->attr = params;
}

xboolean x_uri_params_iter_next(XUriParamsIter *iter, xchar **attribute, xchar **value, XError **error)
{
    RealIter *ri = (RealIter *)iter;
    xchar *decoded_attr, *decoded_value;
    const xchar *attr_end, *val, *val_end;
    XUriFlags decode_flags = X_URI_FLAGS_NONE;
    xboolean www_form = ri->flags & X_URI_PARAMS_WWW_FORM;

    x_return_val_if_fail(iter != NULL, FALSE);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (attribute) {
        *attribute = NULL;
    }

    if (value) {
        *value = NULL;
    }

    if (ri->attr >= ri->end) {
        return FALSE;
    }

    if (ri->flags & X_URI_PARAMS_PARSE_RELAXED) {
        decode_flags = (XUriFlags)(decode_flags | X_URI_FLAGS_PARSE_RELAXED);
    }

    for (val_end = ri->attr; val_end < ri->end; val_end++) {
        if (ri->sep_table[*(xuchar *)val_end]) {
            break;
        }
    }

    attr_end = (const xchar *)memchr(ri->attr, '=', val_end - ri->attr);
    if (!attr_end) {
        x_set_error_literal(error, X_URI_ERROR, X_URI_ERROR_FAILED, _("Missing ‘=’ and parameter value"));
        return FALSE;
    }

    if (!uri_decode(&decoded_attr, NULL, ri->attr, attr_end - ri->attr, www_form, decode_flags, X_URI_ERROR_FAILED, error)) {
        return FALSE;
    }

    val = attr_end + 1;
    if (!uri_decode(&decoded_value, NULL, val, val_end - val, www_form, decode_flags, X_URI_ERROR_FAILED, error)) {
        x_free(decoded_attr);
        return FALSE;
    }

    if (attribute) {
        *attribute = x_steal_pointer(&decoded_attr);
    }

    if (value) {
        *value = x_steal_pointer(&decoded_value);
    }

    x_free(decoded_attr);
    x_free(decoded_value);

    ri->attr = val_end + 1;
    return TRUE;
}

XHashTable *x_uri_parse_params(const xchar *params, xssize length, const xchar *separators, XUriParamsFlags flags, XError **error)
{
    XHashTable *hash;
    XError *err = NULL;
    XUriParamsIter iter;
    xchar *attribute, *value;

    x_return_val_if_fail(length == 0 || params != NULL, NULL);
    x_return_val_if_fail(length >= -1, NULL);
    x_return_val_if_fail(separators != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (flags & X_URI_PARAMS_CASE_INSENSITIVE) {
        hash = x_hash_table_new_full(str_ascii_case_hash, str_ascii_case_equal, x_free, x_free);
    } else {
        hash = x_hash_table_new_full(x_str_hash, x_str_equal, x_free, x_free);
    }

    x_uri_params_iter_init(&iter, params, length, separators, flags);

    while (x_uri_params_iter_next(&iter, &attribute, &value, &err)) {
        x_hash_table_insert(hash, attribute, value);
    }

    if (err) {
        x_propagate_error(error, x_steal_pointer(&err));
        x_hash_table_destroy(hash);
        return NULL;
    }

    return x_steal_pointer(&hash);
}

const xchar *x_uri_get_scheme(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->scheme;
}

const xchar *x_uri_get_userinfo(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->userinfo;
}

const xchar *x_uri_get_user(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->user;
}

const xchar *x_uri_get_password(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->password;
}

const xchar *x_uri_get_auth_params(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->auth_params;
}

const xchar *x_uri_get_host(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->host;
}

xint x_uri_get_port(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, -1);

    if (uri->port == -1 && uri->flags & X_URI_FLAGS_SCHEME_NORMALIZE) {
        return x_uri_get_default_scheme_port(uri->scheme);
    }

    return uri->port;
}

const xchar *x_uri_get_path(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->path;
}

const xchar *x_uri_get_query(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->query;
}

const xchar *x_uri_get_fragment(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, NULL);
    return uri->fragment;
}

XUriFlags x_uri_get_flags(XUri *uri)
{
    x_return_val_if_fail(uri != NULL, X_URI_FLAGS_NONE);
    return uri->flags;
}

xchar *x_uri_unescape_segment(const xchar *escaped_string, const xchar *escaped_string_end, const xchar *illegal_characters)
{
    xsize length;
    xchar *unescaped;
    xssize decoded_len;

    if (!escaped_string) {
        return NULL;
    }

    if (escaped_string_end) {
        length = escaped_string_end - escaped_string;
    } else {
        length = strlen(escaped_string);
    }

    decoded_len = uri_decoder(&unescaped, illegal_characters, escaped_string, length, FALSE, FALSE, X_URI_FLAGS_ENCODED, (XUriError)0, NULL);
    if (decoded_len < 0) {
        return NULL;
    }

    if (memchr(unescaped, '\0', decoded_len)) {
        x_free(unescaped);
        return NULL;
    }

    return unescaped;
}

xchar *x_uri_unescape_string(const xchar *escaped_string, const xchar *illegal_characters)
{
    return x_uri_unescape_segment(escaped_string, NULL, illegal_characters);
}

xchar *x_uri_escape_string (const xchar *unescaped, const xchar *reserved_chars_allowed, xboolean allow_utf8)
{
    XString *s;

    x_return_val_if_fail(unescaped != NULL, NULL);
    s = x_string_sized_new((size_t)(strlen(unescaped) * 1.25));
    x_string_append_uri_escaped(s, unescaped, reserved_chars_allowed, allow_utf8);

    return x_string_free(s, FALSE);
}

XBytes *x_uri_unescape_bytes(const xchar *escaped_string, xssize length, const char *illegal_characters, XError **error)
{
    xchar *buf;
    xssize unescaped_length;

    x_return_val_if_fail(escaped_string != NULL, NULL);
    x_return_val_if_fail(error == NULL || *error == NULL, NULL);

    if (length == -1) {
        length = strlen(escaped_string);
    }

    unescaped_length = uri_decoder(&buf, illegal_characters, escaped_string, length, FALSE, FALSE, X_URI_FLAGS_ENCODED, X_URI_ERROR_FAILED, error);
    if (unescaped_length == -1) {
        return NULL;
    }

    return x_bytes_new_take(buf, unescaped_length);
}

xchar *x_uri_escape_bytes(const xuint8 *unescaped, xsize length, const xchar *reserved_chars_allowed)
{
    XString *string;

    x_return_val_if_fail(unescaped != NULL, NULL);
    string = x_string_sized_new((size_t)(length * 1.25));
    _uri_encoder(string, unescaped, length, reserved_chars_allowed, FALSE);

    return x_string_free(string, FALSE);
}

static xssize x_uri_scheme_length(const xchar *uri)
{
    const xchar *p;

    p = uri;
    if (!x_ascii_isalpha(*p)) {
        return -1;
    }

    p++;
    while (x_ascii_isalnum(*p) || *p == '.' || *p == '+' || *p == '-') {
        p++;
    }

    if (p > uri && *p == ':') {
        return p - uri;
    }

    return -1;
}

xchar *x_uri_parse_scheme(const xchar *uri)
{
    xssize len;

    x_return_val_if_fail(uri != NULL, NULL);

    len = x_uri_scheme_length(uri);
    return len == -1 ? NULL : x_strndup(uri, len);
}

const xchar *x_uri_peek_scheme(const xchar *uri)
{
    xssize len;
    xchar *lower_scheme;
    const xchar *scheme;

    x_return_val_if_fail(uri != NULL, NULL);

    len = x_uri_scheme_length(uri);
    if (len == -1) {
        return NULL;
    }

    lower_scheme = x_ascii_strdown(uri, len);
    scheme = x_intern_string(lower_scheme);
    x_free(lower_scheme);

    return scheme;
}

X_DEFINE_QUARK(x-uri-quark, x_uri_error)
