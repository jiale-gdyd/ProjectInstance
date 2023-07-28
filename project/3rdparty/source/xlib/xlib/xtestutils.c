#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/resource.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xrand.h>
#include <xlib/xlib/xmain.h>
#include <xlib/xlib/xtypes.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xtimer.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xspawn.h>
#include <xlib/xlib/xpattern.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xlib-private.h>
#include <xlib/xlib/xutilsprivate.h>

XLIB_VAR char *__xlib_assert_msg;
char *__xlib_assert_msg = NULL;

#define X_TEST_STATUS_TIMED_OUT             1024
#define TAP_SUBTEST_PREFIX                  "#    "
#define TAP_VERSION                         X_STRINGIFY(13)

struct XTestCase {
    xchar *name;
    xuint fixture_size;
    void (*fixture_setup)(void *, xconstpointer);
    void (*fixture_test)(void *, xconstpointer);
    void (*fixture_teardown)(void *, xconstpointer);
    xpointer test_data;
};

struct XTestSuite {
    xchar  *name;
    XSList *suites;
    XSList *cases;
};

typedef struct XestroyEntry XestroyEntry;
struct XestroyEntry {
    XestroyEntry   *next;
    XDestroyNotify destroy_func;
    xpointer       destroy_data;
};

static void test_cleanup(void);
static void test_trap_clear(void);
static void test_run_seed(const xchar *rseed);
static xuint8 *x_test_log_dump(XTestLogMsg *msg, xuint *len);
static void xtest_default_log_handler(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer unused_data);
static void x_test_tap_print(unsigned subtest_level, xboolean commented, const char *format, ...) X_GNUC_PRINTF(3, 4);

static const char *const x_test_result_names[] = {
    "OK",
    "SKIP",
    "FAIL",
    "TODO"
};

static int test_log_fd = -1;
static xboolean test_mode_fatal = TRUE;
static xboolean x_test_run_once = TRUE;
static xboolean test_isolate_dirs = FALSE;
static xchar *test_isolate_dirs_tmpdir = NULL;
static const xchar *test_tmpdir = NULL;
static xboolean test_run_list = FALSE;
static xchar *test_run_seedstr = NULL;
X_LOCK_DEFINE_STATIC(test_run_rand);
static XRand *test_run_rand = NULL;
static xchar *test_run_name = "";
static xchar *test_run_name_path = "";
static XSList **test_filename_free_list;
static xuint test_run_forks = 0;
static xuint test_run_count = 0;
static xuint test_count = 0;
static xuint test_skipped_count = 0;
static XTestResult test_run_success = X_TEST_RUN_FAILURE;
static xchar *test_run_msg = NULL;
static xuint test_startup_skip_count = 0;
static XTimer *test_user_timer = NULL;
static double test_user_stamp = 0;
static XSList *test_paths = NULL;
static xboolean test_prefix = FALSE;
static xboolean test_prefix_extended = FALSE;
static XSList *test_paths_skipped = NULL;
static xboolean test_prefix_skipped = FALSE;
static xboolean test_prefix_extended_skipped = FALSE;
static XTestSuite *test_suite_root = NULL;
static int test_trap_last_status = 0;
static XPid test_trap_last_pid = 0;
static char *test_trap_last_subprocess = NULL;
static char *test_trap_last_stdout = NULL;
static char *test_trap_last_stderr = NULL;
static char *test_uri_base = NULL;
static xboolean test_debug_log = FALSE;
static xboolean test_tap_log = TRUE;
static xboolean test_nonfatal_assertions = FALSE;
static XestroyEntry *test_destroy_queue = NULL;
static const char *test_argv0 = NULL;
static char *test_argv0_dirname = NULL;
static const char *test_disted_files_dir;
static const char *test_built_files_dir;
static char *test_initial_cwd = NULL;
static xboolean test_in_forked_child = FALSE;
static xboolean test_in_subprocess = FALSE;
static xboolean test_is_subtest = FALSE;

static XTestConfig mutable_test_config_vars = {
    FALSE,
    TRUE,
    FALSE,
    FALSE,
    FALSE,
    TRUE,
};
const XTestConfig *const x_test_config_vars = &mutable_test_config_vars;
static xboolean no_g_set_prgname = FALSE;
static XPrintFunc x_default_print_func = NULL;

enum {
    X_TEST_CASE_LARGS_RESULT         = 0,
    X_TEST_CASE_LARGS_RUN_FORKS      = 1,
    X_TEST_CASE_LARGS_EXECUTION_TIME = 2,
    X_TEST_CASE_LARGS_MAX
};

static inline xboolean is_subtest(void)
{
    return test_is_subtest || test_in_forked_child || test_in_subprocess;
}

static void x_test_print_handler_full(const xchar *string, xboolean use_tap_format, xboolean is_tap_comment, unsigned subtest_level)
{
    x_assert(string != NULL);

    if (X_LIKELY(use_tap_format) && (strchr(string, '\n') != NULL)) {
        const char *line = string;
        static xboolean last_had_final_newline = TRUE;
        XString *output =x_string_new_len(NULL, strlen(string) + 2);

        do {
            const char *next = strchr (line, '\n');

            if (last_had_final_newline && (next || (*line != '\0'))) {
                for (unsigned l = 0; l < subtest_level; ++l) {
                    x_string_append(output, TAP_SUBTEST_PREFIX);
                }

                if X_LIKELY(is_tap_comment) {
                    x_string_append(output, "# ");
                }
            }

            if (next) {
                next += 1;
                x_string_append_len(output, line, next - line);
            } else {
                x_string_append(output, line);
                last_had_final_newline = (*line == '\0');
            }

            line = next;
        } while (line != NULL);

        x_default_print_func(output->str);
        x_string_free(x_steal_pointer(&output), TRUE);
    } else {
        x_default_print_func(string);
    }
}

static void x_test_print_handler(const xchar *string)
{
    x_test_print_handler_full(string, test_tap_log, TRUE, is_subtest() ? 1 : 0);
}

static void x_test_tap_print(unsigned subtest_level, xboolean commented, const char *format, ...)
{
    va_list args;
    char *string;

    va_start(args, format);
    string = x_strdup_vprintf(format, args);
    va_end(args);

    x_test_print_handler_full(string, TRUE, commented, subtest_level);
    x_free(string);
}

const char *x_test_log_type_name(XTestLogType log_type)
{
    switch (log_type) {
        case X_TEST_LOG_NONE:         return "none";
        case X_TEST_LOG_ERROR:        return "error";
        case X_TEST_LOG_START_BINARY: return "binary";
        case X_TEST_LOG_LIST_CASE:    return "list";
        case X_TEST_LOG_SKIP_CASE:    return "skip";
        case X_TEST_LOG_START_CASE:   return "start";
        case X_TEST_LOG_STOP_CASE:    return "stop";
        case X_TEST_LOG_MIN_RESULT:   return "minperf";
        case X_TEST_LOG_MAX_RESULT:   return "maxperf";
        case X_TEST_LOG_MESSAGE:      return "message";
        case X_TEST_LOG_START_SUITE:  return "start suite";
        case X_TEST_LOG_STOP_SUITE:   return "stop suite";
    }

    return "???";
}

static void x_test_log_send(xuint n_bytes, const xuint8 *buffer)
{
    if (test_log_fd >= 0) {
        int r;
        do {
            r = write(test_log_fd, buffer, n_bytes);
        } while (r < 0 && errno == EINTR);
    }

    if (test_debug_log) {
        xuint ui;
        XString *output;
        XTestLogMsg *msg;
        XTestLogBuffer *lbuffer = x_test_log_buffer_new();

        x_test_log_buffer_push(lbuffer, n_bytes, buffer);
        msg = x_test_log_buffer_pop(lbuffer);
        x_warn_if_fail(msg != NULL);
        x_warn_if_fail(lbuffer->data->len == 0);
        x_test_log_buffer_free(lbuffer);

        output = x_string_new(NULL);
        x_string_printf(output, "{*LOG(%s)", x_test_log_type_name(msg->log_type));
        for (ui = 0; ui < msg->n_strings; ui++) {
            x_string_append_printf(output, ":{%s}", msg->strings[ui]);
        }

        if (msg->n_nums) {
            x_string_append(output, ":(");
            for (ui = 0; ui < msg->n_nums; ui++) {
                if ((long double)(long)msg->nums[ui] == msg->nums[ui]) {
                    x_string_append_printf(output, "%s%ld", ui ? ";" : "", (long)msg->nums[ui]);
                } else {
                    x_string_append_printf(output, "%s%.16g", ui ? ";" : "", (double)msg->nums[ui]);
                }
            }
            x_string_append_c(output, ')');
        }

        x_string_append(output, ":LOG*}");
        x_printerr("%s\n", output->str);
        x_string_free(output, TRUE);
        x_test_log_msg_free(msg);
    }
}

