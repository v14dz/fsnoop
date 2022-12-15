/* event.c -- Functions handler for the Inotify mechanism.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#include <sys/inotify.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>

#include "event.h"
#include "actions.h"
#include "util.h"
#include "global.h"
#include "module.h"
#include "id.h"

/* Structure to help with Inotify event conversions. 
 */
static struct event_conv ec[] = {

    { IN_ATTRIB, "IN_ATTRIB       ", "M" },
    { IN_CREATE, "IN_CREATE       ", "C" },
    { IN_DELETE, "IN_DELETE       ", "D" },
    { IN_MODIFY, "IN_MODIFY       ", "U" },
    { IN_ACCESS, "IN_ACCESS       ", "x" },
    { IN_CLOSE_WRITE, "IN_CLOSE_WRITE  ", "x" },
    { IN_CLOSE_NOWRITE, "IN_CLOSE_NOWRITE", "x" },
    { IN_DELETE_SELF, "IN_DELETE_SELF  ", "x" },
    { IN_MOVE_SELF, "IN_MOVE_SELF    ", "x" },
    { IN_MOVED_FROM, "IN_MOVED_FROM   ", "x" },
    { IN_MOVED_TO, "IN_MOVED_TO     ", "x" },
    { IN_OPEN, "IN_OPEN         ", "x" },
    { 0, NULL, "" }
};

/* For performance purposes, this will contain file names of monitored
 * file, indexed with the watch descriptor (as a simple hash table).
 */
static char *hash_table[MAX_MONITORED_FILES];

/* Initializes an Inotify instance and adds a new watch for each file to
 * monitor (global attribute g_params->files[]).  On success, a new file
 * descriptor is returned.  On error, -1 is returned. 
 */
int monitor_init() {

    int fd, wd, i = 0, file_nb = 0;
    uint32_t imask;

    imask = get_option(do_longevent) ? IN_ALL_EVENTS : DFT_INOTIFY_BITS;

    /* initialise inotify instance */
    fd = inotify_init();

    if (fd < 0)
        return -1;

    /* don't use default mask if payload is loaded. */
    if (get_option(do_payload))
        imask = *g_paymod->mask;

    for (i = 0; g_params->files[i] != NULL; i++) {
        wd = inotify_add_watch(fd, g_params->files[i], imask);

        /* if there's problem to create an inotify watch descriptor with a
         * file, we just ignore it. */
        if (wd < 0) {
            fprintf(stderr,
                    "WARNING: error occured while creating inotify "
                    "watch for %s.  This file will be ignored.\n",
                    g_params->files[i]);
            continue;
        }

        hash_table[wd] = strdup(g_params->files[i]);

        file_nb++;              /* number of file that we now monitor. */
    }

    if (file_nb == 0) {
        fprintf(stderr, "No file to monitor.\n");
        return -1;
    }

    return fd;
}

/* Transforms an Inotify mask into a string.  If the value isn't
 * recognized, it returns "x".  If do_longevent is set, it returns the full
 * Inotify event's name (IN_ATTRIB, IN_CREATE, etc.).  If do_longevent
 * isn't set, it returns a one char string (M, C, etc.).
 */
static char *action_from_mask(uint32_t mask) {

    int i;

    for (i = 0; ec[i].str != NULL; i++)
        if (mask & ec[i].mask)
            return get_option(do_longevent) ? ec[i].str : ec[i].c;

    return "?";
}

char *get_fullpath_from_wd() {

    return hash_table[g_event->wd];
}

static char get_type_from_mask(uint32_t mask) {

    return (mask & IN_ISDIR) ? 'D' : 'F';
}

char *imask_to_str(uint32_t mask) {

    int i;
    void *result;
    char *ptr;

    result = malloc(200);
    ptr = result;

    if (!result) {
        fprintf(stderr, "imask_to_str(): malloc failed.\n");
        exit(1);
    }

    for (i = 0; ec[i].str != NULL; i++)
        if (ec[i].mask & mask) {
            strcat((char *) result, ec[i].str);

            /* remove white spaces at the end of the event name. */
            if ((ptr = strchr(ptr, ' ')))
                *ptr++ = '\0';

            strcat((char *) result, " ");
        }

    if ((ptr = strrchr((char *) result, ' ')))
        *ptr = '\0';

    return result;
}

