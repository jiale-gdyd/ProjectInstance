#include "acl/lib_fiber/stdafx.hpp"
#include <stdarg.h>
#if defined(_WIN32) || defined(_WIN64)
#include <process.h>
#else
#include <poll.h>
#endif

/* including the internal headers from lib_acl/src/master */
#include "../../lib_acl/master/template/master_log.h"
#include "winapi_hook.hpp"

#define STACK_SIZE	128000

static int   acl_var_fiber_pid;
static char *acl_var_fiber_procname = NULL;
static char *acl_var_fiber_log_file = NULL;

static int   acl_var_fiber_stack_size = STACK_SIZE;
static int   acl_var_fiber_buf_size;
static int   acl_var_fiber_rw_timeout;
static int   acl_var_fiber_max_debug;
static int   acl_var_fiber_enable_core;
static int   acl_var_fiber_core_limit;
static int   acl_var_fiber_disable_core_onexit;
static int   acl_var_fiber_use_limit;
static int   acl_var_fiber_idle_limit;
static int   acl_var_fiber_wait_limit;
static int   acl_var_fiber_threads;
static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
    { "fiber_stack_size", STACK_SIZE, &acl_var_fiber_stack_size, 0, 0 },
    { "fiber_buf_size", 8192, &acl_var_fiber_buf_size, 0, 0 },
    { "fiber_rw_timeout", 120, &acl_var_fiber_rw_timeout, 0, 0 },
    { "fiber_max_debug", 1000, &acl_var_fiber_max_debug, 0, 0 },
    { "fiber_enable_core", 1, &acl_var_fiber_enable_core, 0, 0 },
    { "fiber_core_limit", -1, &acl_var_fiber_core_limit, 0, 0 },
    { "fiber_disable_core_onexit", 1, &acl_var_fiber_disable_core_onexit, 0, 0 },
    { "fiber_use_limit", 0, &acl_var_fiber_use_limit, 0, 0 },
    { "fiber_idle_limit", 0, &acl_var_fiber_idle_limit, 0 , 0 },
    { "fiber_wait_limit", 10, &acl_var_fiber_wait_limit, 0, 0 },
    { "fiber_threads", 1, &acl_var_fiber_threads, 0, 0 },

    { 0, 0, 0, 0, 0 },
};

static char *acl_var_fiber_master_service;
static char *acl_var_fiber_queue_dir;
static char *acl_var_fiber_log_debug;
static char *acl_var_fiber_deny_banner;
static char *acl_var_fiber_access_allow;
static char *acl_var_fiber_owner;
static char *acl_var_fiber_dispatch_addr;
static char *acl_var_fiber_dispatch_type;
static char *acl_var_fiber_master_reuseport;     /* just for stand alone */
static char *acl_var_fiber_master_private;
static char *acl_var_fiber_schedule_event;

static int var_fiber_master_reuseport = 0;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
    { "master_service", "", &acl_var_fiber_master_service },
    { "master_debug", "all:1", &acl_var_fiber_log_debug },
    { "master_reuseport", "", &acl_var_fiber_master_reuseport },
    { "master_private", "n", &acl_var_fiber_master_private },
    { "fiber_queue_dir", "/opt/soft/acl-master/var", &acl_var_fiber_queue_dir },
    { "fiber_deny_banner", "Denied!\r\n", &acl_var_fiber_deny_banner },
    { "fiber_access_allow", "all", &acl_var_fiber_access_allow },
    { "fiber_owner", "", &acl_var_fiber_owner },
    { "fiber_dispatch_addr", "", &acl_var_fiber_dispatch_addr },
    { "fiber_dispatch_type", "default", &acl_var_fiber_dispatch_type },
    { "fiber_schedule_event", "kernel", &acl_var_fiber_schedule_event },

    { 0, 0, 0 },
};

static int  acl_var_fiber_quick_abort;
static int  acl_var_fiber_share_stack;
static int  acl_var_fiber_hook_log;
static ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
    { "fiber_quick_abort", 1, &acl_var_fiber_quick_abort },
    { "fiber_share_stack", 0, &acl_var_fiber_share_stack },
    { "fiber_hook_log", 1, &acl_var_fiber_hook_log },

    { 0, 0, 0 },
};

static int    __daemon_mode = 0;
static void (*__service)(void*, ACL_VSTREAM*) = NULL;
static void  *__service_ctx = NULL;
static char   __service_name[256];
static void (*__service_onexit)(void*) = NULL;
static char  *__deny_info = NULL;
static ACL_MASTER_SERVER_ON_LISTEN_FN   __server_on_listen = NULL;
static ACL_MASTER_SERVER_THREAD_INIT_FN __thread_init;
static ACL_MASTER_SERVER_SIGHUP_FN      __sighup_handler = NULL;
static void  *__thread_init_ctx = NULL;
static char  __conf_file[1024];

static int      __fiber_schedule_event = FIBER_EVENT_KERNEL;
static unsigned __server_generation;
static int      __server_stopping = 0;

static ACL_ATOMIC_CLOCK *__clock = NULL;

typedef struct FIBER_SERVER {
    acl_pthread_t tid;
    ACL_MBOX     *in;
    ACL_MBOX     *out;
    ACL_VSTREAM **sstreams;
    ACL_FIBER   **accepters;
    int           socket_count;
    int           fdtype;
} FIBER_SERVER;

const char *acl_fiber_server_conf(void)
{
    return __conf_file;
}