static void x_test_log(XTestLogType lbit, const xchar *string1, const xchar *string2, xuint n_args, long double *largs)
{
    xboolean fail;
    xdouble timing;
    XTestLogMsg msg;
    xuint8 *dbuffer;
    XTestResult result;
    xuint32 dbufferlen;
    unsigned subtest_level;
    xchar *astrings[3] = { NULL, NULL, NULL };

    if (x_once_init_enter(&x_default_print_func)) {
        x_once_init_leave(&x_default_print_func, x_set_print_handler(x_test_print_handler));
        x_assert_nonnull(x_default_print_func);
    }

    subtest_level = is_subtest() ? 1 : 0;

    switch (lbit) {
        case X_TEST_LOG_START_BINARY:
            if (test_tap_log) {
                if (!is_subtest()) {
                    x_test_tap_print(0, FALSE, "TAP version " TAP_VERSION "\n");
                } else {
                    x_test_tap_print(subtest_level > 0 ? subtest_level - 1 : 0, TRUE, "Subtest: %s\n", test_argv0);
                }

                x_print("random seed: %s\n", string2);
            } else if (x_test_verbose()) {
                x_print("XTest: random seed: %s\n", string2);
            }
            break;

        case X_TEST_LOG_START_SUITE:
            if (test_tap_log) {
                if (string1[0] != 0) {
                    x_print("Start of %s tests\n", string1);
                } else if (test_paths == NULL) {
                    x_test_tap_print(subtest_level, FALSE, "1..%d\n", test_count);
                }
            }
            break;

        case X_TEST_LOG_STOP_SUITE:
            if (test_tap_log) {
                if (string1[0] != 0) {
                    x_print("End of %s tests\n", string1);
                } else if (test_paths != NULL) {
                    x_test_tap_print(subtest_level, FALSE, "1..%d\n", test_run_count);
                }
            }
            break;

        case X_TEST_LOG_STOP_CASE:
            result = (XTestResult)largs[X_TEST_CASE_LARGS_RESULT];
            timing = largs[X_TEST_CASE_LARGS_EXECUTION_TIME];
            fail = result == X_TEST_RUN_FAILURE;
            if (test_tap_log) {
                XString *tap_output;

                if (fail || result == X_TEST_RUN_INCOMPLETE) {
                    tap_output = x_string_new("not ok");
                } else {
                    tap_output = x_string_new("ok");
                }

                if (is_subtest()) {
                    x_string_prepend(tap_output, TAP_SUBTEST_PREFIX);
                }

                x_string_append_printf(tap_output, " %d %s", test_run_count, string1);
                if (result == X_TEST_RUN_INCOMPLETE) {
                    x_string_append_printf(tap_output, " # TODO %s", string2 ? string2 : "");
                } else if (result == X_TEST_RUN_SKIPPED) {
                    x_string_append_printf(tap_output, " # SKIP %s", string2 ? string2 : "");
                } else if (result == X_TEST_RUN_FAILURE && string2 != NULL) {
                    x_string_append_printf(tap_output, " - %s", string2);
                }

                x_string_append_c(tap_output, '\n');
                x_default_print_func(tap_output->str);
                x_string_free(x_steal_pointer(&tap_output), TRUE);

                if (timing > 0.5) {
                    tap_output = x_string_new("# ");
                    x_string_append_printf(tap_output, "slow test %s executed in %0.2lf secs\n", string1, timing);
                    x_default_print_func(tap_output->str);
                    x_string_free(x_steal_pointer(&tap_output), TRUE);
                }
            } else if (x_test_verbose()) {
                x_print("XTest: result: %s\n", x_test_result_names[result]);
            } else if (!x_test_quiet() && !test_in_subprocess) {
                x_print("%s\n", x_test_result_names[result]);
            }

            if (fail && test_mode_fatal) {
                if (test_tap_log) {
                    x_test_tap_print(0, FALSE, "Bail out!\n");
                }

                x_abort();
            }

            if (result == X_TEST_RUN_SKIPPED || result == X_TEST_RUN_INCOMPLETE) {
                test_skipped_count++;
            }
            break;

        case X_TEST_LOG_SKIP_CASE:
            if (test_tap_log) {
                x_test_tap_print(subtest_level, FALSE, "ok %d %s # SKIP\n", test_run_count, string1);
            }
            break;

        case X_TEST_LOG_MIN_RESULT:
            if (test_tap_log) {
                x_print("min perf: %s\n", string1);
            } else if (x_test_verbose()) {
                x_print("(MINPERF:%s)\n", string1);
            }
            break;

        case X_TEST_LOG_MAX_RESULT:
            if (test_tap_log) {
                x_print("max perf: %s\n", string1);
            } else if (x_test_verbose()) {
                x_print("(MAXPERF:%s)\n", string1);
            }
            break;

        case X_TEST_LOG_MESSAGE:
            if (test_tap_log) {
                x_print("%s\n", string1);
            } else if (x_test_verbose()) {
                x_print("(MSG: %s)\n", string1);
            }
            break;

        case X_TEST_LOG_ERROR:
            if (test_tap_log) {
                char *message = x_strdup(string1);
                if (message) {
                    char *line = message;
                    while ((line = strchr(line, '\n'))) {
                        *(line++) = ' ';
                    }

                    message = x_strstrip(message);
                }

                if (test_run_name && *test_run_name != '\0') {
                    if (message && *message != '\0') {
                        x_test_tap_print(subtest_level, FALSE, "not ok %s - %s\n", test_run_name, message);
                    } else {
                        x_test_tap_print(subtest_level, FALSE, "not ok %s\n", test_run_name);
                    }

                    x_clear_pointer(&message, x_free);
                }

                if (message && *message != '\0') {
                    x_test_tap_print(subtest_level, FALSE, "Bail out! %s\n", message);
                } else {
                    x_test_tap_print(subtest_level, FALSE, "Bail out!\n");
                }

                x_free(message);
            } else if (x_test_verbose()) {
                x_print("(ERROR: %s)\n", string1);
            }
            break;

        default:;
    }

    msg.log_type = lbit;
    msg.n_strings = (string1 != NULL) + (string1 && string2);
    msg.strings = astrings;
    astrings[0] = (xchar*) string1;
    astrings[1] = astrings[0] ? (xchar*) string2 : NULL;
    msg.n_nums = n_args;
    msg.nums = largs;

    dbuffer = x_test_log_dump(&msg, &dbufferlen);
    x_test_log_send(dbufferlen, dbuffer);
    x_free(dbuffer);

    switch (lbit) {
        case X_TEST_LOG_START_CASE:
            if (test_tap_log) {
                ;
            } else if (x_test_verbose()) {
                x_print("GTest: run: %s\n", string1);
            } else if (!x_test_quiet()) {
                x_print("%s: ", string1);
            }
            break;

        default:;
    }
}

void x_test_disable_crash_reporting(void)
{
#ifdef HAVE_SYS_RESOURCE_H
    struct rlimit limit = { 0, 0 };
    (void)setrlimit(RLIMIT_CORE, &limit);
#endif

#if defined(PR_SET_DUMPABLE)
    (void)prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
#endif
}

static void parse_args(xint *argc_p, xchar ***argv_p)
{
    xuint i, e;
    xuint argc = *argc_p;
    xchar **argv = *argv_p;

    test_argv0 = argv[0];
    test_initial_cwd = x_get_current_dir();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--x-fatal-warnings") == 0) {
            XLogLevelFlags fatal_mask = (XLogLevelFlags)x_log_set_always_fatal((XLogLevelFlags)X_LOG_FATAL_MASK);
            fatal_mask = (XLogLevelFlags)(fatal_mask | X_LOG_LEVEL_WARNING | X_LOG_LEVEL_CRITICAL);
            x_log_set_always_fatal(fatal_mask);
            argv[i] = NULL;
        } else if (strcmp (argv[i], "--keep-going") == 0 || strcmp (argv[i], "-k") == 0) {
            test_mode_fatal = FALSE;
            argv[i] = NULL;
        } else if (strcmp (argv[i], "--debug-log") == 0) {
            test_debug_log = TRUE;
            argv[i] = NULL;
        } else if (strcmp (argv[i], "--tap") == 0) {
            test_tap_log = TRUE;
            argv[i] = NULL;
        } else if (strcmp ("--XTestLogFD", argv[i]) == 0 || strncmp ("--XTestLogFD=", argv[i], 13) == 0) {
            xchar *equal = argv[i] + 12;
            if (*equal == '=') {
                test_log_fd = x_ascii_strtoull(equal + 1, NULL, 0);
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_log_fd = x_ascii_strtoull(argv[i], NULL, 0);
            }

            argv[i] = NULL;
            test_tap_log = FALSE;
        } else if (strcmp ("--XTestSkipCount", argv[i]) == 0 || strncmp ("--XTestSkipCount=", argv[i], 17) == 0) {
            xchar *equal = argv[i] + 16;
            if (*equal == '=') {
                test_startup_skip_count = x_ascii_strtoull(equal + 1, NULL, 0);
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_startup_skip_count = x_ascii_strtoull(argv[i], NULL, 0);
            }

            argv[i] = NULL;
        } else if (strcmp ("--XTestSubprocess", argv[i]) == 0) {
            test_in_subprocess = TRUE;
            x_test_disable_crash_reporting();
            argv[i] = NULL;
            test_tap_log = FALSE;
        } else if (strcmp("-p", argv[i]) == 0 || strncmp("-p=", argv[i], 3) == 0) {
            xchar *equal = argv[i] + 2;
            if (*equal == '=') {
                test_paths = x_slist_prepend(test_paths, equal + 1);
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_paths = x_slist_prepend(test_paths, argv[i]);
            }

            argv[i] = NULL;
            if (test_prefix_extended) {
                printf ("do not mix [-r | --run-prefix] with '-p'\n");
                exit (1);
            }

            test_prefix = TRUE;
        } else if (strcmp("-r", argv[i]) == 0 || strncmp("-r=", argv[i], 3) == 0 || strcmp("--run-prefix", argv[i]) == 0 || strncmp("--run-prefix=", argv[i], 13) == 0) {
            xchar *equal = argv[i] + 2;
            if (*equal == '=') {
                test_paths = x_slist_prepend(test_paths, equal + 1);
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_paths = x_slist_prepend(test_paths, argv[i]);
            }

            argv[i] = NULL;
            if (test_prefix) {
                printf("do not mix [-r | --run-prefix] with '-p'\n");
                exit(1);
            }

            test_prefix_extended = TRUE;
        } else if (strcmp("-s", argv[i]) == 0 || strncmp("-s=", argv[i], 3) == 0) {
            xchar *equal = argv[i] + 2;
            if (*equal == '=') {
                test_paths_skipped = x_slist_prepend(test_paths_skipped, equal + 1);
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_paths_skipped = x_slist_prepend(test_paths_skipped, argv[i]);
            }

            argv[i] = NULL;
            if (test_prefix_extended_skipped) {
                printf ("do not mix [-x | --skip-prefix] with '-s'\n");
                exit (1);
            }

            test_prefix_skipped = TRUE;
        } else if (strcmp("-x", argv[i]) == 0 || strncmp("-x=", argv[i], 3) == 0 || strcmp("--skip-prefix", argv[i]) == 0 || strncmp("--skip-prefix=", argv[i], 14) == 0) {
            xchar *equal = argv[i] + 2;
            if (*equal == '=') {
                test_paths_skipped = x_slist_prepend(test_paths_skipped, equal + 1);
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_paths_skipped = x_slist_prepend(test_paths_skipped, argv[i]);
            }
            argv[i] = NULL;
            if (test_prefix_skipped) {
                printf ("do not mix [-x | --skip-prefix] with '-s'\n");
                exit (1);
            }

            test_prefix_extended_skipped = TRUE;
        } else if (strcmp("-m", argv[i]) == 0 || strncmp("-m=", argv[i], 3) == 0) {
            xchar *equal = argv[i] + 2;
            const xchar *mode = "";

            if (*equal == '=') {
                mode = equal + 1;
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                mode = argv[i];
            }

            if (strcmp(mode, "perf") == 0) {
                mutable_test_config_vars.test_perf = TRUE;
            } else if (strcmp(mode, "slow") == 0) {
                mutable_test_config_vars.test_quick = FALSE;
            } else if (strcmp(mode, "thorough") == 0) {
                mutable_test_config_vars.test_quick = FALSE;
            } else if (strcmp(mode, "quick") == 0) {
                mutable_test_config_vars.test_quick = TRUE;
                mutable_test_config_vars.test_perf = FALSE;
            } else if (strcmp(mode, "undefined") == 0) {
                mutable_test_config_vars.test_undefined = TRUE;
            } else if (strcmp(mode, "no-undefined") == 0) {
                mutable_test_config_vars.test_undefined = FALSE;
            } else {
                x_error("unknown test mode: -m %s", mode);
            }

            argv[i] = NULL;
        } else if(strcmp ("-q", argv[i]) == 0 || strcmp("--quiet", argv[i]) == 0) {
            mutable_test_config_vars.test_quiet = TRUE;
            mutable_test_config_vars.test_verbose = FALSE;
            argv[i] = NULL;
        } else if (strcmp("--verbose", argv[i]) == 0) {
            mutable_test_config_vars.test_quiet = FALSE;
            mutable_test_config_vars.test_verbose = TRUE;
            argv[i] = NULL;
        } else if (strcmp("-l", argv[i]) == 0) {
            test_run_list = TRUE;
            argv[i] = NULL;
        } else if (strcmp("--seed", argv[i]) == 0 || strncmp("--seed=", argv[i], 7) == 0) {
            xchar *equal = argv[i] + 6;
            if (*equal == '=') {
                test_run_seedstr = equal + 1;
            } else if (i + 1 < argc) {
                argv[i++] = NULL;
                test_run_seedstr = argv[i];
            }

            argv[i] = NULL;
        } else if (strcmp("-?", argv[i]) == 0 || strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
            printf("Usage:\n"
                    "  %s [OPTION...]\n\n"
                    "Help Options:\n"
                    "  -h, --help                     Show help options\n\n"
                    "Test Options:\n"
                    "  --g-fatal-warnings             Make all warnings fatal\n"
                    "  -l                             List test cases available in a test executable\n"
                    "  -m {perf|slow|thorough|quick}  Execute tests according to mode\n"
                    "  -m {undefined|no-undefined}    Execute tests according to mode\n"
                    "  -p TESTPATH                    Only start test cases matching TESTPATH\n"
                    "  -s TESTPATH                    Skip all tests matching TESTPATH\n"
                    "  [-r | --run-prefix] PREFIX     Only start test cases (or suites) matching PREFIX (incompatible with -p).\n"
                    "                                 Unlike the -p option (which only goes one level deep), this option would \n"
                    "                                 run all tests path that have PREFIX at the beginning of their name.\n"
                    "                                 Note that the prefix used should be a valid test path (and not a simple prefix).\n"
                    "  [-x | --skip-prefix] PREFIX    Skip all tests matching PREFIX (incompatible with -s)\n"
                    "                                 Unlike the -s option (which only skips the exact TESTPATH), this option will \n"
                    "                                 skip all the tests that begins with PREFIX).\n"
                    "  --seed=SEEDSTRING              Start tests with random seed SEEDSTRING\n"
                    "  --debug-log                    debug test logging output\n"
                    "  -q, --quiet                    Run tests quietly\n"
                    "  --verbose                      Run tests verbosely\n",
                    argv[0]);
            exit(0);
        }
    }

    test_paths = x_slist_reverse(test_paths);

    e = 0;
    for (i = 0; i < argc; i++) {
        if (argv[i]) {
            argv[e++] = argv[i];
            if (i >= e) {
                argv[i] = NULL;
            }
        }
    }
    *argc_p = e;
}

