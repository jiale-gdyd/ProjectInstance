#ifndef __X_SCANNER_H__
#define __X_SCANNER_H__

#include "xdataset.h"
#include "xhash.h"

X_BEGIN_DECLS

typedef struct _XScanner XScanner;
typedef union _XTokenValue XTokenValue;
typedef struct _XScannerConfig XScannerConfig;

typedef void (*XScannerMsgFunc)(XScanner *scanner, xchar *message, xboolean error);

#define X_CSET_A_2_Z                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define X_CSET_a_2_z                    "abcdefghijklmnopqrstuvwxyz"
#define X_CSET_DIGITS                   "0123456789"
#define X_CSET_LATINC                   "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\330\331\332\333\334\335\336"
#define X_CSET_LATINS                   "\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\370\371\372\373\374\375\376\377"

typedef enum {
    X_ERR_UNKNOWN,
    X_ERR_UNEXP_EOF,
    X_ERR_UNEXP_EOF_IN_STRING,
    X_ERR_UNEXP_EOF_IN_COMMENT,
    X_ERR_NON_DIGIT_IN_CONST,
    X_ERR_DIGIT_RADIX,
    X_ERR_FLOAT_RADIX,
    X_ERR_FLOAT_MALFORMED
} XErrorType;

typedef enum {
    X_TOKEN_EOF         =   0,

    X_TOKEN_LEFT_PAREN  = '(',
    X_TOKEN_RIGHT_PAREN = ')',
    X_TOKEN_LEFT_CURLY  = '{',
    X_TOKEN_RIGHT_CURLY = '}',
    X_TOKEN_LEFT_BRACE  = '[',
    X_TOKEN_RIGHT_BRACE = ']',
    X_TOKEN_EQUAL_SIGN  = '=',
    X_TOKEN_COMMA       = ',',

    X_TOKEN_NONE        = 256,
    X_TOKEN_ERROR,

    X_TOKEN_CHAR,
    X_TOKEN_BINARY,
    X_TOKEN_OCTAL,
    X_TOKEN_INT,
    X_TOKEN_HEX,
    X_TOKEN_FLOAT,
    X_TOKEN_STRING,

    X_TOKEN_SYMBOL,
    X_TOKEN_IDENTIFIER,
    X_TOKEN_IDENTIFIER_NULL,

    X_TOKEN_COMMENT_SINGLE,
    X_TOKEN_COMMENT_MULTI,

    X_TOKEN_LAST
} XTokenType;

union _XTokenValue {
    xpointer v_symbol;
    xchar    *v_identifier;
    xulong   v_binary;
    xulong   v_octal;
    xulong   v_int;
    xuint64  v_int64;
    xdouble  v_float;
    xulong   v_hex;
    xchar    *v_string;
    xchar    *v_comment;
    xuchar   v_char;
    xuint    v_error;
};

struct _XScannerConfig {
    xchar *cset_skip_characters;
    xchar *cset_identifier_first;
    xchar *cset_identifier_nth;
    xchar *cpair_comment_single;

    xuint case_sensitive : 1;

    xuint skip_comment_multi : 1;
    xuint skip_comment_single : 1;
    xuint scan_comment_multi : 1;
    xuint scan_identifier : 1;
    xuint scan_identifier_1char : 1;
    xuint scan_identifier_NULL : 1;
    xuint scan_symbols : 1;
    xuint scan_binary : 1;
    xuint scan_octal : 1;
    xuint scan_float : 1;
    xuint scan_hex : 1;
    xuint scan_hex_dollar : 1;
    xuint scan_string_sq : 1;
    xuint scan_string_dq : 1;
    xuint numbers_2_int : 1;
    xuint int_2_float : 1;
    xuint identifier_2_string : 1;
    xuint char_2_token : 1;
    xuint symbol_2_token : 1;
    xuint scope_0_fallback : 1;
    xuint store_int64 : 1;

    xuint padding_dummy;
};

struct _XScanner {
    xpointer        user_data;
    xuint           max_parse_errors;

    xuint           parse_errors;
    const xchar     *input_name;
    XData           *qdata;

