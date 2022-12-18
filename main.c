/* main.c -- The main program.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "global.h"
#include "util.h"
#include "event.h"
#include "module.h"
#include "actions.h"
#include "id.h"

#define PROG_NAME               "fsnoop"
#define PROG_VERSION            "3.5"

/* print fatal error and exit. */
#define fatal(x...)   { fprintf(stderr, x); exit(1); }

/* same but display usage. */
#define fatal_u(x...) { fprintf(stderr, x); \
                        usage(); exit(1); }

/* Outputs the usage. 
 */
static void usage() {

    printf("Usage: %s [OPTIONS] [DIR1[,DIR2,...]] [-- COMMAND]\n" "\n"
           "  -d              Run as a daemon\n"
           "  -e              Monitor every Inotify events\n"
           "  -fd             Open file descriptors when it's possible\n"
           "  -k              Send SIGSTOP signal to the running process (COMMAND)\n"
           "  -o <filename>   Redirected output to a specific file\n"
           "  -p <paymod.so>  Load a payload module (DSO file)\n"
           "  -r              Monitor directory contents recursively\n"
           "  -t              Prefix each line with the time of day\n"
           "  -v              Display version\n"
           "\n"
           "If no DIR is specified, default writable directories such as /tmp,\n"
           "/var/tmp, /dev/shm, etc. are monitored.\n"
           "\n"
           "If COMMAND is specified, monitoring will occur only during the process\n"
           "duration.\n", PROG_NAME);
}

/* Displays opened file descriptor and launches a shell.  This is a signal
 * handler function called on ctrl-c while running the -fd option. 
 */
static void launch_shell(int signum) {

    char command[PATH_MAX + ARG_MAX];

    printf("\n\nHere are opened file descriptors.  You can display their "
           "contents by using the \"cat\" command.  For example, to display "
           "fd #4 use: \"cat <&4\"\n\n");

    sprintf(command, "/bin/ls -l /proc/%d/fd/; PS1=\"%s$ \" /bin/sh",
            getpid(), PROG_NAME);

    system(command);
    exit(0);
}

/* Creates filename and redirect stdout into it.  It's called when
 * specifying the -o option.
 */
static void set_output_file(char *filename) {

    int fd;
    char *dir;
    char **mon_dir;

    dir = dirname(filename);

    /* compare dir with g_params->files[] */
    mon_dir = g_params->files;

    while (*mon_dir) {

        if (!strcmp(dir, *mon_dir)) {

            fprintf(stderr,
                    "Error: Writing output file in a monitored dir is "
                    "not a good idea.\n");

            exit(1);
        }

        mon_dir++;
    }

    fd = open_output_file(filename);

    if (fd < 0) {
        fprintf(stderr, "Error: Cannot open output file.\n");
        exit(1);
    }

    /* bug fix: force a write on stdout before redirecting stdout to
     * filename. */
    fprintf(stdout, "Appending output to %s\n", filename);

    /* if stdout is buffered (ex: stdout = /dev/null), the line above will be
     * written in the output file, so we flush stdout before redefining it.
     */
    fflush(stdout);

    if (dup2(fd, STDOUT_FILENO) < 1) {
        fprintf(stderr, "Error: dup2() failed.\n");
        exit(1);
    }

    /* We disable buffering on stdout. */
    setbuf(stdout, NULL);

    close(fd);
}

/* Checks command line parameters (argv) and sets option masks (defined in
 * global.h).
 */
