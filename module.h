/* module.h -- Functions that handle payload modules.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#ifndef _FSNOOP_HANDLE_MODULE_H
#define _FSNOOP_HANDLE_MODULE_H

#define PIDSTR   "HEREPID"

struct paymod {
  void (*payload) ();
  char *title, *dso, *file, *proc_name;
  int *count;
  uint32_t *mask;
};

extern void load_module(void);

extern void unload_module(void);

extern void substitute_process_id(void);

#endif
