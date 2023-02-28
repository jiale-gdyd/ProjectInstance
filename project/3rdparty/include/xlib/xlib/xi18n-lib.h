#ifndef __X_I18N_LIB_H__
#define __X_I18N_LIB_H__

#include <string.h>
#include "../xlib.h"
#include "xlibintl.h"

#ifndef _
#define  _(String)              ((char *)x_dgettext(GETTEXT_PACKAGE, String))
#endif

#define Q_(String)              x_dpgettext(GETTEXT_PACKAGE, String, 0)
#define N_(String)              (String)

#ifndef C_
#define C_(Context, String)     x_dpgettext(GETTEXT_PACKAGE, Context "\004" String, strlen(Context) + 1)
#endif

#define NC_(Context, String)    (String)

#endif
