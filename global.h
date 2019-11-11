/* global.h -- Contains global variables.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#ifndef _FSNOOP_GLOBAL_H
#define _FSNOOP_GLOBAL_H

#include <sys/types.h>
#include <sys/inotify.h>
#include <linux/limits.h>

#include "event.h"
#include "id.h"

/* Option's mask. 
 */
#define do_kill                 0x01
#define do_printdate            0x02
#define do_runasdaemon          0x04
#define do_openfd               0x08
#define do_recursive            0x10
#define do_payload              0x20
#define do_longevent            0x40

/* Some macros to manage the options above (g_options stores the options).
 */
#define get_option(x)           (g_options  & x)
#define set_option(x)           (g_options |= x)
#define unset_option(x)         (g_options ^= x)

extern int8_t g_options;

struct command {
    char *cmdline[PATH_MAX + ARG_MAX];
    pid_t pid;
};

struct parameters {
    char *files[MAX_MONITORED_FILES], *target_file, *output_file;
    void (*run_action)();
    struct command *cmd;
};

extern struct parameters *g_params;

extern char *writable_dirs[];

extern struct paymod *g_paymod;

extern struct inotify_event *g_event;

extern int g_ifd;

extern struct id_name *g_pw_list;
extern struct id_name *g_gr_list;

#endif
