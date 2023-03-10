#ifndef __XLIB_H__
#define __XLIB_H__

#define __XLIB_H_INSIDE__

#include "xlib/xalloca.h"
#include "xlib/xarray.h"
#include "xlib/xasyncqueue.h"
#include "xlib/xatomic.h"
#include "xlib/xbacktrace.h"
#include "xlib/xbase64.h"
#include "xlib/xbitlock.h"
#include "xlib/xbookmarkfile.h"
#include "xlib/xbytes.h"
#include "xlib/xcharset.h"
#include "xlib/xchecksum.h"
#include "xlib/xconvert.h"
#include "xlib/xdataset.h"
#include "xlib/xdate.h"
#include "xlib/xdatetime.h"
#include "xlib/xdir.h"
#include "xlib/xenviron.h"
#include "xlib/xerror.h"
#include "xlib/xfileutils.h"
#include "xlib/xgettext.h"
#include "xlib/xhash.h"
#include "xlib/xhmac.h"
#include "xlib/xhook.h"
#include "xlib/xhostutils.h"
#include "xlib/xiochannel.h"
#include "xlib/xkeyfile.h"
#include "xlib/xlist.h"
#include "xlib/xmacros.h"
#include "xlib/xmain.h"
#include "xlib/xmappedfile.h"
#include "xlib/xmarkup.h"
#include "xlib/xmem.h"
#include "xlib/xmessages.h"
#include "xlib/xnode.h"
#include "xlib/xoption.h"
#include "xlib/xpathbuf.h"
#include "xlib/xpattern.h"
#include "xlib/xpoll.h"
#include "xlib/xprimes.h"
#include "xlib/xqsort.h"
#include "xlib/xquark.h"
#include "xlib/xqueue.h"
#include "xlib/xrand.h"
#include "xlib/xrcbox.h"
#include "xlib/xrefcount.h"
#include "xlib/xrefstring.h"
#include "xlib/xregex.h"
#include "xlib/xscanner.h"
#include "xlib/xsequence.h"
#include "xlib/xshell.h"
#include "xlib/xslice.h"
#include "xlib/xslist.h"
#include "xlib/xspawn.h"
#include "xlib/xstrfuncs.h"
#include "xlib/xstringchunk.h"
#include "xlib/xstring.h"
#include "xlib/xstrvbuilder.h"
#include "xlib/xtestutils.h"
#include "xlib/xthread.h"
#include "xlib/xthreadpool.h"
#include "xlib/xtimer.h"
#include "xlib/xtimezone.h"
#include "xlib/xtrashstack.h"
#include "xlib/xtree.h"
#include "xlib/xtypes.h"
#include "xlib/xunicode.h"
#include "xlib/xuri.h"
#include "xlib/xutils.h"
#include "xlib/xuuid.h"
#include "xlib/xvariant.h"
#include "xlib/xvarianttype.h"
#include "xlib/xversion.h"
#include "xlib/xversionmacros.h"

#include "xlib/deprecated/xallocator.h"
#include "xlib/deprecated/xcache.h"
#include "xlib/deprecated/xcompletion.h"
#include "xlib/deprecated/xmain.h"
#include "xlib/deprecated/xrel.h"
#include "xlib/deprecated/xthread.h"

#include "xmod/xmodule.h"

#include "xlib/xlib-autocleanups.h"
#include "xlib/xlib-typeof.h"

#undef __XLIB_H_INSIDE__

#endif
