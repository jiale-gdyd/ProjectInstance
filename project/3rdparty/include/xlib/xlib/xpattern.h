#ifndef __X_PATTERN_H__
#define __X_PATTERN_H__

#include "xtypes.h"

X_BEGIN_DECLS

typedef struct _XPatternSpec XPatternSpec;

XLIB_AVAILABLE_IN_ALL
XPatternSpec *x_pattern_spec_new(const xchar *pattern);

XLIB_AVAILABLE_IN_ALL
void x_pattern_spec_free(XPatternSpec *pspec);

XLIB_AVAILABLE_IN_2_70
XPatternSpec *x_pattern_spec_copy(XPatternSpec *pspec);

XLIB_AVAILABLE_IN_ALL
xboolean x_pattern_spec_equal(XPatternSpec *pspec1, XPatternSpec *pspec2);

XLIB_AVAILABLE_IN_2_70
xboolean x_pattern_spec_match(XPatternSpec *pspec, xsize string_length, const xchar *string, const xchar *string_reversed);

XLIB_AVAILABLE_IN_2_70
xboolean x_pattern_spec_match_string(XPatternSpec *pspec, const xchar *string);

XLIB_DEPRECATED_IN_2_70_FOR(x_pattern_spec_match)
xboolean x_pattern_match(XPatternSpec *pspec, xuint string_length, const xchar *string, const xchar *string_reversed);

XLIB_DEPRECATED_IN_2_70_FOR(x_pattern_spec_match_string)
xboolean x_pattern_match_string(XPatternSpec *pspec, const xchar *string);

XLIB_AVAILABLE_IN_ALL
xboolean x_pattern_match_simple(const xchar *pattern, const xchar *string);

X_END_DECLS

#endif
