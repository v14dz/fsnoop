/* global.c -- Contains global variables.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <stdlib.h>

#include "global.h"
#include "event.h"

/* Default user's writable directories on a Debian system.  If it is
 * compiled on a RedHat system, "/var/lock" must be removed (not world
 * writable).
 */
char *writable_dirs[] = {
    "/dev/shm",
    "/tmp",
    "/var/lock",
    "/var/tmp",
    NULL
};

/* 8-bits variable to store options. 
 */
int8_t g_options = 0;

/* Parameters information.
 */
struct parameters *g_params = NULL;

/* Inotify file descriptor.
 */
int g_ifd;

/* Inotify events.
 */
struct inotify_event *g_event;

/* Payload module information. 
 */
struct paymod *g_paymod = NULL;

struct id_name *g_pw_list;
struct id_name *g_gr_list;
