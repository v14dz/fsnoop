/* actions.c -- Actions executed on each event.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#include <sys/inotify.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#include "actions.h"
#include "event.h"
#include "global.h"
#include "module.h"

static char file[NAME_MAX];

/* Called when do_recursive option is settled.
 */
void action_recursive() {

    /* if new event is a dir we update the Inotify watch descriptor. */
    if (g_event->mask & IN_ISDIR) {

        sprintf(file, "%s/%s", get_fullpath_from_wd(), g_event->name);
        monitor_update(file);
    }

    print_event(0);
}

/* Called when do_openfd is settled.
 */
void action_openfd() {

    print_event(snipe_fd());
}

/* Called when do_recursive and do_openfd options are settled.
 */
void action_ropenfd() {

    /* if new event is a dir we update the Inotify watch descriptor. */
    if (g_event->mask & IN_ISDIR) {

        sprintf(file, "%s/%s", get_fullpath_from_wd(), g_event->name);
        monitor_update(file);

    } else {

        print_event(snipe_fd());
    }
}

/* Called by default (no option settled).
 */
void action_normal_output() {

    print_event(0);
}

/* Called when do_kill is settled. 
 */
void action_kill() {

    sprintf(file, "%s/%s", get_fullpath_from_wd(), g_event->name);

    if (strncmp(file, g_params->target_file, strlen(g_params->target_file))
        == 0) {

        kill(g_params->cmd->pid, 19);
        print_event(0);
        printf("*** PID %d stopped, type [Enter] to resume execution: ",
               g_params->cmd->pid);

        /* we don't want to send SIGSTOP anymore, so we can now return to a
         * basic action. */
        g_params->run_action = &action_normal_output;

        getchar();

        kill(g_params->cmd->pid, 18);
        printf("*** PID %d resumed ...\n", g_params->cmd->pid);
    }

    /* If targeted file is /var/tmp/abc/def and /var/tmp/abc does not exist,
     * we want to update the watch descriptor. */
    if (g_event->mask & IN_ISDIR) {
        monitor_update(file);
    }
}

/* Called when do_payload is settled. 
 */
void action_payload() {

    sprintf(file, "%s/%s", get_fullpath_from_wd(), g_event->name);

    if (strncmp(file, g_paymod->file, strlen(g_paymod->file)) == 0) {

        if ((*g_paymod->count) == 0) {

            g_paymod->payload();        /* execute evil code */
            unload_module();    /* unload module (execute "destructor" routine) */
            exit(0);            /* we've done. */

        } else {

            (*g_paymod->count)--;

        }
    }

    /* If targeted file is /var/tmp/abc/def and /var/tmp/abc does not exist,
     * we want to update the watch descriptor. */
    if (g_event->mask & IN_ISDIR) {
        monitor_update(file);
    }
}

/* Called when do_payload and g_paymod->proc_name are settled. 
 */
void action_payload_proc() {

    /* need to wait for the process to start, and substite the PID in the
     * filename specified in the paymod.*/
    substitute_process_id();

    g_paymod->payload();        /* execute evil code */
    unload_module();            /* unload module (execute "destructor" routine) */
    exit(0);                    /* we've done. */
}
