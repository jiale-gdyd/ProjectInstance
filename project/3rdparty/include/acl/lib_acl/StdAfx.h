#ifndef ACL_LIBACL_STDAFX_H
#define ACL_LIBACL_STDAFX_H

#ifdef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <string.h>
#include <errno.h>
#include <float.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>

#ifdef	ACL_UNIX

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

#ifdef ACL_FREEBSD
#include <netinet/in_systm.h>
#include <netinet/in.h>
#endif

#endif

#include "lib_acl.h"

#endif

#endif
