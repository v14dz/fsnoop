/* event.h -- Functions handler for the Inotify mechanism.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#ifndef _FSNOOP_INOTIFY_H
#define _FSNOOP_INOTIFY_H

#include <sys/inotify.h>

#define MAX_MONITORED_FILES 	64

#define DFT_INOTIFY_BITS ( IN_ATTRIB | IN_CREATE | IN_DELETE | \
                           IN_DELETE_SELF | IN_MODIFY )

extern int monitor_init(void);

extern void monitor_update(char *);

extern int monitor_watch(int);

extern char *imask_to_str(uint32_t);

extern void print_event(int);

extern int snipe_fd(void);

extern char *get_fullpath_from_wd(void);

struct event_conv {
    uint32_t mask;
    char *str;
    char *c;
};

#endif