static void rm_rf(const xchar *path)
{
    XDir *dir = NULL;
    const xchar *entry;

    dir = x_dir_open(path, 0, NULL);
    if (dir == NULL) {
        (void)x_remove(path);
        return;
    }

    while ((entry = x_dir_read_name(dir)) != NULL) {
        xchar *sub_path = x_build_filename(path, entry, NULL);
        rm_rf(sub_path);
        x_free(sub_path);
    }

    x_dir_close(dir);
    x_rmdir(path);
}

static xboolean test_do_isolate_dirs(XError **error)
{
    xchar *data_dirs[3];
    xchar *subdir = NULL;
    xchar *config_dirs[3];
    xchar *home_dir = NULL, *cache_dir = NULL, *config_dir = NULL;
    xchar *state_dir = NULL, *data_dir = NULL, *runtime_dir = NULL;

    if (!test_isolate_dirs) {
        return TRUE;
    }

    subdir = x_build_filename(test_tmpdir, test_run_name_path, ".dirs", NULL);

    runtime_dir = x_build_filename(subdir, "runtime", NULL);
    if (x_mkdir_with_parents(runtime_dir, 0700) != 0) {
        xint saved_errno = errno;
        x_set_error(error, X_FILE_ERROR, x_file_error_from_errno(saved_errno), "Failed to create XDG_RUNTIME_DIR ‘%s’: %s", runtime_dir, x_strerror(saved_errno));
        x_free(runtime_dir);
        x_free(subdir);
        return FALSE;
    }

    home_dir = x_build_filename(subdir, "home", NULL);
    cache_dir = x_build_filename(subdir, "cache", NULL);
    config_dir = x_build_filename(subdir, "config", NULL);
    data_dir = x_build_filename(subdir, "data", NULL);
    state_dir = x_build_filename(subdir, "state", NULL);

    config_dirs[0] = x_build_filename(subdir, "system-config1", NULL);
    config_dirs[1] = x_build_filename(subdir, "system-config2", NULL);
    config_dirs[2] = NULL;

    data_dirs[0] = x_build_filename(subdir, "system-data1", NULL);
    data_dirs[1] = x_build_filename(subdir, "system-data2", NULL);
    data_dirs[2] = NULL;

    x_set_user_dirs("HOME", home_dir,
        "XDG_CACHE_HOME", cache_dir,
        "XDG_CONFIG_DIRS", config_dirs,
        "XDG_CONFIG_HOME", config_dir,
        "XDG_DATA_DIRS", data_dirs,
        "XDG_DATA_HOME", data_dir,
        "XDG_STATE_HOME", state_dir,
        "XDG_RUNTIME_DIR", runtime_dir,
        NULL);

    x_free(runtime_dir);
    x_free(state_dir);
    x_free(data_dir);
    x_free(config_dir);
    x_free(cache_dir);
    x_free(home_dir);
    x_free(data_dirs[1]);
    x_free(data_dirs[0]);
    x_free(config_dirs[1]);
    x_free(config_dirs[0]);
    x_free(subdir);

    return TRUE;
}

static void test_rm_isolate_dirs(void)
{
    xchar *subdir = NULL;

    if (!test_isolate_dirs) {
        return;
    }

    subdir = x_build_filename(test_tmpdir, test_run_name_path, NULL);
    rm_rf(subdir);
    x_free(subdir);
}

void (x_test_init)(int *argc, char ***argv, ...)
{
    va_list args;
    xpointer option;
    static char seedstr[4 + 4 * 8 + 1];
    XLogLevelFlags fatal_mask = (XLogLevelFlags)x_log_set_always_fatal((XLogLevelFlags) X_LOG_FATAL_MASK);

    fatal_mask = (XLogLevelFlags) (fatal_mask | X_LOG_LEVEL_WARNING | X_LOG_LEVEL_CRITICAL);
    x_log_set_always_fatal(fatal_mask);

    x_return_if_fail(argc != NULL);
    x_return_if_fail(argv != NULL);
    x_return_if_fail(x_test_config_vars->test_initialized == FALSE);
    mutable_test_config_vars.test_initialized = TRUE;

    va_start(args, argv);
    while ((option = va_arg(args, char *))) {
        if (x_strcmp0((const char *)option, "no_g_set_prgname") == 0) {
            no_g_set_prgname = TRUE;
        } else if (x_strcmp0((const char *)option, X_TEST_OPTION_ISOLATE_DIRS) == 0) {
            test_isolate_dirs = TRUE;
        }
    }
    va_end (args);

    parse_args(argc, argv);

    if (test_run_seedstr == NULL) {
        x_snprintf(seedstr, sizeof(seedstr), "R02S%08x%08x%08x%08x", x_random_int(), x_random_int(), x_random_int(), x_random_int());
        test_run_seedstr = seedstr;
    }

    if (!x_get_prgname() && !no_g_set_prgname) {
        x_set_prgname((*argv)[0]);
    }

    if (x_getenv("X_TEST_ROOT_PROCESS")) {
        test_is_subtest = TRUE;
    } else if (!x_setenv("X_TEST_ROOT_PROCESS", test_argv0 ? test_argv0 : "root", TRUE)) {
        x_printerr("%s: Failed to set environment variable ‘%s’\n", test_argv0, "G_TEST_ROOT_PROCESS");
        exit (1);
    }

    if (test_isolate_dirs) {
        if (x_getenv("X_TEST_TMPDIR") == NULL) {
            xchar *tmpl = NULL;
            xchar *test_prgname = NULL;
            XError *local_error = NULL;

            test_prgname = x_path_get_basename(x_get_prgname());
            if (*test_prgname == '\0') {
                x_free(test_prgname);
                test_prgname = x_strdup("unknown");
            }

            tmpl = x_strdup_printf("test_%s_XXXXXX", test_prgname);
            x_free(test_prgname);

            test_isolate_dirs_tmpdir = x_dir_make_tmp(tmpl, &local_error);
            if (local_error != NULL) {
                x_printerr("%s: Failed to create temporary directory: %s\n", (*argv)[0], local_error->message);
                x_error_free(local_error);
                exit(1);
            }
            x_free(tmpl);

            if (!x_setenv("X_TEST_TMPDIR", test_isolate_dirs_tmpdir, TRUE)) {
                x_printerr("%s: Failed to set environment variable ‘%s’\n", (*argv)[0], "G_TEST_TMPDIR");
                exit(1);
            }

            _x_unset_cached_tmp_dir();

            {
                const xchar *overridden_environment_variables[] = {
                    "HOME",
                    "XDG_CACHE_HOME",
                    "XDG_CONFIG_DIRS",
                    "XDG_CONFIG_HOME",
                    "XDG_DATA_DIRS",
                    "XDG_DATA_HOME",
                    "XDG_RUNTIME_DIR",
                };

                xsize i;
                for (i = 0; i < X_N_ELEMENTS(overridden_environment_variables); i++) {
                    if (!x_setenv(overridden_environment_variables[i], "/dev/null", TRUE)) {
                        x_printerr("%s: Failed to set environment variable ‘%s’\n", (*argv)[0], overridden_environment_variables[i]);
                        exit(1);
                    }
                }
            }
        }

        test_tmpdir = x_getenv("X_TEST_TMPDIR");
    }

    if (1) {
        XRand *rg = x_rand_new_with_seed(0xc8c49fb6);
        xuint32 t1 = x_rand_int(rg), t2 = x_rand_int(rg), t3 = x_rand_int(rg), t4 = x_rand_int(rg);

        if (t1 != 0xfab39f9b || t2 != 0xb948fb0e || t3 != 0x3d31be26 || t4 != 0x43a19d66) {
            x_warning("random numbers are not XRand-2.2 compatible, seeds may be broken (check $G_RANDOM_VERSION)");
        }

        x_rand_free(rg);
    }

    test_run_seed(test_run_seedstr);

    x_log_set_default_handler(xtest_default_log_handler, NULL);
    x_test_log(X_TEST_LOG_START_BINARY, x_get_prgname(), test_run_seedstr, 0, NULL);

    test_argv0_dirname = (test_argv0 != NULL) ? x_path_get_dirname(test_argv0) : x_strdup(".");

    if (x_str_has_suffix(test_argv0_dirname, "/.libs")) {
        xchar *tmp;
        tmp = x_path_get_dirname(test_argv0_dirname);
        x_free(test_argv0_dirname);
        test_argv0_dirname = tmp;
    }

    test_disted_files_dir = x_getenv("X_TEST_SRCDIR");
    if (!test_disted_files_dir) {
        test_disted_files_dir = test_argv0_dirname;
    }

    test_built_files_dir = x_getenv("X_TEST_BUILDDIR");
    if (!test_built_files_dir) {
        test_built_files_dir = test_argv0_dirname;
    }
}

