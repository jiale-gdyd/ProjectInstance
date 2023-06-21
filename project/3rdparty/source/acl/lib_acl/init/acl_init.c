#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <signal.h>
#endif
#include "acl/lib_acl/stdlib/acl_sys_patch.h"
#include "acl/lib_acl/stdlib/acl_msg.h"
#include "acl/lib_acl/stdlib/acl_vstream.h"
#include "acl/lib_acl/stdlib/acl_vstring.h"
#include "acl/lib_acl/thread/acl_pthread.h"
#include "acl/lib_acl/init/acl_init.h"

#endif /* ACL_PREPARE_COMPILE */

#include "init.h"

static char *version = "3.6.1-2 20230322-15:35";

const char *acl_version(void)
{
    return version;
}

const char *acl_verbose(void)
{
    static ACL_VSTRING *buf = NULL;
    if (buf) {
        ACL_VSTRING_RESET(buf);
    } else {
        buf = acl_vstring_alloc(128);
    }

    acl_vstring_strcpy(buf, version);

#ifdef HAS_ATOMIC
    acl_vstring_strcat(buf, ", HAS_ATOMIC");
#endif

#ifdef HAVE_NO_ATEXIT
    acl_vstring_strcat(buf, ", HAVE_NO_ATEXIT");
#endif

    return acl_vstring_str(buf);
}

#ifdef ACL_UNIX
static acl_pthread_t acl_var_main_tid = (acl_pthread_t) -1;
#elif defined(ACL_WINDOWS)
static unsigned long acl_var_main_tid = (unsigned long) -1;
#else
#error "Unknown OS"
#endif

#ifdef	ACL_UNIX
void acl_lib_init(void) __attribute__ ((constructor));
#endif

void acl_lib_init(void)
{
    static int __have_inited = 0;

    if (__have_inited)
        return;
    __have_inited = 1;
#ifdef ACL_UNIX
    signal(SIGPIPE, SIG_IGN);
#elif  defined(ACL_WINDOWS)
    acl_socket_init();
    acl_vstream_init();
#endif
    acl_var_main_tid = acl_pthread_self();
}

#ifdef	ACL_UNIX
void acl_lib_end(void) __attribute__ ((destructor));
#endif

void acl_lib_end(void)
{
    static int __have_ended = 0;

    if (__have_ended)
        return;
    __have_ended = 1;
#if  defined(ACL_WINDOWS)
    acl_socket_end();
    acl_pthread_end();
#endif
}

int __acl_var_use_poll = 1;

void acl_poll_prefered(int yesno)
{
    __acl_var_use_poll = yesno;
}

#ifdef ACL_WINDOWS
static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void get_main_thread_id(void)
{
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 th32;
    DWORD currentPID;
    BOOL  bOk;

    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return;
    currentPID = GetCurrentProcessId();
    th32.dwSize = sizeof(THREADENTRY32);

    for (bOk = Thread32First(hThreadSnap, &th32); bOk;
        bOk = Thread32Next(hThreadSnap, &th32))
    {
        if (th32.th32OwnerProcessID == currentPID) {
            acl_var_main_tid = th32.th32ThreadID;
            break;
        }
    }
}
#endif

unsigned long acl_main_thread_self()
{
#ifdef ACL_UNIX
    return ((unsigned long) acl_var_main_tid);
#elif defined(ACL_WINDOWS)
    if (acl_var_main_tid == (unsigned long) -1)
        acl_pthread_once(&__once_control, get_main_thread_id);
    return (acl_var_main_tid);
#else
#error "Unknown OS"
#endif
}