static void fiber_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
    ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
    const char *peer = ACL_VSTREAM_PEER(cstream);
    char  addr[256];

    if (peer) {
        char *ptr;
        ACL_SAFE_STRNCPY(addr, peer, sizeof(addr));
        ptr = strchr(addr, ':');
        if (ptr) {
            *ptr = 0;
        }
    } else {
        addr[0] = 0;
    }

    if (addr[0] != 0 && !acl_access_permit(addr)) {
        if (__deny_info && *__deny_info) {
            acl_vstream_fprintf(cstream, "%s\r\n", __deny_info);
        }
        acl_vstream_close(cstream);
    } else {
        acl_atomic_clock_users_count_inc(__clock);
        __service(__service_ctx, cstream);
        acl_atomic_clock_users_add(__clock, -1);
        acl_vstream_close(cstream);
    }
}

static void thread_fiber_accept(ACL_FIBER *fiber, void *ctx)
{
    static socket_t __max_fd = 0, __last_fd = 0;
    ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx, *cstream;
    char  ip[64];
    ACL_FIBER_ATTR attr;

    acl_fiber_attr_init(&attr);
    acl_fiber_attr_setstacksize(&attr, acl_var_fiber_stack_size);
    acl_fiber_attr_setsharestack(&attr, acl_var_fiber_share_stack ? 1 : 0);

    // We must set the listen fd in blocking mode for io_uring engine.
    if (__fiber_schedule_event == FIBER_EVENT_IO_URING) {
        acl_non_blocking(ACL_VSTREAM_SOCK(sstream), ACL_BLOCKING);
    }

    while (1) {
        cstream = acl_vstream_accept(sstream, ip, sizeof(ip));
        if (cstream != NULL) {
            __last_fd = ACL_VSTREAM_SOCK(cstream);
            if (__last_fd > __max_fd) {
                __max_fd = __last_fd;
            }

            acl_fiber_create2(&attr, fiber_client, cstream);
            continue;
        }

        if (acl_fiber_killed(fiber)) {
            acl_msg_info("%s(%d), %s: fiber accept was killed",
                __FILE__, __LINE__, __FUNCTION__);
            break;
        }

#if ACL_EAGAIN == ACL_EWOULDBLOCK
        if (errno == ACL_EAGAIN || errno == ACL_EINTR) {
#else
        if (errno == ACL_EAGAIN || errno == ACL_EWOULDBLOCK
            || errno == ACL_EINTR) {
#endif
            continue;
        }

        acl_msg_error("%s(%d), %s: accept error: %s(%d, %d), maxfd: %d"
            ", lastfd: %d, listenfd: %d, stoping ...",
            __FILE__, __LINE__, __FUNCTION__,
            acl_last_serror(), errno, ACL_EAGAIN,
            __max_fd, __last_fd, ACL_VSTREAM_SOCK(sstream));

        acl_fiber_sleep(1);
    }

    acl_msg_info("%s(%d), %s: close listenfd=%d", __FILE__, __LINE__,
        __FUNCTION__, ACL_VSTREAM_SOCK(sstream));
    acl_vstream_close(sstream);
}

static void thread_fiber_monitor(ACL_FIBER *fiber acl_unused, void *ctx)
{
    FIBER_SERVER *server = (FIBER_SERVER *) ctx;
    static int dummy;
    int i;

    (void) acl_mbox_read(server->in, -1, NULL);

    // kill all accepting fibers of the current thread
    for (i = 0; i < server->socket_count; i++) {
        acl_fiber_kill(server->accepters[i]);
    }

    // response to the main thread
    (void) acl_mbox_send(server->out, &dummy);

    // waiting for the next command for stop fiber schedule
    (void) acl_mbox_read(server->in, -1, NULL);

    // notify fiber schedule to stop now
    acl_fiber_schedule_stop();
}

static void *thread_main(void *ctx)
{
    FIBER_SERVER *server =(FIBER_SERVER *) ctx;
    static int dummy;
    int i;

    if (__thread_init) {
        __thread_init(__thread_init_ctx);
    }

    // create accept fibers for each listening socket
    for (i = 0; i < server->socket_count; i++) {
        server->accepters[i] = acl_fiber_create(
            thread_fiber_accept, server->sstreams[i], STACK_SIZE);
    }

    // create monitor fiber waiting STOPPING command from main thread
    acl_fiber_create(thread_fiber_monitor, server, STACK_SIZE);

    // schedule the current thread fibers
    acl_fiber_schedule_with(__fiber_schedule_event);

    // got STOPPING from main thread and notify main thread
    (void) acl_mbox_send(server->out, &dummy);

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////

static ACL_FIBER     *__sighup_fiber = NULL;
static FIBER_SERVER **__servers = NULL;
static void server_free(FIBER_SERVER *server);

static int __exit_status = 0;

static void main_server_onexit(void)
{
    if (__service_onexit) {
        __service_onexit(__service_ctx);
        __service_onexit = NULL;
    }
}

static void main_server_exit(ACL_FIBER *fiber, int status)
{
#if !defined(_WIN32) && !defined(_WIN64)
    if (acl_var_fiber_disable_core_onexit) {
        acl_set_core_limit(0);
    }
#endif

    if (__sighup_fiber) {
        acl_fiber_kill(__sighup_fiber);
        __sighup_fiber = NULL;
    }

    acl_msg_info("%s(%d), %s: fiber-%u, service exit now!",
        __FILE__, __LINE__, __FUNCTION__, acl_fiber_id(fiber));

    /* stop the main thread fiber schedule proccess */
    acl_fiber_schedule_stop();
    acl_app_conf_unload();
    acl_debug_end();
    __exit_status = status;
}

static void main_stop_listen(void)
{
    static int dummy;
    int i;

    if (__server_stopping || __servers == NULL) {
        return;
    }

    for (i = 0; __servers[i] != NULL; i++) {
        (void) acl_mbox_send(__servers[i]->in, &dummy);
        (void) acl_mbox_read(__servers[i]->out, -1, NULL);
    }
}

static void main_stop_servers(void)
{
    static int dummy;
    int i;

    if (__server_stopping || __servers == NULL) {
        return;
    }

    __server_stopping = 1;

    for (i = 0; __servers[i] != NULL; i++) {
        (void) acl_mbox_send(__servers[i]->in, &dummy);
        (void) acl_mbox_read(__servers[i]->out, -1, NULL);
    }
}

static void main_free_servers(void)
{
    int i;
    for (i = 0; __servers[i] != NULL; i++) {
        server_free(__servers[i]);
    }

    if (acl_var_fiber_procname) {
        acl_myfree(acl_var_fiber_procname);
        acl_var_fiber_procname = NULL;
    }
    if (acl_var_fiber_log_file) {
        acl_myfree(acl_var_fiber_log_file);
        acl_var_fiber_log_file = NULL;
    }
    acl_myfree(__servers);
    __servers = NULL;
    acl_free_app_conf_str_table(__conf_str_tab);
}

#if !defined(_WIN32) && !defined(_WIN64)

static void main_fiber_monitor_master(ACL_FIBER *fiber, void *ctx)
{
    ACL_VSTREAM *stat_stream = (ACL_VSTREAM *) ctx;
    char  buf[8192];
    int   ret, n = 0;

    stat_stream->rw_timeout = -1;
    ret = acl_vstream_read(stat_stream, buf, sizeof(buf));
    acl_msg_info("%s(%d), %s: disconnect(%d) from acl_master, clients %lld",
        __FILE__, __LINE__, __FUNCTION__, ret,
        acl_atomic_clock_users(__clock));

    /* must close listening as quickly as possible, so that acl_master
    * can rebind the addr quickly.
    */
    acl_msg_info("%s(%d), %s: stopping listen ...",
        __FILE__, __LINE__, __FUNCTION__);
    main_stop_listen();
    acl_msg_info("%s(%d), %s: stop listen ok",
        __FILE__, __LINE__, __FUNCTION__);

    /* the user's callback will be called here */
    acl_msg_info("%s(%d), %s: calling server_onexit ...",
        __FILE__, __LINE__, __FUNCTION__);
    main_server_onexit();
    acl_msg_info("%s(%d), %s: call server_onexit ok",
        __FILE__, __LINE__, __FUNCTION__);

    while (!acl_var_fiber_quick_abort) {
        if (acl_atomic_clock_users(__clock) <= 0) {
            acl_msg_warn("%s(%d), %s: all clients closed!",
                __FILE__, __LINE__, __FUNCTION__);
            break;
        }

        acl_fiber_sleep(1);
        n++;

        if (acl_var_fiber_wait_limit > 0
            && n >= acl_var_fiber_wait_limit) {

            acl_msg_warn("%s(%d), %s: too long, clients: %lld",
                __FILE__, __LINE__, __FUNCTION__,
                acl_atomic_clock_users(__clock));
            break;
        }

        acl_msg_info("%s(%d), %s: waiting %d, clients %lld",
            __FILE__, __LINE__, __FUNCTION__, n,
            acl_atomic_clock_users(__clock));
    }

    /* then notify all threads' fiber schedule to stop */
    acl_msg_info("%s(%d), %s: stopping servers ...",
        __FILE__, __LINE__, __FUNCTION__);
    main_stop_servers();
    acl_msg_info("%s(%d), %s: stop servers ok",
        __FILE__, __LINE__, __FUNCTION__);

    acl_msg_info("%s(%d), %s: freeing servers ...",
        __FILE__, __LINE__, __FUNCTION__);
    main_free_servers();
    acl_msg_info("%s(%d), %s: free servers ok",
        __FILE__, __LINE__, __FUNCTION__);

    acl_msg_info("%s(%d), %s: call main_server_exit",
        __FILE__, __LINE__, __FUNCTION__);
    main_server_exit(fiber, 0);
}

static int main_dispatch_receive(int dispatch_fd)
{
    char  buf[256], remote[256], local[256];
    int   fd = -1, ret;
    ACL_VSTREAM *cstream;
    ACL_FIBER_ATTR attr;

    acl_fiber_attr_init(&attr);
    acl_fiber_attr_setstacksize(&attr, acl_var_fiber_stack_size);
    acl_fiber_attr_setsharestack(&attr, acl_var_fiber_share_stack ? 1 : 0);

    ret = acl_read_fd(dispatch_fd, buf, sizeof(buf) - 1, &fd);
    if (ret < 0 || fd < 0) {
        acl_msg_warn("%s(%d), %s: read from master_dispatch(%s) error",
            __FILE__, __LINE__, __FUNCTION__,
            acl_var_fiber_dispatch_addr);
        return -1;
    }

    buf[ret] = 0;

    cstream = acl_vstream_fdopen(fd, O_RDWR, acl_var_fiber_buf_size,
        acl_var_fiber_rw_timeout, ACL_VSTREAM_TYPE_SOCK);

    if (acl_getsockname(fd, local, sizeof(local)) == 0) {
        acl_vstream_set_local(cstream, local);
    }
    if (acl_getpeername(fd, remote, sizeof(remote)) == 0) {
        acl_vstream_set_peer(cstream, remote);
    }

    acl_fiber_create2(&attr, fiber_client, cstream);
    return 0;
}

static int main_dispatch_report(ACL_VSTREAM *conn)
{
    char buf[256];

    snprintf(buf, sizeof(buf), "count=%lld&used=%llu&pid=%u&type=%s"
        "&max_threads=%d&curr_threads=%d&busy_threads=%d&qlen=0\r\n",
        acl_atomic_clock_users(__clock),
        (unsigned long long) acl_atomic_clock_count(__clock),
        (unsigned) getpid(), acl_var_fiber_dispatch_type, 1, 1, 1);

    if (acl_vstream_writen(conn, buf, strlen(buf)) == ACL_VSTREAM_EOF) {
        acl_msg_warn("%s(%d), %s: write to master_dispatch(%s) failed",
            __FILE__, __LINE__, __FUNCTION__,
            acl_var_fiber_dispatch_addr);
        return -1;
    }

    return 0;
}

static void main_dispatch_poll(ACL_VSTREAM *conn)
{
    struct pollfd pfd;
    time_t last = time(NULL);

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = ACL_VSTREAM_SOCK(conn);
    pfd.events = POLLIN;

    while (!__server_stopping) {
        int n = poll(&pfd, 1, 1000);
        if (n < 0) {
            acl_msg_error("%s(%d), %s: poll error %s", __FILE__,
                __LINE__, __FUNCTION__, acl_last_serror());
            break;
        }

        if (n > 0 && pfd.revents & POLLIN) {
            if (main_dispatch_receive(ACL_VSTREAM_SOCK(conn)) < 0) {
                break;
            }

            pfd.revents = 0;
        }

        if (time(NULL) - last >= 1) {
            if (main_dispatch_report(conn) < 0) {
                break;
            }

            last = time(NULL);
        }
    }
}

static void main_fiber_dispatch(ACL_FIBER *fiber, void *ctx acl_unused)
{
    ACL_VSTREAM *conn;

    if (!acl_var_fiber_dispatch_addr || !*acl_var_fiber_dispatch_addr) {
        return;
    }

    while (!__server_stopping) {
        conn = acl_vstream_connect(acl_var_fiber_dispatch_addr,
                ACL_BLOCKING, 0, 0, 4096);
        if (conn == NULL) {
            acl_msg_warn("%s(%d), %s: connect %s error %s",
                __FILE__, __LINE__, __FUNCTION__,
                acl_var_fiber_dispatch_addr, acl_last_serror());
            acl_fiber_sleep(1);
            continue;
        }

        acl_msg_info("%s(%d), %s: connect %s ok",
            __FILE__, __LINE__, __FUNCTION__,
            acl_var_fiber_dispatch_addr);

        main_dispatch_poll(conn);
        acl_vstream_close(conn);
    }

    acl_msg_info("%s(%d), %s: fiber-%u exit now", __FILE__, __LINE__,
        __FUNCTION__, acl_fiber_id(fiber));
}

#endif

static void main_fiber_sighup(ACL_FIBER *fiber, void *ctx acl_unused)
{
    ACL_VSTRING *buf;

    while (1) {
        acl_fiber_sleep(1);

        if (acl_fiber_killed(fiber)) {
            break;
        }

        if (!__sighup_handler || !acl_var_server_gotsighup) {
            continue;
        }

        acl_var_server_gotsighup = 0;

        buf = acl_vstring_alloc(128);

#if !defined(_WIN32) && !defined(_WIN64)
        if (__sighup_handler(__service_ctx, buf) < 0) {
            acl_master_notify(acl_var_fiber_pid,
                __server_generation,
                ACL_MASTER_STAT_SIGHUP_ERR);
        } else {
            acl_master_notify(acl_var_fiber_pid,
                __server_generation,
                ACL_MASTER_STAT_SIGHUP_OK);
        }
#endif

        acl_vstring_free(buf);
    }
}

static void main_fiber_monitor_used(ACL_FIBER *fiber, void *ctx acl_unused)
{
    if (acl_var_fiber_use_limit <= 0) {
        acl_msg_warn("%s(%d), %s: invalid fiber_use_limit(%d)",
            __FILE__, __LINE__, __FUNCTION__,
            acl_var_fiber_use_limit);
        return;
    }

    while (1) {
        if (acl_atomic_clock_users(__clock) > 0) {
            acl_fiber_sleep(1);
            continue;
        }

        if (acl_atomic_clock_count(__clock) >=
            (unsigned) acl_var_fiber_use_limit) {
            break;
        }

        acl_fiber_sleep(1);
    }

    acl_msg_info("%s(%d), %s: use_limit reached %d",
        __FILE__, __LINE__, __FUNCTION__, acl_var_fiber_use_limit);

    main_stop_listen();
    main_server_onexit();
    main_stop_servers();
    main_free_servers();

    main_server_exit(fiber, 0);
}

static void main_fiber_monitor_idle(ACL_FIBER *fiber, void *ctx acl_unused)
{
    time_t last = time(NULL);

    while (1) {
        if (acl_atomic_clock_users(__clock) > 0) {
            acl_fiber_sleep(1);
            time(&last);
            continue;
        }

        if (time(NULL) - last >= acl_var_fiber_idle_limit) {
            break;
        }

        acl_fiber_sleep(1);
    }

    acl_msg_info("%s(%d), %s: idle %ld seconds, limit %d,"
        "users %lld, used %lld", __FILE__, __LINE__, __FUNCTION__,
        time(NULL) - last, acl_var_fiber_idle_limit,
        acl_atomic_clock_users(__clock),
        acl_atomic_clock_count(__clock));

    main_stop_listen();
    main_server_onexit();
    main_stop_servers();
    main_free_servers();

    main_server_exit(fiber, 0);
}

static void main_thread_loop(void)
{
#if !defined(_WIN32) && !defined(_WIN64)
    if (__daemon_mode) {
        ACL_VSTREAM *stat_stream = acl_vstream_fdopen(
                ACL_MASTER_STATUS_FD, O_RDWR, 8192, -1,
                ACL_VSTREAM_TYPE_SOCK);

        acl_fiber_create(main_fiber_monitor_master,
            stat_stream, STACK_SIZE);
        acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
        acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
        acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);
        if (acl_var_fiber_dispatch_addr && *acl_var_fiber_dispatch_addr) {
            acl_fiber_create(main_fiber_dispatch, NULL, STACK_SIZE);
        }
    }
#endif

    ACL_FIBER_ATTR attr;

    acl_fiber_attr_init(&attr);
    acl_fiber_attr_setstacksize(&attr, acl_var_fiber_stack_size);
    acl_fiber_attr_setsharestack(&attr, acl_var_fiber_share_stack ? 1 : 0);

    __sighup_fiber = acl_fiber_create2(&attr, main_fiber_sighup, NULL);

    if (acl_var_fiber_use_limit > 0) {
        acl_fiber_create(main_fiber_monitor_used, NULL, STACK_SIZE);
    }

    if (acl_var_fiber_idle_limit > 0) {
        acl_fiber_create(main_fiber_monitor_idle, NULL, STACK_SIZE);
    }

    acl_server_sighup_setup();
    acl_server_sigterm_setup();

    acl_msg_info("daemon started, log=%s, ev=%d", acl_var_fiber_log_file,
            __fiber_schedule_event);
    acl_fiber_schedule_with(__fiber_schedule_event);
    acl_msg_info("daemon stopped now, exit status=%d", __exit_status);
    exit(__exit_status);
}

static FIBER_SERVER *server_alloc(int socket_count, int fdtype)
{
    FIBER_SERVER *server = (FIBER_SERVER *)
        acl_mycalloc(1, sizeof(FIBER_SERVER));

    server->socket_count = socket_count;
    server->fdtype       = fdtype;
    server->in           = acl_mbox_create();
    server->out          = acl_mbox_create();

    server->sstreams  = (ACL_VSTREAM **)
        acl_mycalloc(socket_count, sizeof(ACL_VSTREAM *));
    server->accepters = (ACL_FIBER **)
        acl_mycalloc(socket_count, sizeof(ACL_FIBER *));

    return server;
}

static void server_free(FIBER_SERVER *server)
{
    acl_myfree(server->sstreams);
    acl_myfree(server->accepters);
    acl_mbox_free(server->in, NULL);
    acl_mbox_free(server->out, NULL);
    if (__clock) {
        acl_atomic_clock_free(__clock);
        __clock = NULL;
    }
    acl_myfree(server);
}

static FIBER_SERVER **servers_alloc(int nthreads, int socket_count, int fdtype)
{
    FIBER_SERVER **servers = (FIBER_SERVER **)
        acl_mycalloc(nthreads + 1, sizeof(FIBER_SERVER*));
    int i;

    for (i = 0; i < nthreads; i++) {
        servers[i] = server_alloc(socket_count, fdtype);
    }

    return servers;
}

#if !defined(_WIN32) && !defined(_WIN64)
static void server_daemon_open(FIBER_SERVER *server, int n)
{
    ACL_SOCKET fd = ACL_MASTER_LISTEN_FD, lfd;
    int i = 0;

    if (server->socket_count <= 0) {
        acl_msg_error("%s(%d), %s: invalid socket_count=%d",
            __FILE__, __LINE__, __FUNCTION__, server->socket_count);
        exit(6);
    }

    for (; fd < ACL_MASTER_LISTEN_FD + server->socket_count; fd++) {
        lfd = n == 0 ? fd : dup(fd);
        server->sstreams[i++] = acl_vstream_fdopen(lfd, O_RDWR,
            acl_var_fiber_buf_size, acl_var_fiber_rw_timeout,
            server->fdtype);
        acl_close_on_exec(lfd, ACL_CLOSE_ON_EXEC);
    }
}

static void servers_daemon(int count, int fdtype, int nthreads)
{
    int i;

    __servers = servers_alloc(nthreads, count, fdtype);

    for (i = 0; i < nthreads; i++) {
        server_daemon_open(__servers[i], i);
    }
}
#endif

static void server_open(FIBER_SERVER *server, ACL_ARGV *addrs)
{
    const char *myname = "server_open";
    ACL_ITER iter;
    unsigned flag = ACL_INET_FLAG_NONE;
    int i = 0;

    if (var_fiber_master_reuseport) {
        flag |= ACL_INET_FLAG_REUSEPORT;
    }

    acl_foreach(iter, addrs) {
        const char* addr = (const char*) iter.data;
        ACL_VSTREAM* sstream = acl_vstream_listen_ex(addr, 128,
                flag, 0, 0);
        if (sstream == NULL) {
            acl_msg_fatal("%s(%d): listen %s error(%s)",
                myname, __LINE__, addr, acl_last_serror());
        }

        acl_msg_info("%s: listen %s ok", myname, addr);

#if !defined(_WIN32) && !defined(_WIN64)
        acl_close_on_exec(ACL_VSTREAM_SOCK(sstream), ACL_CLOSE_ON_EXEC);
#endif
        server->sstreams[i++] = sstream;
    }
}

static void servers_open(const char *addrs, int fdtype, int nthreads)
{
    const char *pri = !strcmp(acl_var_fiber_master_private, "y") ?
        "private" : "public";
    char *unix_path = acl_concatenate(acl_var_fiber_queue_dir, "/",
            pri, NULL);
    ACL_ARGV* tokens = acl_search_addrs(addrs, unix_path);
    int i;

    acl_myfree(unix_path);

    if (tokens == NULL) {
        acl_msg_fatal("%s(%d), %s: can't find valid addrs from %s",
            __FILE__, __LINE__, __FUNCTION__, addrs);
    }

    __servers = servers_alloc(nthreads, tokens->argc, fdtype);

    for (i = 0; i < nthreads; i++) {
        server_open(__servers[i], tokens);
    }

    acl_argv_free(tokens);
}

static void servers_start(FIBER_SERVER **servers, int nthreads)
{
    acl_pthread_attr_t attr;
    int i;

    if (nthreads <= 0) {
        acl_msg_fatal("%s(%d), %s: invalid nthreads %d",
            __FILE__, __LINE__, __FUNCTION__, nthreads);
    }

    /* this can only be called in the main thread */
    if (__server_on_listen) {
        for (i = 0; i < nthreads; i++) {
            FIBER_SERVER *server = servers[i];
            int j;

            for (j = 0; j < server->socket_count; j++) {
                __server_on_listen(__service_ctx,
                    server->sstreams[j]);
            }
        }
    }

    __clock = acl_atomic_clock_alloc();
    acl_pthread_attr_init(&attr);
    acl_pthread_attr_setdetachstate(&attr, ACL_PTHREAD_CREATE_DETACHED);

    for (i = 0; i < nthreads; i++) {
        acl_pthread_create(&servers[i]->tid, &attr,
            thread_main, servers[i]);
    }

    main_thread_loop();
}

static void open_service_log(void)
{
#if !defined(_WIN32) && !defined(_WIN64)
    /* first, close the master's log */
    master_log_close();
#endif

    /* second, open the service's log */
    if (acl_master_log_enabled()) {
        acl_msg_open(acl_var_fiber_log_file, acl_var_fiber_procname);
    }

    if (acl_var_fiber_log_debug && *acl_var_fiber_log_debug
        && acl_var_fiber_max_debug >= 100) {

        acl_debug_init2(acl_var_fiber_log_debug,
            acl_var_fiber_max_debug);
    }
}

static void server_init(const char *procname)
{
    const char *myname = "server_init";
    static int inited = 0;
    const char* ptr;

    if (inited) {
        return;
    }

    inited = 1;

    /* Don't die when a process goes away unexpectedly. */
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Don't die for frivolous reasons. */
#ifdef SIGXFSZ
    signal(SIGXFSZ, SIG_IGN);
#endif

    /* May need this every now and then. */
#if defined(_WIN32) || defined(_WIN64)
    acl_var_fiber_pid = _getpid();
#else
    acl_var_fiber_pid = getpid();
#endif
    acl_var_fiber_procname = acl_mystrdup(acl_safe_basename(procname));

    ptr = acl_getenv("SERVICE_LOG");
    if ((ptr = acl_getenv("SERVICE_LOG")) != NULL && *ptr != 0) {
        acl_var_fiber_log_file = acl_mystrdup(ptr);
    } else {
        acl_var_fiber_log_file = acl_mystrdup("acl_master.log");
        acl_msg_info("%s(%d)->%s: can't get SERVICE_LOG's env value,"
            " use %s log", __FILE__, __LINE__, myname,
            acl_var_fiber_log_file);
    }

    acl_get_app_conf_int_table(__conf_int_tab);
    acl_get_app_conf_str_table(__conf_str_tab);
    acl_get_app_conf_bool_table(__conf_bool_tab);

#define EQ !strcasecmp
    if (EQ(acl_var_fiber_master_reuseport, "yes")
        || EQ(acl_var_fiber_master_reuseport, "true")
        || EQ(acl_var_fiber_master_reuseport, "on")) {

        var_fiber_master_reuseport = 1;
    }

    if (__deny_info == NULL) {
        __deny_info = acl_var_fiber_deny_banner;
    }
    if (acl_var_fiber_access_allow && *acl_var_fiber_access_allow) {
        acl_access_add(acl_var_fiber_access_allow, ", \t", ":");
    }
}

static void usage(int argc, char * argv[])
{
    int   i;
    const char *service_name;

    if (argc <= 0) {
        acl_msg_fatal("%s(%d): argc(%d) invalid",
            __FILE__, __LINE__, argc);
    }

    service_name = acl_safe_basename(argv[0]);

    for (i = 0; i < argc; i++) {
        acl_msg_info("argv[%d]: %s", i, argv[i]);
    }

    acl_msg_info("usage: %s -H[help]"
        " -c [use chroot]"
        " -n service_name"
        " -s socket_count"
        " -t transport"
        " -u [use setgid initgroups setuid]"
        " -f conf_file"
        " -L listen_addrs",
        service_name);
}

static int __first_name;
static va_list __ap_dest;
static ACL_MASTER_SERVER_INIT_FN pre_jail = NULL;
static ACL_MASTER_SERVER_INIT_FN post_init = NULL;

static void parse_args(void)
{
    const char *myname = "parse_args";
    int name = __first_name;

    for (; name != ACL_APP_CTL_END; name = va_arg(__ap_dest, int)) {
        switch (name) {
        case ACL_MASTER_SERVER_BOOL_TABLE:
            acl_get_app_conf_bool_table(
                va_arg(__ap_dest, ACL_CONFIG_BOOL_TABLE *));
            break;
        case ACL_MASTER_SERVER_INT_TABLE:
            acl_get_app_conf_int_table(
                va_arg(__ap_dest, ACL_CONFIG_INT_TABLE *));
            break;
        case ACL_MASTER_SERVER_INT64_TABLE:
            acl_get_app_conf_int64_table(
                va_arg(__ap_dest, ACL_CONFIG_INT64_TABLE *));
            break;
        case ACL_MASTER_SERVER_STR_TABLE:
            acl_get_app_conf_str_table(
                va_arg(__ap_dest, ACL_CONFIG_STR_TABLE *));
            break;
        case ACL_MASTER_SERVER_PRE_INIT:
            pre_jail = va_arg(__ap_dest, ACL_MASTER_SERVER_INIT_FN);
            break;
        case ACL_MASTER_SERVER_POST_INIT:
            post_init = va_arg(__ap_dest, ACL_MASTER_SERVER_INIT_FN);
            break;
        case ACL_MASTER_SERVER_ON_LISTEN:
            __server_on_listen = va_arg(__ap_dest,
                ACL_MASTER_SERVER_ON_LISTEN_FN);
            break;
        case ACL_MASTER_SERVER_EXIT:
            __service_onexit =
                va_arg(__ap_dest, ACL_MASTER_SERVER_EXIT_FN);
            break;
        case ACL_MASTER_SERVER_THREAD_INIT:
            __thread_init = va_arg(__ap_dest,
                ACL_MASTER_SERVER_THREAD_INIT_FN);
            break;
        case ACL_MASTER_SERVER_THREAD_INIT_CTX:
            __thread_init_ctx = va_arg(__ap_dest, void *);
            break;
        case ACL_MASTER_SERVER_DENY_INFO:
            __deny_info =
                acl_mystrdup(va_arg(__ap_dest, const char*));
            break;
        case ACL_MASTER_SERVER_SIGHUP:
            __sighup_handler =
                va_arg(__ap_dest, ACL_MASTER_SERVER_SIGHUP_FN);
            break;
        default:
            acl_msg_fatal("%s: bad name(%d)", myname, name);
            break;
        }
    }
}

static void fiber_log_writer(void *, const char *fmt, va_list ap)
{
    va_list tmp;
    va_copy(tmp, ap);

    acl::string buf;
    buf.vformat(fmt, tmp);
    //acl_msg_info("%s", buf.c_str());
    printf("%s\r\n", buf.c_str());
}

static void hook_fiber_log(void)
{
#if !defined(_WIN32) && !defined(_WIN64)
    ACL_ARRAY *loggers = acl_log_get_streams();
    ACL_ITER iter;

    if (loggers) {
        acl_foreach(iter, loggers) {
            ACL_VSTREAM *fp = (ACL_VSTREAM*) iter.data;
            acl_fiber_set_sysio(ACL_VSTREAM_FILE(fp));
        }

        acl_log_free_streams(loggers);
    }
    // If hook flag been set, the log of fiber module will be
    // written to the current log file.
    acl_fiber_msg_register(fiber_log_writer, NULL);
#endif
}

void acl_fiber_server_main(int argc, char *argv[],
    void (*service)(void*, ACL_VSTREAM*), void *ctx, int name, ...)
{
    const char *myname = __FUNCTION__;
    const char *service_name = acl_safe_basename(argv[0]);
    const char *root_dir = NULL, *user = NULL, *addrs = NULL;
    int   c, socket_count = 1, fdtype = ACL_VSTREAM_TYPE_LISTEN;
#if !defined(_WIN32) && !defined(_WIN64)
    char *generation;
#endif
    va_list ap;

    /* Set up call-back info. */
    __service     = service;
    __service_ctx = ctx;
    __first_name  = name;

    va_start(ap, name);

#if defined(_WIN32) || defined(_WIN64)
# if  _MSC_VER >= 1911
    va_copy(__ap_dest, ap);
# else
    __ap_dest = ap;
# endif
#else
    va_copy(__ap_dest, ap);
    master_log_open(argv[0]);
#endif
    va_end(ap);

    __conf_file[0] = 0;

#ifndef __APPLE__
# if !defined(_WIN32) && !defined(_WIN64)
    opterr = 0;
# endif
    optind = 0;
    optarg = 0;
#endif  // __APPLE__

#if defined(_WIN32) || defined(_WIN64)
    if (winapi_hook()) {
        acl_msg_info("hook Win API ok");
    } else {
        acl_msg_error("hook Win API error: %s", acl_last_serror());
    }
#endif

    while ((c = getopt(argc, argv, "Hc:n:s:t:uf:L:")) > 0) {
        switch (c) {
        case 'H':
            usage(argc, argv);
            exit (0);
        case 'f':
            acl_app_conf_load(optarg);
#if defined(_WIN32) || defined(_WIN64)
            _snprintf(__conf_file, sizeof(__conf_file), "%s", optarg);
#else
            snprintf(__conf_file, sizeof(__conf_file), "%s", optarg);
#endif
            break;
        case 'c':
            root_dir = "setme";
            break;
        case 'n':
            service_name = optarg;
            break;
        case 's':
            socket_count = atoi(optarg);
            break;
        case 'u':
            user = "setme";
            break;
        case 't':
            /* deprecated, just go through */
            break;
        case 'L':
            addrs = optarg;
            break;
        default:
            break;
        }
    }

    if (__conf_file[0] == 0) {
        acl_msg_info("%s(%d), %s: no configure file",
            __FILE__, __LINE__, myname);
    } else {
        acl_msg_info("%s(%d), %s: configure file=%s", 
            __FILE__, __LINE__, myname, __conf_file);
    }

    ACL_SAFE_STRNCPY(__service_name, service_name, sizeof(__service_name));

#if defined(_WIN32) || defined(_WIN64)
    __daemon_mode = 0;
#else
    if (isatty(STDIN_FILENO)) {
        __daemon_mode = 0;
    } else {
        const char *mode = getenv("MASTER_SERVICE");
        if (mode && strcasecmp(mode, "ALONE") == 0) {
            __daemon_mode = 0;
        } else {
            __daemon_mode = 1;
        }
    }
#endif

    /*******************************************************************/

    /* Application-specific initialization. */

    /* load configure, set signal */

    server_init(argv[0]);

    parse_args();

#if defined(_WIN32) || defined(_WIN64)
    servers_open(addrs, fdtype, acl_var_fiber_threads);
#else
    if (root_dir) {
        root_dir = acl_var_fiber_queue_dir;
    }

    if (user) {
        user = acl_var_fiber_owner;
    }

    /* Retrieve process generation from environment. */
    if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
        if (!acl_alldig(generation)) {
            acl_msg_fatal("bad generation: %s", generation);
        }
        sscanf(generation, "%o", &__server_generation);
        if (acl_msg_verbose) {
            acl_msg_info("process generation: %s (%o)",
                generation, __server_generation);
        }
    }

    /*******************************************************************/

    /* Run pre-jail initialization. */
    if (__daemon_mode && *acl_var_fiber_queue_dir
        && chdir(acl_var_fiber_queue_dir) < 0) {

        acl_msg_fatal("chdir(\"%s\"): %s", acl_var_fiber_queue_dir,
            acl_last_serror());
    }

    /* create static servers object */

    if (__daemon_mode == 0) {
        if (addrs == NULL || *addrs == 0) {
            addrs = acl_var_fiber_master_service;
        }
        assert(addrs && *addrs);
        servers_open(addrs, fdtype, acl_var_fiber_threads);
    } else if (var_fiber_master_reuseport) {
        assert(acl_var_fiber_master_service);
        assert(*acl_var_fiber_master_service);
        addrs = acl_var_fiber_master_service;

        servers_open(addrs, fdtype, acl_var_fiber_threads);
    } else if (socket_count > 0) {
        servers_daemon(socket_count, fdtype, acl_var_fiber_threads);
    } else {
        acl_msg_fatal("%s(%d): invalid socket_count: %d",
            myname, __LINE__, socket_count);
    }
#endif // !_WIN32 && !_WIN64

    acl_assert(__servers);

    if (pre_jail) {
        pre_jail(__service_ctx);
    }

#if !defined(_WIN32) && !defined(_WIN64)
    if (user && *user) {
        acl_chroot_uid(root_dir, user);
    }
#endif

    /* open the server's log */
    open_service_log();

#if !defined(_WIN32) && !defined(_WIN64)
    /* if enable dump core when program crashed ? */
    if (acl_var_fiber_enable_core && acl_var_fiber_core_limit != 0) {
        acl_set_core_limit(acl_var_fiber_core_limit);
    }
#endif

    /* Run post-jail initialization. */
    if (post_init) {
        post_init(__service_ctx);
    }

    if (strcasecmp(acl_var_fiber_schedule_event, "kernel") == 0) {
        __fiber_schedule_event = FIBER_EVENT_KERNEL;
    } else if (strcasecmp(acl_var_fiber_schedule_event, "poll") == 0) {
        __fiber_schedule_event = FIBER_EVENT_POLL;
    } else if (strcasecmp(acl_var_fiber_schedule_event, "select") == 0) {
        __fiber_schedule_event = FIBER_EVENT_SELECT;
    } else if (strcasecmp(acl_var_fiber_schedule_event, "wmsg") == 0) {
        __fiber_schedule_event = FIBER_EVENT_WMSG;
    } else if (strcasecmp(acl_var_fiber_schedule_event, "io_uring") == 0) {
        __fiber_schedule_event = FIBER_EVENT_IO_URING;
    } else {
        __fiber_schedule_event = FIBER_EVENT_KERNEL;
    }

    acl_msg_info("schedule event type - %s", acl_var_fiber_schedule_event);

    if (acl_var_fiber_hook_log) {
        hook_fiber_log();
    }

#if !defined(_WIN32) && !defined(_WIN64)
    /* notify master that child started ok */
    if (__daemon_mode) {
        acl_master_notify(acl_var_fiber_pid, __server_generation,
            ACL_MASTER_STAT_START_OK);
    }
#endif
    servers_start(__servers, acl_var_fiber_threads);
}