static void test_cleanup (void)
{
    x_clear_pointer(&test_run_rand, x_rand_free);
    x_clear_pointer(&test_argv0_dirname, x_free);
    x_clear_pointer(&test_initial_cwd, x_free);
}

static void test_run_seed(const xchar *rseed)
{
    xuint seed_failed = 0;

    if (test_run_rand) {
        x_rand_free(test_run_rand);
    }
    test_run_rand = NULL;

    while (strchr(" \t\v\r\n\f", *rseed)) {
        rseed++;
    }

    if (strncmp(rseed, "R02S", 4) == 0) {
        const char *s = rseed + 4;
        if (strlen(s) >= 32) {
            xuint32 seedarray[4];
            xchar *p, hexbuf[9] = { 0, };
            memcpy(hexbuf, s + 0, 8);
            seedarray[0] = x_ascii_strtoull(hexbuf, &p, 16);
            seed_failed += p != NULL && *p != 0;
            memcpy(hexbuf, s + 8, 8);
            seedarray[1] = x_ascii_strtoull(hexbuf, &p, 16);
            seed_failed += p != NULL && *p != 0;
            memcpy(hexbuf, s + 16, 8);
            seedarray[2] = x_ascii_strtoull(hexbuf, &p, 16);
            seed_failed += p != NULL && *p != 0;
            memcpy(hexbuf, s + 24, 8);
            seedarray[3] = x_ascii_strtoull(hexbuf, &p, 16);
            seed_failed += p != NULL && *p != 0;
            if (!seed_failed) {
                test_run_rand = x_rand_new_with_seed_array(seedarray, 4);
                return;
            }
        }
    }

    x_error("Unknown or invalid random seed: %s", rseed);
}

xint32 x_test_rand_int (void)
{
    xint32 r;

    X_LOCK(test_run_rand);
    r = x_rand_int(test_run_rand);
    X_UNLOCK(test_run_rand);

    return r;
}

xint32 x_test_rand_int_range(xint32 begin, xint32 end)
{
    xint32 r;

    X_LOCK(test_run_rand);
    r = x_rand_int_range(test_run_rand, begin, end);
    X_UNLOCK(test_run_rand);

    return r;
}

double x_test_rand_double(void)
{
    double r;

    X_LOCK(test_run_rand);
    r = x_rand_double(test_run_rand);
    X_UNLOCK(test_run_rand);

    return r;
}

double x_test_rand_double_range(double range_start, double range_end)
{
    double r;

    X_LOCK (test_run_rand);
    r = x_rand_double_range(test_run_rand, range_start, range_end);
    X_UNLOCK (test_run_rand);

    return r;
}

void x_test_timer_start(void)
{
    if (!test_user_timer) {
        test_user_timer = x_timer_new();
    }

    test_user_stamp = 0;
    x_timer_start(test_user_timer);
}

double x_test_timer_elapsed(void)
{
    test_user_stamp = test_user_timer ? x_timer_elapsed(test_user_timer, NULL) : 0;
    return test_user_stamp;
}

double x_test_timer_last (void)
{
    return test_user_stamp;
}

void x_test_minimized_result(double minimized_quantity, const char *format, ...)
{
    va_list args;
    xchar *buffer;
    long double largs = minimized_quantity;

    va_start(args, format);
    buffer = x_strdup_vprintf(format, args);
    va_end(args);

    x_test_log(X_TEST_LOG_MIN_RESULT, buffer, NULL, 1, &largs);
    x_free(buffer);
}

void x_test_maximized_result(double maximized_quantity, const char *format, ...)
{
    va_list args;
    xchar *buffer;
    long double largs = maximized_quantity;

    va_start(args, format);
    buffer = x_strdup_vprintf(format, args);
    va_end(args);

    x_test_log(X_TEST_LOG_MAX_RESULT, buffer, NULL, 1, &largs);
    x_free(buffer);
}

void x_test_message(const char *format, ...)
{
    va_list args;
    xchar *buffer;

    va_start(args, format);
    buffer = x_strdup_vprintf(format, args);
    va_end(args);

    x_test_log(X_TEST_LOG_MESSAGE, buffer, NULL, 0, NULL);
    x_free(buffer);
}

void x_test_bug_base(const char *uri_pattern)
{
    x_free(test_uri_base);
    test_uri_base = x_strdup(uri_pattern);
}

void x_test_bug(const char *bug_uri_snippet)
{
    const char *c = NULL;

    x_return_if_fail(bug_uri_snippet != NULL);

    if (x_str_has_prefix(bug_uri_snippet, "http:") || x_str_has_prefix(bug_uri_snippet, "https:")) {
        x_test_message("Bug Reference: %s", bug_uri_snippet);
        return;
    }

    if (test_uri_base != NULL) {
        c = strstr (test_uri_base, "%s");
    }

    if (c) {
        char *b = x_strndup(test_uri_base, c - test_uri_base);
        char *s = x_strconcat(b, bug_uri_snippet, c + 2, NULL);

        x_free(b);
        x_test_message("Bug Reference: %s", s);
        x_free(s);
    } else {
        x_test_message("Bug Reference: %s%s", test_uri_base ? test_uri_base : "", bug_uri_snippet);
    }
}

void x_test_summary(const char *summary)
{
    x_return_if_fail(summary != NULL);
    x_return_if_fail(strchr (summary, '\n') == NULL);
    x_return_if_fail(strchr (summary, '\r') == NULL);

    x_test_message("%s summary: %s", test_run_name, summary);
}

XTestSuite *x_test_get_root(void)
{
    if (!test_suite_root) {
        test_suite_root = x_test_create_suite("root");
        x_free(test_suite_root->name);
        test_suite_root->name = x_strdup("");
    }

    return test_suite_root;
}

int x_test_run(void)
{
    int ret;
    XTestSuite *suite;

    if (atexit(test_cleanup) != 0) {
        int errsv = errno;
        x_error("Unable to register test cleanup to be run at exit: %s", x_strerror(errsv));
    }

    suite = x_test_get_root();
    if (x_test_run_suite(suite) != 0) {
        ret = 1;
        goto out;
    }

    if (test_isolate_dirs_tmpdir != NULL) {
        rm_rf(test_isolate_dirs_tmpdir);
        x_free(test_isolate_dirs_tmpdir);
        test_isolate_dirs_tmpdir = NULL;
    }

    if (test_tap_log) {
        ret = 0;
        goto out;
    }

    if (test_run_count > 0 && test_run_count == test_skipped_count) {
        ret = 77;
        goto out;
    } else {
        ret = 0;
        goto out;
    }

out:
    x_test_suite_free(suite);
    return ret;
}

XTestCase *x_test_create_case(const char *test_name, xsize data_size, xconstpointer test_data, XTestFixtureFunc data_setup, XTestFixtureFunc data_test, XTestFixtureFunc data_teardown)
{
    XTestCase *tc;

    x_return_val_if_fail(test_name != NULL, NULL);
    x_return_val_if_fail(strchr (test_name, '/') == NULL, NULL);
    x_return_val_if_fail(test_name[0] != 0, NULL);
    x_return_val_if_fail(data_test != NULL, NULL);

    tc = x_slice_new0(XTestCase);
    tc->name = x_strdup(test_name);
    tc->test_data = (xpointer)test_data;
    tc->fixture_size = data_size;
    tc->fixture_setup = (void (*)(void *, xconstpointer))data_setup;
    tc->fixture_test = (void (*)(void *, xconstpointer))data_test;
    tc->fixture_teardown = (void (*)(void *, xconstpointer))data_teardown;

    return tc;
}

static xint find_suite(xconstpointer l, xconstpointer s)
{
    const xchar *str = (const xchar *)s;
    const XTestSuite *suite = (const XTestSuite *)l;

    return strcmp(suite->name, str);
}

static xint find_case(xconstpointer l, xconstpointer s)
{
    const xchar *str = (const xchar *)s;
    const XTestCase *tc = (const XTestCase *)l;

    return strcmp(tc->name, str);
}

void x_test_add_vtable(const char *testpath, xsize data_size, xconstpointer test_data, XTestFixtureFunc data_setup, XTestFixtureFunc fixture_test_func, XTestFixtureFunc data_teardown)
{
    xuint ui;
    xchar **segments;
    XTestSuite *suite;

    x_return_if_fail(testpath != NULL);
    x_return_if_fail(x_path_is_absolute(testpath));
    x_return_if_fail(fixture_test_func != NULL);
    x_return_if_fail(!test_isolate_dirs || strstr (testpath, "/.") == NULL);

    suite = x_test_get_root();
    segments = x_strsplit(testpath, "/", -1);
    for (ui = 0; segments[ui] != NULL; ui++) {
        const char *seg = segments[ui];
        xboolean islast = segments[ui + 1] == NULL;
        if (islast && !seg[0]) {
            x_error("invalid test case path: %s", testpath);
        } else if (!seg[0]) {
            continue;
        } else if (!islast) {
            XSList *l;
            XTestSuite *csuite;

            l = x_slist_find_custom(suite->suites, seg, find_suite);
            if (l) {
                csuite = (XTestSuite *)l->data;
            } else {
                csuite = x_test_create_suite(seg);
                x_test_suite_add_suite(suite, csuite);
            }

            suite = csuite;
        } else  {
            XTestCase *tc;

            if (x_slist_find_custom(suite->cases, seg, find_case)) {
                x_error("duplicate test case path: %s", testpath);
            }

            tc = x_test_create_case(seg, data_size, test_data, data_setup, fixture_test_func, data_teardown);
            x_test_suite_add(suite, tc);
        }
    }

    x_strfreev(segments);
}

void x_test_fail(void)
{
    test_run_success = X_TEST_RUN_FAILURE;
    x_clear_pointer(&test_run_msg, x_free);
}

void x_test_fail_printf(const char *format, ...)
{
    va_list args;

    test_run_success = X_TEST_RUN_FAILURE;
    va_start(args, format);
    x_free(test_run_msg);
    test_run_msg = x_strdup_vprintf(format, args);
    va_end(args);
}

void x_test_incomplete(const xchar *msg)
{
    test_run_success = X_TEST_RUN_INCOMPLETE;
    x_free(test_run_msg);
    test_run_msg = x_strdup(msg);
}

void x_test_incomplete_printf(const char *format, ...)
{
    va_list args;

    test_run_success = X_TEST_RUN_INCOMPLETE;
    va_start(args, format);
    x_free(test_run_msg);
    test_run_msg = x_strdup_vprintf(format, args);
    va_end(args);
}

