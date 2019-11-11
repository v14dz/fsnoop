/* module.c -- Functions that handle payload modules.
 *
 * This file is part of fsnoop.
 * 
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <signal.h>
#include <string.h>

#include "event.h"
#include "global.h"
#include "module.h"
#include "util.h"

/* Outputs information about current module. 
 */
#define mprintf(format, ...) printf("[+] %s: "format, g_paymod->dso, \
                             ##__VA_ARGS__)

int signal_caught = 0;
static void *handle;

/* When ctrl-c is typed, this function is called.  It exits properly by
 * first unloading module. 
 */
static void ctrlc_handler() {

    signal_caught = 1;

    printf("\n[+] Signal caught.\n");
    unload_module();

    exit(0);
}

/* Unloads the module.
 */
void unload_module(void) {

    if (!signal_caught)
        mprintf("Exploitation done.\n");

    mprintf("Unloading module.\n");

    if (dlclose(handle) != 0)
        mprintf("Error: dlclose() failed.\n");
}

/* Loads the DSO, retrieve module parameters and feed the g_paymod
 * structure.
 */
void load_module(void) {

    if (!(handle = dlopen(g_paymod->dso, RTLD_LAZY))) {
        mprintf("Error: dlopen() failed.\n");
        exit(1);
    }

    if (!(g_paymod->title = dlsym(handle, "title"))) {
        g_paymod->title = strdup("** Untitled paymod **");
    }

    if (!(g_paymod->file = dlsym(handle, "file"))) {
        g_paymod->file = strdup("/dev/null");
    }

    g_paymod->proc_name = dlsym(handle, "proc_name");

    if (!(g_paymod->count = (int *) dlsym(handle, "count"))) {
        /* count symbol not found, we set a default one (0). */
        g_paymod->count = malloc(sizeof(int));

        if (!g_paymod->count) {
            fprintf(stderr, "load_module(): malloc failed.\n");
            exit(1);
        }

        *g_paymod->count = 0;
    }

    if (!(g_paymod->mask = (uint32_t *) dlsym(handle, "mask"))) {
        /* mask symbol not found, we set a default one (DFT_INOTIFY_BITS) */
        g_paymod->mask = malloc(sizeof(uint32_t));

        if (!g_paymod->mask) {
            fprintf(stderr, "load_module(): malloc failed.\n");
            exit(1);
        }

        *g_paymod->mask = DFT_INOTIFY_BITS;
    }

    if (!(g_paymod->payload = dlsym(handle, "payload"))) {
        mprintf("Error: payload symbol not found.\n");
        exit(1);
    }

    mprintf("%s\n", g_paymod->title);

    if (!g_paymod->proc_name) {

        mprintf("payload=[%p] file=[%s] mask=[%s] count=[%d]\n",
                g_paymod->payload, g_paymod->file,
                imask_to_str(*g_paymod->mask), *g_paymod->count);

    } else {

        if (strstr(g_paymod->file, PIDSTR)) {
            mprintf("payload=[%p] file=[%s]\n",
                    g_paymod->payload, g_paymod->file);

            mprintf("waiting for command: \"%s\"\n", g_paymod->proc_name);

        } else if (strstr(g_paymod->file, "/dev/null")) {

            mprintf("payload=[%p]\n", g_paymod->payload);
        } else {

            mprintf("payload=[%p] file=[%s]\n", g_paymod->payload,
                    g_paymod->file);
        }
    }

    /* now the module is correctly loaded, destructor routines should be
     * executed on ctrl-c. */
    signal(SIGINT, ctrlc_handler);
}

/* Waits for the process name g_paymod->proc_name and replaces the PIDSTR
 * string by the process ID in the g_paymod->file. 
 */
void substitute_process_id(void) {

    char *pid;

    pid = loop_pgrep(g_paymod->proc_name);

    sub_str(PIDSTR, pid, g_paymod->file);
}
