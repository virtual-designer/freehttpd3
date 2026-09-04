#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "assert.h"
#include "libtest.h"

static struct option const long_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "report", required_argument, NULL, 'r' },
    { "color", required_argument, NULL, 'c' },
    { NULL, 0, NULL, 0 },
};

static const char *short_options = "hr:c:";

enum test_status
{
    STATUS_PASS = 1,
    STATUS_FAIL,
    STATUS_RUN,
};

static const char *test_status_lut[] = {
    [STATUS_PASS] = "PASS",
    [STATUS_FAIL] = "FAIL",
    [STATUS_RUN] = "RUN",
};

static const char *test_status_color_lut[] = {
    [STATUS_PASS] = "\033[1;32m",
    [STATUS_FAIL] = "\033[1;31m",
    [STATUS_RUN] = "\033[2;37m",
};

extern struct libtest_config LIBTEST_CONFIG_SYMBOL;
static size_t failed_count = 0, passed_count = 0;
static bool enable_color = false;
static bool enable_verbose_mode = false;
static char *report_file_path = NULL;
static char *argv0_src = NULL;

const char **program_argv = NULL;
int program_argc = 0;

const char *libtest_suite_name = NULL;
const char *libtest_test_case_name = NULL;

static void
print_status (const char *status, int pad_len, const char *pad,
              const char *color, const char *text)
{
    fprintf (stdout, "  %*s%s%-8s%s %s\n", pad_len, pad,
             enable_color ? color : "", status, enable_color ? "\033[0m" : "",
             text);
}

static void
print_test_case_status (const char *name,
                        enum test_status status)
{
    fprintf (stdout, "  %s%-8s%s %s::%s%s%s\n",
             enable_color ? test_status_color_lut[status] : "",
             test_status_lut[status], enable_color ? "\033[0m" : "", libtest_suite_name,
             enable_color ? "\033[1;38m" : "", name,
             enable_color ? "\033[0m" : "");
}

static void
print_summary (void)
{
    fprintf (stdout, "  %s%-8s%s %s - %s%zu%s passed, %s%zu%s failed\n",
             enable_color ? "\033[2;37m" : "", "SUMMARY",
             enable_color ? "\033[0m" : "", libtest_suite_name,
             enable_color ? "\033[1;32m" : "", passed_count,
             enable_color ? "\033[0m" : "", enable_color ? "\033[1;31m" : "",
             failed_count, enable_color ? "\033[0m" : "");
}

static const char *
get_signal_name (int sig)
{
    switch (sig)
    {
#ifdef SIGHUP
        case SIGHUP:
            return "SIGHUP";
#endif
#ifdef SIGINT
        case SIGINT:
            return "SIGINT";
#endif
#ifdef SIGQUIT
        case SIGQUIT:
            return "SIGQUIT";
#endif
#ifdef SIGILL
        case SIGILL:
            return "SIGILL";
#endif
#ifdef SIGTRAP
        case SIGTRAP:
            return "SIGTRAP";
#endif
#ifdef SIGABRT
        case SIGABRT:
            return "SIGABRT";
#endif
#ifdef SIGBUS
        case SIGBUS:
            return "SIGBUS";
#endif
#ifdef SIGFPE
        case SIGFPE:
            return "SIGFPE";
#endif
#ifdef SIGKILL
        case SIGKILL:
            return "SIGKILL";
#endif
#ifdef SIGUSR1
        case SIGUSR1:
            return "SIGUSR1";
#endif
#ifdef SIGSEGV
        case SIGSEGV:
            return "SIGSEGV";
#endif
#ifdef SIGUSR2
        case SIGUSR2:
            return "SIGUSR2";
#endif
#ifdef SIGPIPE
        case SIGPIPE:
            return "SIGPIPE";
#endif
#ifdef SIGALRM
        case SIGALRM:
            return "SIGALRM";
#endif
#ifdef SIGTERM
        case SIGTERM:
            return "SIGTERM";
#endif
#ifdef SIGCHLD
        case SIGCHLD:
            return "SIGCHLD";
#endif
#ifdef SIGCONT
        case SIGCONT:
            return "SIGCONT";
#endif
#ifdef SIGSTOP
        case SIGSTOP:
            return "SIGSTOP";
#endif
#ifdef SIGTSTP
        case SIGTSTP:
            return "SIGTSTP";
#endif
#ifdef SIGTTIN
        case SIGTTIN:
            return "SIGTTIN";
#endif
#ifdef SIGTTOU
        case SIGTTOU:
            return "SIGTTOU";
#endif
#ifdef SIGURG
        case SIGURG:
            return "SIGURG";
#endif
#ifdef SIGXCPU
        case SIGXCPU:
            return "SIGXCPU";
#endif
#ifdef SIGXFSZ
        case SIGXFSZ:
            return "SIGXFSZ";
#endif
#ifdef SIGVTALRM
        case SIGVTALRM:
            return "SIGVTALRM";
#endif
#ifdef SIGPROF
        case SIGPROF:
            return "SIGPROF";
#endif
#ifdef SIGWINCH
        case SIGWINCH:
            return "SIGWINCH";
#endif
#ifdef SIGPOLL
        case SIGPOLL:
            return "SIGPOLL";
#endif
#ifdef SIGSYS
        case SIGSYS:
            return "SIGSYS";
#endif

        default:
            return "UNKNOWN";
    }
}