void x_test_skip(const xchar *msg)
{
    test_run_success = X_TEST_RUN_SKIPPED;
    x_free(test_run_msg);
    test_run_msg = x_strdup(msg);
}

void x_test_skip_printf(const char *format, ...)
{
    va_list args;

    test_run_success = X_TEST_RUN_SKIPPED;
    va_start(args, format);
    x_free(test_run_msg);
    test_run_msg = x_strdup_vprintf(format, args);
    va_end(args);
}

xboolean x_test_failed(void)
{
    return test_run_success != X_TEST_RUN_SUCCESS;
}

void x_test_set_nonfatal_assertions(void)
{
    if (!x_test_config_vars->test_initialized) {
        x_error("x_test_set_nonfatal_assertions called without x_test_init");
    }

    test_nonfatal_assertions = TRUE;
    test_mode_fatal = FALSE;
}

void x_test_add_func(const char *testpath, XTestFunc test_func)
{
    x_return_if_fail(testpath != NULL);
    x_return_if_fail(testpath[0] == '/');
    x_return_if_fail(test_func != NULL);
    x_test_add_vtable(testpath, 0, NULL, NULL, (XTestFixtureFunc)test_func, NULL);
}

void x_test_add_data_func(const char *testpath, xconstpointer test_data, XTestDataFunc test_func)
{
    x_return_if_fail(testpath != NULL);
    x_return_if_fail(testpath[0] == '/');
    x_return_if_fail(test_func != NULL);

    x_test_add_vtable(testpath, 0, test_data, NULL, (XTestFixtureFunc)test_func, NULL);
}

void x_test_add_data_func_full(const char *testpath, xpointer test_data, XTestDataFunc test_func, XDestroyNotify data_free_func)
{
    x_return_if_fail(testpath != NULL);
    x_return_if_fail(testpath[0] == '/');
    x_return_if_fail(test_func != NULL);

    x_test_add_vtable(testpath, 0, test_data, NULL, (XTestFixtureFunc)test_func, (XTestFixtureFunc)data_free_func);
}