void print_event(int fd) {

    char filename[NAME_MAX];
    struct stat st;

    sprintf(filename, "%s/%s", get_fullpath_from_wd(), g_event->name);

    if ((!get_option(do_kill)) && lstat(filename, &st) == 0) {

        /* lstat() worked, output will look like this: */
        if (!get_option(do_printdate)) {

            printf("[%s] %s %d %s %s %d  %s %s",
                   action_from_mask(g_event->mask),
                   sym_perm(st.st_mode), (int) st.st_nlink,
                   name_from_id(g_pw_list, st.st_uid),
                   name_from_id(g_gr_list, st.st_gid), (int) st.st_size,
                   display_ascii_date(st.st_mtime), filename);

        } else {

            printf("%s [%s] %s %d %s %s %d  %s %s", display_current_time(),
                   action_from_mask(g_event->mask),
                   sym_perm(st.st_mode), (int) st.st_nlink,
                   name_from_id(g_pw_list, st.st_uid),
                   name_from_id(g_gr_list, st.st_gid), (int) st.st_size,
                   display_ascii_date(st.st_mtime), filename);
        }

    } else {

        /* minimal output */
        if (!get_option(do_printdate)) {

            printf("[%s] %c %s", action_from_mask(g_event->mask),
                   get_type_from_mask(g_event->mask), filename);

        } else {

            printf("%s [%s] %c %s", display_current_time(),
                   action_from_mask(g_event->mask),
                   get_type_from_mask(g_event->mask), filename);
        }
    }

    if (fd > 0)
        printf(" (opened fd=%d)\n", fd);
    else
        printf("\n");
}

/* open the new file when it's possible. 
 */
int snipe_fd() {

    char filename[NAME_MAX];

    if ((g_event->mask & IN_ATTRIB) || (g_event->mask & IN_CREATE)) {

        sprintf(filename, "%s/%s", get_fullpath_from_wd(), g_event->name);

        return open(filename, O_RDONLY);
    }

    return 0;
}

void monitor_update(char *newfile) {

    int wd;

    wd = inotify_add_watch(g_ifd, newfile, DFT_INOTIFY_BITS);

    hash_table[wd] = strdup(newfile);
}

int monitor_watch(int do_fork) {

    char *buf;
    int len, bs, i, child_pid;

    if (do_fork && (child_pid = fork()))
        return child_pid;

    bs = 10240 * (sizeof(struct inotify_event) + 16);

    buf = malloc(bs);

    if (!buf) {
        fprintf(stderr, "monitor_watch(): malloc failed.\n");
        exit(1);
    }

    for (i = 0; g_params->files[i]; i++)
        printf("[+] monitor %s\n", g_params->files[i]);

    if (get_option(do_openfd)) {
        printf
            ("[+] As then \"-fd\" option is being used, you can launch new shell by\n"
             "    using \"ctrl-c\" and explore opened file descriptors.\n");
    }

    /* we know all program options, we can now initialize the g_params->run_action
     * function pointer. */

    if (get_option(do_payload)) {

        g_params->run_action = action_payload;

    } else if (get_option(do_recursive) && get_option(do_openfd)) {

        g_params->run_action = &action_ropenfd;

    } else if (get_option(do_recursive)) {

        g_params->run_action = &action_recursive;

    } else if (get_option(do_openfd)) {

        g_params->run_action = &action_openfd;

    } else if (get_option(do_kill)) {

        g_params->run_action = &action_kill;

    } else {

        g_params->run_action = &action_normal_output;

    }

    for (;;) {

        i = 0;

        len = read(g_ifd, buf, bs);

        while (i < len) {

            g_event = (struct inotify_event *) &buf[i];

            if (g_event->len && run_action) // check that run_action is not null
                g_params->run_action();

            i += sizeof(struct inotify_event) + g_event->len;
        }
    }

    return 0;
}
