#ifndef __XLIBINTL_H__
#define __XLIBINTL_H__

#include "config.h"
#include "xtypes.h"
#include "xmacros.h"
#include "xversionmacros.h"

#ifndef SIZEOF_CHAR
#error "config.h must be included prior to xlibintl.h"
#endif

XLIB_AVAILABLE_IN_ALL
const xchar *xlib_gettext(const xchar *str);// X_GNUC_FORMAT(1);

XLIB_AVAILABLE_IN_ALL
const xchar *xlib_pgettext(const xchar *msgctxtid, xsize msgidoffset);// X_GNUC_FORMAT(1);

#ifdef ENABLE_NLS

#include <libintl.h>

#ifndef _
#define _(String)                                   xlib_gettext(String)
#endif

#define P_(String)                                  xlib_gettext(String)

#ifndef C_
#define C_(Context, String)                         xlib_pgettext(Context "\004" String, strlen(Context) + 1)
#endif

#ifdef gettext_noop
#define N_(String)                                  gettext_noop(String)
#else
#define N_(String)                                  (String)
#endif

#else

#define _(String)                                   (String)
#define N_(String)                                  (String)
#define P_(String)                                  (String)
#define C_(Context, String)                         (String)
#define textdomain(String)                          ((String) ? (String) : "messages")
#define gettext(String)                             (String)
#define dgettext(Domain, String)                    (String)
#define dcgettext(Domain, String, Type)             (String)
#define dngettext(Domain, String1, String2, N)      ((N) == 1 ? (String1) : (String2))
#define bindtextdomain(Domain, Directory)           (Domain) 
#define bind_textdomain_codeset(Domain, Codeset)
#endif

#define I_(string)                                  x_intern_static_string(string)

#endif