static int check_params(char **argv) {

    int i = 1, n = 0, file_nb = 0;
    char *ptr, *p, **cmd;

    while ((argv[i] != NULL) && strcmp(argv[i], "--")) {

        if ((strcmp(argv[i], "-h") == 0)
            || (strcmp(argv[i], "--help") == 0)) {
            usage();
            exit(0);
        }

        if ((strcmp(argv[i], "-v") == 0) ||
            (strcmp(argv[i], "--version") == 0)) {

            printf("%s version %s\n", PROG_NAME, PROG_VERSION);
            exit(0);
        }

        /* print full event names option? */
        if (strcmp("-e", argv[i]) == 0) {
            set_option(do_longevent);
            i++;
            continue;
        }

        /* time of day option? */
        if (strcmp("-t", argv[i]) == 0) {
            set_option(do_printdate);
            i++;
            continue;
        }

        /* kill option? */
        if (strcmp("-k", argv[i]) == 0) {
            set_option(do_kill);
            i++;

            if (argv[i] == NULL) {
                fprintf(stderr, "Error: kill option requires a file.\n");
                usage();
                exit(1);
            }

            if (!(g_params->target_file = strdup(argv[i]))) {
                fprintf(stderr, "check_params(): malloc failed.\n");
                exit(1);
            }

            i++;
            continue;
        }

        /* daemon option? */
        if (strcmp("-d", argv[i]) == 0) {
            set_option(do_runasdaemon);
            set_option(do_printdate);
            i++;
            continue;
        }

        /* openfd option? */
        if (strcmp("-fd", argv[i]) == 0) {
            set_option(do_openfd);
            i++;
            continue;
        }

        /* recursive option? */
        if (strcmp("-r", argv[i]) == 0) {
            set_option(do_recursive);
            i++;
            continue;
        }

        /* payload option? */
        if (strcmp("-p", argv[i]) == 0) {
            set_option(do_payload);
            i++;

            if (argv[i] == NULL) {
                fprintf(stderr,
                        "Error: payload option requires a dynamic library"
                        " to load.\n");
                usage();
                exit(1);
            }

            if (file_exists(argv[i]) != 0) {
                fprintf(stderr, "Dynamic library %s does not exist.\n",
                        argv[i]);
                exit(1);
            }

            if (!(g_paymod = malloc(sizeof(struct paymod)))) {
                fprintf(stderr, "check_params(): malloc failed.\n");
                exit(1);
            }

            g_paymod->dso = get_path(argv[i]);

            i++;

            continue;
        }

        /* output option? */
        if (strcmp("-o", argv[i]) == 0) {
            i++;

            if (argv[i] == NULL) {
                fprintf(stderr, "Error: output option requires a file.\n");
                usage();
                exit(1);
            }

            g_params->output_file = argv[i];
            i++;

            continue;
        }

        /* specific directories? */
        p = strdup(argv[i]);

        while ((ptr = strchr(p, ',')) != NULL) {
            *ptr = '\0';

            if (strlen(p) > NAME_MAX)
                fatal("Too much characters in filename.\n");

            clean_path_str(p);

            g_params->files[n] = strdup(p);
            p = ptr + 1;
            n++;
            file_nb++;          /* a good opportunity to count files */
        }

        clean_path_str(p);

        g_params->files[n] = strdup(p);
        file_nb++;
        i++;
    }

    if (get_option(do_openfd) && (get_option(do_runasdaemon) ||
                                  get_option(do_kill)
                                  || get_option(do_payload)))
        fatal_u("You can't use -fd with -k or -d or -p.\n");

    if ((get_option(do_runasdaemon)) && (g_params->output_file == NULL))
        fatal_u("You can't use -d without specifying an output file.\n");

    /* at this time, if no file is selected for monitoring, this is maybe
     * because we use a payload module or the kill option. */
    if (file_nb == 0) {

        if (get_option(do_kill)) {

            /* we use the kill option, the file to monitor is specified as
             * g_params->target_file but may not exist. */

            g_params->files[n] =
                existing_parent_dir(g_params->target_file);
            file_nb++;
        }

        if (get_option(do_payload)) {

            /* we use a payload module, the file to monitor is specified inside
             * the module. */

            load_module();
            g_params->files[n] = existing_parent_dir(g_paymod->file);
            file_nb++;

        }

        if (file_nb == 0) {

            /* Still no file to monitor so we we feed g_params->files[] with
             * writable_dirs[]. */

            for (n = 0; writable_dirs[n] != NULL; n++) {
                g_params->files[n] = writable_dirs[n];
                file_nb++;
            }

        }
    }

    /* Inotify needs existing files to succeed.  We will check that now. */
    for (n = 0; g_params->files[n] != NULL; n++) {
        if (file_exists(g_params->files[n]) != 0)
            fatal
                ("File %s does not exist.\nInotify needs existing files or "
                 "directories.\n", g_params->files[n]);
    }

    /* We have finished to parse arguments and files.  Now we have two
     * solution:
     * 
     *   argv[i] = NULL   => no command
     *   argv[i] = --     => A command is specified at argv[i+1]
     */

    if (argv[i] == NULL) {

        /* it's stupid to use -k without specifying a command.  Because program
         * won't know which PID to kill! */

        if (get_option(do_kill))
            fatal_u("You can't use -k without specifying a command\n");

        return 0;
    }

    /* Here we know that parameter argv[i] contains "--".  We need to check
     * that cmd wasn't ommited */

    if (!argv[i + 1])
        fatal_u("You specified \"--\" but none command is specified.\n");

    cmd = argv + i + 1;

    /* cmd[0] now contains the exe name.  We need to find out if this is
     * absolute path or not */
    if (**cmd != '/' && (**cmd != '.' && *(*cmd + 1) != '/')) {

        /* Absolute path wasn't specified.  Lets find where this binary is. */
        ptr = search_binary(cmd[0]);

        if (ptr == NULL)
            fatal
                ("Command can't be found in the PATH.  Try absolute path.\n");

        g_params->cmd->cmdline[0] = strdup(ptr);

    } else {

        if (file_exists(cmd[0]) != 0)
            fatal("Command does not exist.\n");

        g_params->cmd->cmdline[0] = strdup(cmd[0]);
    }

    /* Global attribute g_params->cmd->cmdline[0] is now set, we now need to copy the
     * other arguments. */
    for (i = 1; i < ARG_MAX && cmd[i] != NULL; i++)
        g_params->cmd->cmdline[i] = cmd[i];

    return 0;
}

/* Runs main program.
 */
int main(int argc, char **argv) {

    int inot_pid, pid_status;

    /* first of all, we need space to store our global parameters. */
    if (!(g_params = malloc(sizeof(struct parameters)))) {
        fprintf(stderr, "Error: malloc() failed.\n");
        exit(1);
    }

    if (!(g_params->cmd = malloc(sizeof(struct command)))) {
        fprintf(stderr, "Error: malloc() failed.\n");
        exit(1);
    }

    check_params(argv);

    /* We now have global attributes do_kill, g_params->files[], g_params->cmd->cmdline[],
     * g_params->output_file available.  We're ready. */

    if (get_option(do_openfd)) {
        signal(SIGINT, launch_shell);
    }

    if (g_params->output_file)
        set_output_file(g_params->output_file);

    if (get_option(do_runasdaemon))
        runas_daemon();

    g_pw_list = load_passwd();
    g_gr_list = load_group();

    /* If do_payload AND proc_name are settled, we don't use Inotify.  So we
     * directly launch an action.
     */
    if (get_option(do_payload) && g_paymod->proc_name) {

        action_payload_proc();

    } else if (g_params->cmd->cmdline[0] != NULL) {

        g_ifd = monitor_init();
        g_params->cmd->pid = exec_program(g_params->cmd->cmdline);
        inot_pid = monitor_watch(1);

        waitpid(g_params->cmd->pid, &pid_status, 0);
        sleep(1);
        kill(inot_pid, SIGTERM);
    } else {

        g_ifd = monitor_init();
        monitor_watch(0);
    }

    return 0;
}