static xboolean x_test_suite_case_exists(XTestSuite *suite, const char *test_path)
{
    char *slash;
    XSList *iter;
    XTestCase *tc;

    test_path++;
    slash = (char *)strchr(test_path, '/');

    if (slash) {
        for (iter = suite->suites; iter; iter = iter->next) {
            XTestSuite *child_suite = (XTestSuite *)iter->data;

            if (!strncmp(child_suite->name, test_path, slash - test_path)) {
                if (x_test_suite_case_exists(child_suite, slash)) {
                    return TRUE;
                }
            }
        }
    } else {
        for (iter = suite->cases; iter; iter = iter->next) {
            tc = (XTestCase *)iter->data;
            if (!strcmp(tc->name, test_path)) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

XTestSuite *x_test_create_suite(const char *suite_name)
{
    XTestSuite *ts;
    x_return_val_if_fail(suite_name != NULL, NULL);
    x_return_val_if_fail(strchr(suite_name, '/') == NULL, NULL);
    x_return_val_if_fail(suite_name[0] != 0, NULL);
    ts = x_slice_new0(XTestSuite);
    ts->name = x_strdup(suite_name);

    return ts;
}

void x_test_suite_add(XTestSuite *suite, XTestCase *test_case)
{
    x_return_if_fail(suite != NULL);
    x_return_if_fail(test_case != NULL);

    suite->cases = x_slist_append(suite->cases, test_case);
}

void x_test_suite_add_suite(XTestSuite *suite, XTestSuite *nestedsuite)
{
    x_return_if_fail(suite != NULL);
    x_return_if_fail(nestedsuite != NULL);

    suite->suites = x_slist_append(suite->suites, nestedsuite);
}

void x_test_queue_free(xpointer gfree_pointer)
{
    if (gfree_pointer) {
        x_test_queue_destroy(x_free, gfree_pointer);
    }
}

void x_test_queue_destroy(XDestroyNotify destroy_func, xpointer destroy_data)
{
    XestroyEntry *dentry;

    x_return_if_fail(destroy_func != NULL);

    dentry = x_slice_new0(XestroyEntry);
    dentry->destroy_func = destroy_func;
    dentry->destroy_data = destroy_data;
    dentry->next = test_destroy_queue;
    test_destroy_queue = dentry;
}

static xint test_has_prefix(xconstpointer a, xconstpointer b)
{
    const xchar *test_path_skipped_local = (const xchar *)a;
    const xchar *test_run_name_local = (const xchar *)b;
    if (test_prefix_extended_skipped) {

        if (!test_path_skipped_local || !test_run_name_local) {
          return FALSE;
        }

        return strncmp(test_run_name_local, test_path_skipped_local, strlen(test_path_skipped_local));
    }

    return x_strcmp0(test_run_name_local, test_path_skipped_local);
}

static xboolean test_case_run(XTestCase *tc)
{
    xboolean success = X_TEST_RUN_SUCCESS;
    xchar *old_base = x_strdup(test_uri_base);
    XSList **old_free_list, *filename_free_list = NULL;

    old_free_list = test_filename_free_list;
    test_filename_free_list = &filename_free_list;

    if (++test_run_count <= test_startup_skip_count) {
        x_test_log(X_TEST_LOG_SKIP_CASE, test_run_name, NULL, 0, NULL);
    } else if (test_run_list) {
        x_print("%s\n", test_run_name);
        x_test_log(X_TEST_LOG_LIST_CASE, test_run_name, NULL, 0, NULL);
    } else {
        void *fixture;
        long double largs[X_TEST_CASE_LARGS_MAX];
        XTimer *test_run_timer = x_timer_new();

        x_test_log(X_TEST_LOG_START_CASE, test_run_name, NULL, 0, NULL);
        test_run_forks = 0;
        test_run_success = X_TEST_RUN_SUCCESS;
        x_clear_pointer(&test_run_msg, x_free);
        x_test_log_set_fatal_handler(NULL, NULL);

        if (test_paths_skipped && x_slist_find_custom(test_paths_skipped, test_run_name, (XCompareFunc)test_has_prefix)) {
            x_test_skip("by request (-s option)");
        } else {
            XError *local_error = NULL;

            if (!test_do_isolate_dirs(&local_error)) {
                x_test_log(X_TEST_LOG_ERROR, local_error->message, NULL, 0, NULL);
                x_test_fail();
                x_error_free(local_error);
            } else {
                x_timer_start(test_run_timer);
                fixture = tc->fixture_size ? x_malloc0(tc->fixture_size) : tc->test_data;
                test_run_seed(test_run_seedstr);
                if (tc->fixture_setup) {
                    tc->fixture_setup(fixture, tc->test_data);
                }

                tc->fixture_test(fixture, tc->test_data);
                test_trap_clear();

                while (test_destroy_queue) {
                    XestroyEntry *dentry = test_destroy_queue;
                    test_destroy_queue = dentry->next;
                    dentry->destroy_func (dentry->destroy_data);
                    x_slice_free(XestroyEntry, dentry);
                }

                if (tc->fixture_teardown) {
                    tc->fixture_teardown(fixture, tc->test_data);
                }

                if (tc->fixture_size) {
                    x_free(fixture);
                }

                x_timer_stop(test_run_timer);
            }

            test_rm_isolate_dirs();
        }

        success = test_run_success;
        test_run_success = X_TEST_RUN_FAILURE;
        largs[X_TEST_CASE_LARGS_RESULT] = success;
        largs[X_TEST_CASE_LARGS_RUN_FORKS] = test_run_forks;
        largs[X_TEST_CASE_LARGS_EXECUTION_TIME] = x_timer_elapsed(test_run_timer, NULL);

        x_test_log(X_TEST_LOG_STOP_CASE, test_run_name, test_run_msg, X_N_ELEMENTS(largs), largs);
        x_clear_pointer(&test_run_msg, x_free);
        x_timer_destroy(test_run_timer);
    }

    x_slist_free_full(filename_free_list, x_free);
    test_filename_free_list = old_free_list;
    x_free (test_uri_base);
    test_uri_base = old_base;

    return (success == X_TEST_RUN_SUCCESS || success == X_TEST_RUN_SKIPPED || success == X_TEST_RUN_INCOMPLETE);
}

static xboolean path_has_prefix(const char *path, const char *prefix)
{
    int prefix_len = strlen(prefix);
    return (strncmp(path, prefix, prefix_len) == 0 && (path[prefix_len] == '\0' || path[prefix_len] == '/'));
}

static xboolean test_should_run(const char *test_path, const char *cmp_path)
{
    if (strstr(test_run_name, "/subprocess")) {
        if (x_strcmp0(test_path, cmp_path) == 0) {
            return TRUE;
        }

        if (x_test_verbose()) {
            if (test_tap_log) {
                x_print("skipping: %s\n", test_run_name);
            } else {
                x_print("GTest: skipping: %s\n", test_run_name);
            }
        }

        return FALSE;
    }

    return !cmp_path || path_has_prefix(test_path, cmp_path);
}

static int x_test_run_suite_internal(XTestSuite *suite, const char *path)
{
    XSList *iter;
    xuint n_bad = 0;
    xchar *old_name = test_run_name;
    xchar *old_name_path = test_run_name_path;

    x_return_val_if_fail(suite != NULL, -1);
    x_test_log(X_TEST_LOG_START_SUITE, suite->name, NULL, 0, NULL);

    for (iter = suite->cases; iter; iter = iter->next) {
        XTestCase *tc = (XTestCase *)iter->data;

        test_run_name = x_build_path("/", old_name, tc->name, NULL);
        test_run_name_path = x_build_path(X_DIR_SEPARATOR_S, old_name_path, tc->name, NULL);
        if (test_should_run(test_run_name, path)) {
            if (!test_case_run(tc)) {
                n_bad++;
            }
        }

        x_free(test_run_name);
        x_free(test_run_name_path);
    }

    for (iter = suite->suites; iter; iter = iter->next) {
        XTestSuite *ts = (XTestSuite *)iter->data;

        test_run_name = x_build_path("/", old_name, ts->name, NULL);
        test_run_name_path = x_build_path(X_DIR_SEPARATOR_S, old_name_path, ts->name, NULL);
        if (test_prefix_extended) {
            if (!path || path_has_prefix(test_run_name, path)) {
                n_bad += x_test_run_suite_internal(ts, test_run_name);
            } else if (!path || path_has_prefix(path, test_run_name)) {
                n_bad += x_test_run_suite_internal(ts, path);
            }
        } else if (!path || path_has_prefix(path, test_run_name)) {
            n_bad += x_test_run_suite_internal(ts, path);
        }

        x_free(test_run_name);
        x_free(test_run_name_path);
    }

    test_run_name = old_name;
    test_run_name_path = old_name_path;

    x_test_log(X_TEST_LOG_STOP_SUITE, suite->name, NULL, 0, NULL);

    return n_bad;
}

static int x_test_suite_count(XTestSuite *suite)
{
    int n = 0;
    XSList *iter;

    x_return_val_if_fail(suite != NULL, -1);

    for (iter = suite->cases; iter; iter = iter->next) {
        XTestCase *tc = (XTestCase *)iter->data;

        if (strcmp(tc->name, "subprocess") != 0) {
            n++;
        }
    }

    for (iter = suite->suites; iter; iter = iter->next) {
        XTestSuite *ts = (XTestSuite *)iter->data;

        if (strcmp(ts->name, "subprocess") != 0) {
            n += x_test_suite_count(ts);
        }
    }

    return n;
}

int x_test_run_suite(XTestSuite *suite)
{
    int n_bad = 0;

    x_return_val_if_fail(x_test_run_once == TRUE, -1);

    x_test_run_once = FALSE;
    test_count = x_test_suite_count(suite);

    test_run_name = x_strdup_printf("/%s", suite->name);
    test_run_name_path = x_build_path(X_DIR_SEPARATOR_S, suite->name, NULL);

    if (test_paths) {
        XSList *iter;

        for (iter = test_paths; iter; iter = iter->next) {
            n_bad += x_test_run_suite_internal(suite, (const char *)iter->data);
        }
    } else {
        n_bad = x_test_run_suite_internal(suite, NULL);
    }

    x_clear_pointer(&test_run_name, x_free);
    x_clear_pointer(&test_run_name_path, x_free);

    return n_bad;
}

void x_test_case_free(XTestCase *test_case)
{
    x_free(test_case->name);
    x_slice_free(XTestCase, test_case);
}

void x_test_suite_free(XTestSuite *suite)
{
    x_slist_free_full(suite->cases, (XDestroyNotify)x_test_case_free);
    x_free(suite->name);
    x_slist_free_full(suite->suites, (XDestroyNotify)x_test_suite_free);
    x_slice_free(XTestSuite, suite);
}

static void xtest_default_log_handler(const xchar *log_domain, XLogLevelFlags log_level, const xchar *message, xpointer unused_data)
{
    xchar *msg;
    xuint i = 0;
    const xchar *strv[16];
    xboolean fatal = FALSE;

    if (log_domain) {
        strv[i++] = log_domain;
        strv[i++] = "-";
    }

    if (log_level & X_LOG_FLAG_FATAL) {
        strv[i++] = "FATAL-";
        fatal = TRUE;
    }

    if (log_level & X_LOG_FLAG_RECURSION) {
        strv[i++] = "RECURSIVE-";
    }

    if (log_level & X_LOG_LEVEL_ERROR) {
        strv[i++] = "ERROR";
    }

    if (log_level & X_LOG_LEVEL_CRITICAL) {
        strv[i++] = "CRITICAL";
    }

    if (log_level & X_LOG_LEVEL_WARNING) {
        strv[i++] = "WARNING";
    }

    if (log_level & X_LOG_LEVEL_MESSAGE) {
        strv[i++] = "MESSAGE";
    }

    if (log_level & X_LOG_LEVEL_INFO) {
        strv[i++] = "INFO";
    }

    if (log_level & X_LOG_LEVEL_DEBUG) {
        strv[i++] = "DEBUG";
    }

    strv[i++] = ": ";
    strv[i++] = message;
    strv[i++] = NULL;

    msg = x_strjoinv("", (xchar **)strv);
    x_test_log(fatal ? X_TEST_LOG_ERROR : X_TEST_LOG_MESSAGE, msg, NULL, 0, NULL);
 
    x_free(msg);
    if (!test_tap_log) {
        x_log_default_handler(log_domain, log_level, message, unused_data);
    }
}

void x_assertion_message(const char *domain, const char *file, int line, const char *func, const char *message)
{
    char *s;
    char lstr[32];

    if (!message) {
        message = "code should not be reached";
    }

    x_snprintf(lstr, 32, "%d", line);
    s = x_strconcat(domain ? domain : "", domain && domain[0] ? ":" : "", "ERROR:", file, ":", lstr, ":", func, func[0] ? ":" : "", " ", message, NULL);
    x_printerr("**\n%s\n", s);

    if (test_nonfatal_assertions || test_in_subprocess || test_in_forked_child) {
        x_test_log(X_TEST_LOG_MESSAGE, s, NULL, 0, NULL);
    } else {
        x_test_log(X_TEST_LOG_ERROR, s, NULL, 0, NULL);
    }

    if (test_nonfatal_assertions) {
        x_free(s);
        x_test_fail();
        return;
    }

    if (__xlib_assert_msg != NULL) {
        free(__xlib_assert_msg);
    }

    __xlib_assert_msg = (char *)malloc(strlen(s) + 1);
    strcpy(__xlib_assert_msg, s);

    x_free(s);

    if (test_in_subprocess) {
        _exit(1);
    } else {
        x_abort();
    }
}

void x_assertion_message_expr(const char *domain, const char *file, int line, const char *func, const char *expr)
{
    char *s;

    if (!expr) {
        s = x_strdup("code should not be reached");
    } else {
        s = x_strconcat("assertion failed: (", expr, ")", NULL);
    }

    x_assertion_message(domain, file, line, func, s);
    x_free(s);

    if (test_in_subprocess) {
        _exit(1);
    } else {
        x_abort();
    }
}

void x_assertion_message_cmpint(const char *domain, const char *file, int line, const char *func, const char *expr, xuint64 arg1, const char *cmp, xuint64 arg2, char numtype)
{
    char *s = NULL;

    switch (numtype) {
        case 'i':
            s = x_strdup_printf("assertion failed (%s): (%" PRIi64 " %s %" PRIi64 ")", expr, (int64_t)arg1, cmp, (int64_t)arg2);
            break;

        case 'u':
            s = x_strdup_printf("assertion failed (%s): (%" PRIu64 " %s %" PRIu64 ")", expr, (uint64_t)arg1, cmp, (uint64_t)arg2);
            break;

        case 'x':
            s = x_strdup_printf("assertion failed (%s): (0x%08" PRIx64 " %s 0x%08" PRIx64 ")", expr, (uint64_t)arg1, cmp, (uint64_t)arg2);
            break;

        default:
            x_assert_not_reached();
    }

    x_assertion_message(domain, file, line, func, s);
    x_free(s);
}

void x_assertion_message_cmpnum(const char *domain, const char *file, int line, const char *func, const char *expr, long double arg1, const char *cmp, long double arg2, char numtype)
{
    char *s = NULL;

    switch (numtype) {
        case 'i':
        case 'x':
            x_assertion_message_cmpint(domain, file, line, func, expr, (xuint64) arg1, cmp, (xuint64)arg2, numtype);
            break;

        case 'f':
            s = x_strdup_printf("assertion failed (%s): (%.9g %s %.9g)", expr, (double)arg1, cmp, (double)arg2);
            break;

        default:
            x_assert_not_reached();
    }

    x_assertion_message(domain, file, line, func, s);
    x_free(s);
}

void x_assertion_message_cmpstr(const char *domain, const char *file, int line, const char *func, const char *expr, const char *arg1, const char *cmp, const char *arg2)
{
    char *a1, *a2, *s, *t1 = NULL, *t2 = NULL;

    a1 = arg1 ? x_strconcat("\"", t1 = x_strescape(arg1, NULL), "\"", NULL) : x_strdup("NULL");
    a2 = arg2 ? x_strconcat("\"", t2 = x_strescape(arg2, NULL), "\"", NULL) : x_strdup("NULL");
    x_free(t1);
    x_free(t2);

    s = x_strdup_printf("assertion failed (%s): (%s %s %s)", expr, a1, cmp, a2);
    x_free(a1);
    x_free(a2);

    x_assertion_message(domain, file, line, func, s);
    x_free(s);
}

void x_assertion_message_cmpstrv (const char *domain, const char *file, int line, const char *func, const char *expr, const char *const *arg1, const char *const *arg2, xsize first_wrong_idx)
{
    char *a1, *a2, *s, *t1 = NULL, *t2 = NULL;
    const char *s1 = arg1[first_wrong_idx], *s2 = arg2[first_wrong_idx];

    a1 = x_strconcat("\"", t1 = x_strescape(s1, NULL), "\"", NULL);
    a2 = x_strconcat("\"", t2 = x_strescape(s2, NULL), "\"", NULL);
    x_free(t1);
    x_free(t2);

    s = x_strdup_printf("assertion failed (%s): first differing element at index %" X_XSIZE_FORMAT ": %s does not equal %s", expr, first_wrong_idx, a1, a2);
    x_free(a1);
    x_free(a2);

    x_assertion_message(domain, file, line, func, s);
    x_free(s);
}

void x_assertion_message_error(const char *domain, const char *file, int line, const char *func, const char *expr, const XError *error, XQuark error_domain, int error_code)
{
    XString *gstring;

    gstring = x_string_new("assertion failed ");
    if (error_domain) {
        x_string_append_printf(gstring, "(%s == (%s, %d)): ", expr, x_quark_to_string(error_domain), error_code);
    } else {
        x_string_append_printf(gstring, "(%s == NULL): ", expr);
    }

    if (error) {
        x_string_append_printf(gstring, "%s (%s, %d)", error->message, x_quark_to_string(error->domain), error->code);
    } else {
        x_string_append_printf(gstring, "%s is NULL", expr);
    }

    x_assertion_message(domain, file, line, func, gstring->str);
    x_string_free(gstring, TRUE);
}

int x_strcmp0(const char *str1, const char *str2)
{
    if (!str1) {
        return -(str1 != str2);
    }

    if (!str2) {
        return str1 != str2;
    }

    return strcmp(str1, str2);
}

static void test_trap_clear(void)
{
    test_trap_last_status = 0;
    test_trap_last_pid = 0;

    x_clear_pointer(&test_trap_last_subprocess, x_free);
    x_clear_pointer(&test_trap_last_stdout, x_free);
    x_clear_pointer(&test_trap_last_stderr, x_free);
}

static int safe_dup2(int fd1, int fd2)
{
    int ret;

    do {
        ret = dup2(fd1, fd2);
    } while (ret < 0 && errno == EINTR);

    return ret;
}

typedef struct {
    XPid       pid;
    XMainLoop  *loop;
    int        child_status;

    XIOChannel *stdout_io;
    xboolean   echo_stdout;
    XString    *stdout_str;

    XIOChannel *stderr_io;
    xboolean   echo_stderr;
    XString    *stderr_str;
} WaitForChildData;

static void check_complete(WaitForChildData *data)
{
    if (data->child_status != -1 && data->stdout_io == NULL && data->stderr_io == NULL) {
        x_main_loop_quit(data->loop);
    }
}

static void child_exited(XPid pid, xint status, xpointer user_data)
{
    WaitForChildData *data = (WaitForChildData *)user_data;

    x_assert(status != -1);
    data->child_status = status;

    check_complete(data);
}

static xboolean child_timeout(xpointer user_data)
{
    WaitForChildData *data = (WaitForChildData *)user_data;
    kill(data->pid, SIGALRM);

    return FALSE;
}

static xboolean child_read(XIOChannel *io, XIOCondition cond, xpointer user_data)
{
    xchar buf[4096];
    XIOStatus status;
    FILE *echo_file = NULL;
    xsize nread, nwrote, total;
    WaitForChildData *data = (WaitForChildData *)user_data;

    status = x_io_channel_read_chars(io, buf, sizeof (buf), &nread, NULL);
    if (status == X_IO_STATUS_ERROR || status == X_IO_STATUS_EOF) {
        if (io == data->stdout_io) {
            x_clear_pointer(&data->stdout_io, x_io_channel_unref);
        } else {
            x_clear_pointer(&data->stderr_io, x_io_channel_unref);
        }

        check_complete(data);
        return FALSE;
    } else if (status == X_IO_STATUS_AGAIN) {
        return TRUE;
    }

    if (io == data->stdout_io) {
        x_string_append_len(data->stdout_str, buf, nread);
        if (data->echo_stdout) {
            if X_UNLIKELY(!test_tap_log) {
                echo_file = stdout;
            }
        }
    } else {
        x_string_append_len(data->stderr_str, buf, nread);
        if (data->echo_stderr) {
            echo_file = stderr;
        }
    }

    if (echo_file) {
        for (total = 0; total < nread; total += nwrote) {
            int errsv;

            nwrote = fwrite(buf + total, 1, nread - total, echo_file);
            errsv = errno;
            if (nwrote == 0) {
                x_error("write failed: %s", x_strerror(errsv));
            }
        }
    }

    return TRUE;
}

static void wait_for_child(XPid pid, int stdout_fd, xboolean echo_stdout, int stderr_fd, xboolean echo_stderr, xuint64 timeout)
{
    XSource *source;
    WaitForChildData data;
    XMainContext *context;

    data.pid = pid;
    data.child_status = -1;

    context = x_main_context_new();
    data.loop = x_main_loop_new(context, FALSE);

    source = x_child_watch_source_new(pid);
    x_source_set_callback(source, (XSourceFunc)child_exited, &data, NULL);
    x_source_attach(source, context);
    x_source_unref(source);

    data.echo_stdout = echo_stdout;
    data.stdout_str = x_string_new(NULL);
    data.stdout_io = x_io_channel_unix_new(stdout_fd);
    x_io_channel_set_close_on_unref(data.stdout_io, TRUE);
    x_io_channel_set_encoding(data.stdout_io, NULL, NULL);
    x_io_channel_set_buffered(data.stdout_io, FALSE);
    source = x_io_create_watch(data.stdout_io, (XIOCondition)(X_IO_IN | X_IO_ERR | X_IO_HUP));
    x_source_set_callback(source, (XSourceFunc)child_read, &data, NULL);
    x_source_attach(source, context);
    x_source_unref(source);

    data.echo_stderr = echo_stderr;
    data.stderr_str = x_string_new(NULL);
    data.stderr_io = x_io_channel_unix_new(stderr_fd);
    x_io_channel_set_close_on_unref(data.stderr_io, TRUE);
    x_io_channel_set_encoding(data.stderr_io, NULL, NULL);
    x_io_channel_set_buffered(data.stderr_io, FALSE);
    source = x_io_create_watch(data.stderr_io, (XIOCondition)(X_IO_IN | X_IO_ERR | X_IO_HUP));
    x_source_set_callback(source, (XSourceFunc)child_read, &data, NULL);
    x_source_attach(source, context);
    x_source_unref(source);

    if (timeout) {
        source = x_timeout_source_new(0);
        x_source_set_ready_time(source, x_get_monotonic_time() + timeout);
        x_source_set_callback(source, (XSourceFunc)child_timeout, &data, NULL);
        x_source_attach(source, context);
        x_source_unref(source);
    }

    x_main_loop_run(data.loop);
    x_main_loop_unref(data.loop);
    x_main_context_unref(context);

    if (echo_stdout && test_tap_log && (data.stdout_str->len > 0)) {
        xboolean added_newline = FALSE;

        if (data.stdout_str->str[data.stdout_str->len - 1] != '\n') {
            x_string_append_c(data.stdout_str, '\n');
            added_newline = TRUE;
        }

        x_test_print_handler_full(data.stdout_str->str, TRUE, TRUE, 1);
        if (added_newline) {
            x_string_truncate(data.stdout_str, data.stdout_str->len - 1);
        }
    }

    test_trap_last_pid = pid;
    test_trap_last_status = data.child_status;
    test_trap_last_stdout = x_string_free(data.stdout_str, FALSE);
    test_trap_last_stderr = x_string_free(data.stderr_str, FALSE);

    x_clear_pointer(&data.stdout_io, x_io_channel_unref);
    x_clear_pointer(&data.stderr_io, x_io_channel_unref);
}

X_GNUC_BEGIN_IGNORE_DEPRECATIONS
xboolean x_test_trap_fork(xuint64 usec_timeout, XTestTrapFlags test_trap_flags)
{
    int errsv;
    int stdout_pipe[2] = { -1, -1 };
    int stderr_pipe[2] = { -1, -1 };

    test_trap_clear();
    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        errsv = errno;
        x_error("failed to create pipes to fork test program: %s", x_strerror(errsv));
    }

    test_trap_last_pid = fork();
    errsv = errno;

    if (test_trap_last_pid < 0) {
        x_error("failed to fork test program: %s", x_strerror(errsv));
    }

    if (test_trap_last_pid == 0) {
        int fd0 = -1;

        test_in_forked_child = TRUE;
        close (stdout_pipe[0]);
        close (stderr_pipe[0]);

        if (!(test_trap_flags & X_TEST_TRAP_INHERIT_STDIN)) {
            fd0 = x_open("/dev/null", O_RDONLY, 0);
            if (fd0 < 0)
                x_error("failed to open /dev/null for stdin redirection");
        }

        if (safe_dup2(stdout_pipe[1], 1) < 0 || safe_dup2(stderr_pipe[1], 2) < 0 || (fd0 >= 0 && safe_dup2(fd0, 0) < 0)) {
            errsv = errno;
            x_error("failed to dup2() in forked test program: %s", x_strerror(errsv));
        }

        if (fd0 >= 3) {
            close(fd0);
        }

        if (stdout_pipe[1] >= 3) {
            close(stdout_pipe[1]);
        }

        if (stderr_pipe[1] >= 3) {
            close(stderr_pipe[1]);
        }

        x_test_disable_crash_reporting();
        return TRUE;
    } else {
        test_run_forks++;
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        wait_for_child(test_trap_last_pid, stdout_pipe[0], !(test_trap_flags & X_TEST_TRAP_SILENCE_STDOUT), stderr_pipe[0], !(test_trap_flags & X_TEST_TRAP_SILENCE_STDERR), usec_timeout);
        return FALSE;
    }
}
X_GNUC_END_IGNORE_DEPRECATIONS

void x_test_trap_subprocess(const char *test_path, xuint64 usec_timeout, XTestSubprocessFlags test_flags)
{
    XPid pid;
    XPtrArray *argv;
    XSpawnFlags flags;
    XError *error = NULL;
    int stdout_fd, stderr_fd;

    x_assert((test_flags & (X_TEST_TRAP_INHERIT_STDIN | X_TEST_TRAP_SILENCE_STDOUT | X_TEST_TRAP_SILENCE_STDERR)) == 0);

    if (test_path) {
        if (!x_test_suite_case_exists(x_test_get_root(), test_path)) {
            x_error("x_test_trap_subprocess: test does not exist: %s", test_path);
        }
    } else {
        test_path = test_run_name;
    }

    if (x_test_verbose()) {
        if (test_tap_log) {
            x_print("subprocess: %s\n", test_path);
        } else {
            x_print("GTest: subprocess: %s\n", test_path);
        }
    }

    test_trap_clear();
    test_trap_last_subprocess = x_strdup(test_path);

    if (test_argv0 == NULL) {
        x_error("x_test_trap_subprocess() requires argv0 to be passed to x_test_init()");
    }

    argv = x_ptr_array_new();
    x_ptr_array_add(argv, (xpointer)test_argv0);
    x_ptr_array_add(argv, (xpointer)"-q");
    x_ptr_array_add(argv, (xpointer)"-p");
    x_ptr_array_add(argv, (xpointer)test_path);
    x_ptr_array_add(argv, (xpointer)"--XTestSubprocess");

    if (test_log_fd != -1) {
        char log_fd_buf[128];

        x_ptr_array_add(argv, (xpointer)"--XTestLogFD");
        x_snprintf(log_fd_buf, sizeof(log_fd_buf), "%d", test_log_fd);
        x_ptr_array_add(argv, log_fd_buf);
    }
    x_ptr_array_add(argv, NULL);

    flags = (XSpawnFlags)X_SPAWN_DO_NOT_REAP_CHILD;
    if (test_log_fd != -1) {
        flags = (XSpawnFlags)(flags | X_SPAWN_LEAVE_DESCRIPTORS_OPEN);
    }

    if (test_flags & X_TEST_TRAP_INHERIT_STDIN) {
        flags = (XSpawnFlags)(flags | X_SPAWN_CHILD_INHERITS_STDIN);
    }

    if (!x_spawn_async_with_pipes(test_initial_cwd, (char **)argv->pdata, NULL, flags, NULL, NULL, &pid, NULL, &stdout_fd, &stderr_fd, &error)) {
        x_error("x_test_trap_subprocess() failed: %s", error->message);
    }

    x_ptr_array_free(argv, TRUE);
    wait_for_child(pid, stdout_fd, !!(test_flags & X_TEST_SUBPROCESS_INHERIT_STDOUT), stderr_fd, !!(test_flags & X_TEST_SUBPROCESS_INHERIT_STDERR), usec_timeout);
}

xboolean x_test_subprocess(void)
{
    return test_in_subprocess;
}

xboolean x_test_trap_has_passed(void)
{
    return (WIFEXITED(test_trap_last_status) && WEXITSTATUS(test_trap_last_status) == 0);
}

xboolean x_test_trap_reached_timeout(void)
{
    return (WIFSIGNALED(test_trap_last_status) && WTERMSIG(test_trap_last_status) == SIGALRM);
}

static xboolean log_child_output(const xchar *process_id)
{
    xchar *escaped;

    if (WIFEXITED(test_trap_last_status)) {
        if (WEXITSTATUS(test_trap_last_status) == 0) {
            x_test_message("child process (%s) exit status: 0 (success)", process_id);
        } else {
            x_test_message("child process (%s) exit status: %d (error)", process_id, WEXITSTATUS(test_trap_last_status));
        }
    } else if (WIFSIGNALED(test_trap_last_status) && WTERMSIG(test_trap_last_status) == SIGALRM) {
        x_test_message("child process (%s) timed out", process_id);
    } else if (WIFSIGNALED(test_trap_last_status)) {
        const xchar *maybe_dumped_core = "";

#ifdef WCOREDUMP
        if (WCOREDUMP(test_trap_last_status)) {
            maybe_dumped_core = ", core dumped";
        }
#endif
        x_test_message("child process (%s) killed by signal %d (%s)%s", process_id, WTERMSIG(test_trap_last_status), x_strsignal(WTERMSIG(test_trap_last_status)), maybe_dumped_core);
    } else {
        x_test_message("child process (%s) unknown wait status %d", process_id, test_trap_last_status);
    }

    escaped = x_strescape(test_trap_last_stdout, NULL);
    x_test_message("child process (%s) stdout: \"%s\"", process_id, escaped);
    x_free(escaped);

    escaped = x_strescape(test_trap_last_stderr, NULL);
    x_test_message("child process (%s) stderr: \"%s\"", process_id, escaped);
    x_free(escaped);

    return TRUE;
}

void x_test_trap_assertions(const char *domain, const char *file, int line, const char *func, xuint64 assertion_flags, const char *pattern)
{
    xboolean must_pass = assertion_flags == 0;
    xboolean must_fail = assertion_flags == 1;
    xboolean match_result = 0 == (assertion_flags & 1);
    xboolean logged_child_output = FALSE;
    const char *stdout_pattern = (assertion_flags & 2) ? pattern : NULL;
    const char *stderr_pattern = (assertion_flags & 4) ? pattern : NULL;
    const char *match_error = match_result ? "failed to match" : "contains invalid match";
    char *process_id;

    if (test_trap_last_subprocess != NULL) {
        process_id = x_strdup_printf("%s [%d]", test_trap_last_subprocess, test_trap_last_pid);
    } else if (test_trap_last_pid != 0) {
        process_id = x_strdup_printf("%d", test_trap_last_pid);
    } else {
        x_error("x_test_trap_ assertion with no trapped test");
    }

    if (must_pass && !x_test_trap_has_passed()) {
        char *msg;
        logged_child_output = logged_child_output || log_child_output(process_id);

        msg = x_strdup_printf("child process (%s) failed unexpectedly", process_id);
        x_assertion_message(domain, file, line, func, msg);
        x_free(msg);
    }

    if (must_fail && x_test_trap_has_passed()) {
        char *msg;
        logged_child_output = logged_child_output || log_child_output(process_id);

        msg = x_strdup_printf("child process (%s) did not fail as expected", process_id);
        x_assertion_message(domain, file, line, func, msg);
        x_free(msg);
    }

    if (stdout_pattern && match_result == !x_pattern_match_simple(stdout_pattern, test_trap_last_stdout)) {
        char *msg;
        logged_child_output = logged_child_output || log_child_output(process_id);

        x_test_message("stdout was:\n%s", test_trap_last_stdout);

        msg = x_strdup_printf("stdout of child process (%s) %s: %s", process_id, match_error, stdout_pattern);
        x_assertion_message(domain, file, line, func, msg);
        x_free(msg);
    }

    if (stderr_pattern && match_result == !x_pattern_match_simple(stderr_pattern, test_trap_last_stderr)) {
        char *msg;

        logged_child_output = logged_child_output || log_child_output(process_id);

        x_test_message("stderr was:\n%s", test_trap_last_stderr);

        msg = x_strdup_printf("stderr of child process (%s) %s: %s", process_id, match_error, stderr_pattern);
        x_assertion_message(domain, file, line, func, msg);
        x_free(msg);
    }

    (void)logged_child_output;
    x_free(process_id);
}

static void xstring_overwrite_int(XString *gstring, xuint pos, xuint32 vuint)
{
    vuint = x_htonl(vuint);
    x_string_overwrite_len(gstring, pos, (const xchar*) &vuint, 4);
}

static void xstring_append_int(XString *gstring, xuint32 vuint)
{
    vuint = x_htonl(vuint);
    x_string_append_len(gstring, (const xchar *)&vuint, 4);
}

static void xstring_append_double(XString *gstring, double vdouble)
{
    union { double vdouble; xuint64 vuint64; } u;
    u.vdouble = vdouble;
    u.vuint64 = XUINT64_TO_BE(u.vuint64);
    x_string_append_len(gstring, (const xchar *)&u.vuint64, 8);
}

static xuint8 *x_test_log_dump(XTestLogMsg *msg, xuint *len)
{
    xuint ui;
    XString *gstring = x_string_sized_new(1024);

    xstring_append_int(gstring, 0);
    xstring_append_int(gstring, msg->log_type);
    xstring_append_int(gstring, msg->n_strings);
    xstring_append_int(gstring, msg->n_nums);
    xstring_append_int(gstring, 0);

    for (ui = 0; ui < msg->n_strings; ui++) {
        xuint l = strlen(msg->strings[ui]);
        xstring_append_int(gstring, l);
        x_string_append_len(gstring, msg->strings[ui], l);
    }

    for (ui = 0; ui < msg->n_nums; ui++) {
        xstring_append_double(gstring, msg->nums[ui]);
    }

    *len = gstring->len;
    xstring_overwrite_int(gstring, 0, *len);
    return (xuint8 *)x_string_free(gstring, FALSE);
}

static inline long double net_double(const xchar **ipointer)
{
    union { xuint64 vuint64; double vdouble; } u;
    xuint64 aligned_int64;
    memcpy(&aligned_int64, *ipointer, 8);
    *ipointer += 8;
    u.vuint64 = XUINT64_FROM_BE(aligned_int64);

    return u.vdouble;
}

static inline xuint32 net_int(const xchar **ipointer)
{
    xuint32 aligned_int;
    memcpy (&aligned_int, *ipointer, 4);
    *ipointer += 4;

    return x_ntohl(aligned_int);
}

static xboolean x_test_log_extract(XTestLogBuffer *tbuffer)
{
    xuint mlength;
    XTestLogMsg msg;
    const xchar *p = tbuffer->data->str;

    if (tbuffer->data->len < 4 * 5) {
        return FALSE;
    }

    mlength = net_int(&p);
    if (tbuffer->data->len < mlength) {
        return FALSE;
    }

    msg.log_type = (XTestLogType)net_int(&p);
    msg.n_strings = net_int(&p);
    msg.n_nums = net_int(&p);

    if (net_int(&p) == 0) {
        xuint ui;
        msg.strings = x_new0(xchar *, msg.n_strings + 1);
        msg.nums = x_new0(long double, msg.n_nums);
        for (ui = 0; ui < msg.n_strings; ui++) {
            xuint sl = net_int(&p);
            msg.strings[ui] = x_strndup(p, sl);
            p += sl;
        }

        for (ui = 0; ui < msg.n_nums; ui++) {
            msg.nums[ui] = net_double(&p);
        }

        if (p <= tbuffer->data->str + mlength) {
            x_string_erase(tbuffer->data, 0, mlength);
            tbuffer->msgs = x_slist_prepend(tbuffer->msgs, x_memdup2(&msg, sizeof (msg)));
            return TRUE;
        }

        x_free(msg.nums);
        x_strfreev(msg.strings);
    }

    x_error("corrupt log stream from test program");
    return FALSE;
}

XTestLogBuffer *x_test_log_buffer_new(void)
{
    XTestLogBuffer *tb = x_new0(XTestLogBuffer, 1);
    tb->data = x_string_sized_new(1024);
    return tb;
}

void x_test_log_buffer_free(XTestLogBuffer *tbuffer)
{
    x_return_if_fail(tbuffer != NULL);

    while (tbuffer->msgs) {
        x_test_log_msg_free(x_test_log_buffer_pop(tbuffer));
    }

    x_string_free(tbuffer->data, TRUE);
    x_free(tbuffer);
}

void x_test_log_buffer_push(XTestLogBuffer *tbuffer, xuint n_bytes, const xuint8 *bytes)
{
    x_return_if_fail(tbuffer != NULL);

    if (n_bytes) {
        xboolean more_messages;
        x_return_if_fail(bytes != NULL);
        x_string_append_len(tbuffer->data, (const xchar *)bytes, n_bytes);

        do {
            more_messages = x_test_log_extract(tbuffer);
        } while (more_messages);
    }
}

XTestLogMsg *x_test_log_buffer_pop(XTestLogBuffer *tbuffer)
{
    XTestLogMsg *msg = NULL;
    x_return_val_if_fail(tbuffer != NULL, NULL);

    if (tbuffer->msgs) {
        XSList *slist = x_slist_last(tbuffer->msgs);
        msg = (XTestLogMsg *)slist->data;
        tbuffer->msgs = x_slist_delete_link(tbuffer->msgs, slist);
    }

    return msg;
}

void x_test_log_msg_free(XTestLogMsg *tmsg)
{
    x_return_if_fail(tmsg != NULL);

    x_strfreev(tmsg->strings);
    x_free(tmsg->nums);
    x_free(tmsg);
}

static xchar *x_test_build_filename_va(XTestFileType file_type, const xchar *first_path, va_list ap)
{
    const xchar *pathv[16];
    xsize num_path_segments;

    if (file_type == X_TEST_DIST) {
        pathv[0] = test_disted_files_dir;
    } else if (file_type == X_TEST_BUILT) {
        pathv[0] = test_built_files_dir;
    } else {
        x_assert_not_reached();
    }

    pathv[1] = first_path;

    for (num_path_segments = 2; num_path_segments < X_N_ELEMENTS(pathv); num_path_segments++) {
        pathv[num_path_segments] = va_arg(ap, const char *);
        if (pathv[num_path_segments] == NULL) {
            break;
        }
    }

    x_assert_cmpint(num_path_segments, <, X_N_ELEMENTS(pathv));
    return x_build_filenamev((xchar **) pathv);
}

xchar *x_test_build_filename(XTestFileType file_type, const xchar *first_path, ...)
{
    va_list ap;
    xchar *result;

    x_assert(x_test_initialized());

    va_start(ap, first_path);
    result = x_test_build_filename_va(file_type, first_path, ap);
    va_end(ap);

    return result;
}

const xchar *x_test_get_dir(XTestFileType file_type)
{
    x_assert(x_test_initialized());

    if (file_type == X_TEST_DIST) {
        return test_disted_files_dir;
    } else if (file_type == X_TEST_BUILT) {
        return test_built_files_dir;
    }

    x_assert_not_reached();
}

const xchar *x_test_get_filename(XTestFileType file_type, const xchar *first_path, ...)
{
    va_list ap;
    XSList *node;
    xchar *result;

    x_assert(x_test_initialized()); {
    if (test_filename_free_list == NULL)
        x_error("x_test_get_filename() can only be used within testcase functions");
    }

    va_start(ap, first_path);
    result = x_test_build_filename_va(file_type, first_path, ap);
    va_end(ap);

    node = x_slist_prepend(NULL, result);

    do {
        node->next = *test_filename_free_list;
    } while (!x_atomic_pointer_compare_and_exchange(test_filename_free_list, node->next, node));

    return result;
}

const char *x_test_get_path(void)
{
    return test_run_name;
}