static bool
run_hook (const libtest_test_callback_t hook)
{
    if (!hook)
        return true;

    set_assert_fail_count (0);
    set_assert_success_count (0);

    return hook () == 0 && get_assert_fail_count () == 0;
}

static void
usage (void)
{
    fprintf (stdout,
             "Usage: %s <-r|--report=PATH> [-c|--color=<1|0>] [ARGV...]\n",
             argv0_src);
}

int
main (int argc, char **argv)
{   
    enable_color = isatty (STDOUT_FILENO) && isatty (STDIN_FILENO);

    for (;;)
    {
        int longind = 0;
        int c = getopt_long (argc, argv, short_options, long_options, &longind);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                usage ();
                exit (EXIT_SUCCESS);

            case 'c':
                enable_color = !strcmp (optarg, "1");
                break;

            case 'r':
                report_file_path = strdup (optarg);
                break;

            default:
                exit (EXIT_FAILURE);
        }
    }

    if (!report_file_path)
    {
        fprintf (stderr, "%s: a report file path must be provided\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    program_argc = argc - optind;
    program_argv = (const char **) argv + optind;

    const struct libtest_config *config = &LIBTEST_CONFIG_SYMBOL;
    const struct libtest_test_case **test_cases = config->test_cases;

    argv0_src = strdup (argv[0]);
    char *argv0 = argv0_src;
    libtest_suite_name = basename (argv0);

    int fds[2];

    if (pipe (fds) != 0)
    {
        fprintf (stderr, "%s: unable to create pipe: %s\n", argv[0],
                 strerror (errno));
        exit (1);
    }

    fflush (stdout);

    pid_t pid = fork ();

    if (pid < 0)
    {
        fprintf (stderr, "%s: unable to create child process: %s\n", argv[0],
                 strerror (errno));
        exit (1);
    }

    if (pid == 0)
    {
        bool exit_err = false;

        free (report_file_path);
        close (fds[0]);
        setvbuf (stdout, NULL, _IOLBF, 0);

        print_status (enable_verbose_mode ? "SUITE" : "SUITE   ", 0, "",
                      "\033[1;34m", libtest_suite_name);

        if (!run_hook (config->before_all))
            exit_err = true;

        for (size_t i = 0; test_cases[i]; i++)
        {
            const char *name = test_cases[i]->name;
            libtest_test_case_name = name;

            if (!run_hook (config->before_each))
                exit_err = true;

            set_assert_fail_count (0);
            set_assert_success_count (0);
            print_test_case_status (name, STATUS_RUN);

            const int rc = test_cases[i]->callback ();

            const bool passed = rc == 0 && get_assert_fail_count () == 0;

            print_test_case_status (name,
                                    passed ? STATUS_PASS : STATUS_FAIL);

            if (!passed)
                exit_err = true;

            if (write (fds[1], &passed, sizeof passed) != sizeof passed)
            {
                fprintf (stderr, "%s: unable to report '%s': %s\n", libtest_suite_name,
                         name, strerror (errno));
                exit_err = true;
            }

            if (!run_hook (config->after_each))
                exit_err = true;
        }

        if (!run_hook (config->after_all))
            exit_err = true;

        close (fds[1]);
        free (argv0_src);
        return exit_err ? 1 : 0;
    }
    else
    {
        close (fds[1]);

        FILE *outfile = fopen (report_file_path, "w");

        if (!outfile)
        {
            fprintf (stderr, "%s: unable to open '%s': %s\n", argv[0],
                     report_file_path, strerror (errno));
            free (argv0_src);
            free (report_file_path);
            kill (pid, SIGKILL);
            exit (1);
        }

        free (report_file_path);
        setvbuf (outfile, NULL, _IOLBF, 0);

        bool value = false;

        while (read (fds[0], &value, sizeof value) == sizeof value)
        {
            if (value)
                passed_count++;
            else
                failed_count++;
        }

        int status = 0;

        if (waitpid (pid, &status, 0) < 0)
        {
            fprintf (stderr, "%s: unable to wait for child process: %s\n",
                     argv[0], strerror (errno));
            fclose (outfile);
            free (argv0_src);
            exit (1);
        }

        close (fds[0]);

        print_summary ();

        int exit_code = WEXITSTATUS (status);
        int signal = WTERMSIG (status);

        fprintf (outfile, "PASSED %zu;", passed_count);
        fprintf (outfile, "FAILED %zu;", failed_count);

        if (WIFEXITED (status))
            fprintf (outfile, "EXIT %d;\n", exit_code);
        else
            fprintf (outfile, "SIGNAL %s;\n", get_signal_name (signal));

        fclose (outfile);
        free (argv0_src);
        exit (WIFEXITED (status) && exit_code == 0 && failed_count == 0 ? 0
                                                                        : 1);
    }

    return 0;
}