    XScannerConfig  *config;

    XTokenType      token;
    XTokenValue     value;
    xuint           line;
    xuint           position;

    XTokenType      next_token;
    XTokenValue     next_value;
    xuint           next_line;
    xuint           next_position;

    XHashTable      *symbol_table;
    xint            input_fd;
    const xchar     *text;
    const xchar     *text_end;
    xchar           *buffer;
    xuint           scope_id;

    XScannerMsgFunc msg_handler;
};

XLIB_AVAILABLE_IN_ALL
XScanner *x_scanner_new(const XScannerConfig *config_templ);

XLIB_AVAILABLE_IN_ALL
void x_scanner_destroy(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
void x_scanner_input_file(XScanner *scanner, xint input_fd);

XLIB_AVAILABLE_IN_ALL
void x_scanner_sync_file_offset(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
void x_scanner_input_text(XScanner *scanner, const xchar *text, xuint text_len);

XLIB_AVAILABLE_IN_ALL
XTokenType x_scanner_get_next_token(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
XTokenType x_scanner_peek_next_token(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
XTokenType x_scanner_cur_token(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
XTokenValue x_scanner_cur_value(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
xuint x_scanner_cur_line(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
xuint x_scanner_cur_position(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
xboolean x_scanner_eof(XScanner *scanner);

XLIB_AVAILABLE_IN_ALL
xuint x_scanner_set_scope(XScanner *scanner, xuint scope_id);

XLIB_AVAILABLE_IN_ALL
void x_scanner_scope_add_symbol(XScanner *scanner, xuint scope_id, const xchar *symbol, xpointer value);

XLIB_AVAILABLE_IN_ALL
void x_scanner_scope_remove_symbol(XScanner *scanner, xuint scope_id, const xchar *symbol);

XLIB_AVAILABLE_IN_ALL
xpointer x_scanner_scope_lookup_symbol(XScanner *scanner, xuint scope_id, const xchar *symbol);

XLIB_AVAILABLE_IN_ALL
void x_scanner_scope_foreach_symbol(XScanner *scanner, xuint scope_id, XHFunc func, xpointer user_data);

XLIB_AVAILABLE_IN_ALL
xpointer x_scanner_lookup_symbol(XScanner *scanner, const xchar *symbol);

XLIB_AVAILABLE_IN_ALL
void x_scanner_unexp_token(XScanner *scanner, XTokenType expected_token, const xchar *identifier_spec, const xchar *symbol_spec, const xchar *symbol_name, const xchar *message, xint is_error);

XLIB_AVAILABLE_IN_ALL
void x_scanner_error(XScanner *scanner, const xchar *format, ...) X_GNUC_PRINTF (2,3);

XLIB_AVAILABLE_IN_ALL
void x_scanner_warn(XScanner *scanner, const xchar *format, ...) X_GNUC_PRINTF (2,3);

#define x_scanner_add_symbol(scanner, symbol, value)                                \
    X_STMT_START {                                                                  \
        x_scanner_scope_add_symbol((scanner), 0, (symbol), (value));                \
    } X_STMT_END XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_scanner_scope_add_symbol)

#define x_scanner_remove_symbol(scanner, symbol)                                    \
    X_STMT_START {                                                                  \
        x_scanner_scope_remove_symbol((scanner), 0, (symbol));                      \
    } X_STMT_END XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_scanner_scope_remove_symbol)

#define x_scanner_foreach_symbol(scanner, func, data)                               \
    X_STMT_START {                                                                  \
        x_scanner_scope_foreach_symbol((scanner), 0, (func), (data));               \
    } X_STMT_END XLIB_DEPRECATED_MACRO_IN_2_26_FOR(x_scanner_scope_foreach_symbol)

#define x_scanner_freeze_symbol_table(scanner)  ((void)0) XLIB_DEPRECATED_MACRO_IN_2_26
#define x_scanner_thaw_symbol_table(scanner)    ((void)0) XLIB_DEPRECATED_MACRO_IN_2_26

X_END_DECLS

#endif
