#include "acl/lib_acl/StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "acl/lib_acl/stdlib/acl_define.h"
#include "acl/lib_acl/stdlib/acl_stdlib.h"
#include "acl/lib_acl/proctl/acl_proctl.h"

#endif

void acl_proctl_deamon_init(const char *progname acl_unused)
{
    const char *myname = "acl_proctl_deamon_init";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_daemon_loop(void)
{
    const char *myname = "acl_proctl_daemon_loop";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

int acl_proctl_deamon_start_one(const char *progchild acl_unused,
    int argc acl_unused, char *argv[] acl_unused)
{
    const char *myname = "acl_proctl_deamon_start_one";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);

    return (-1);
}

void acl_proctl_start_one(const char *progname acl_unused,
    const char *progchild acl_unused,
    int argc acl_unused, char *argv[] acl_unused)
{
    const char *myname = "acl_proctl_start_one";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_stop_one(const char *progname acl_unused,
    const char *progchild acl_unused,
    int argc acl_unused, char *argv[] acl_unused)
{
    const char *myname = "acl_proctl_stop_one";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_stop_all(const char *progname acl_unused)
{
    const char *myname = "acl_proctl_stop_all";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_quit(const char *progname acl_unused)
{
    const char *myname = "acl_proctl_quit";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}

void acl_proctl_child(const char *progname acl_unused,
    void (*onexit_fn)(void *) acl_unused, void *arg acl_unused)
{
    const char *myname = "acl_proctl_child";

    acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
}
