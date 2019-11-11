/* util.h -- Contains utility functions.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#ifndef _FSNOOP_UTIL_H
#define _FSNOOP_UTIL_H

#include <sys/types.h>

extern int open_output_file(char *);

extern int file_exists(char *);

extern char *get_path(char *);

extern char *existing_parent_dir(char *);

extern char *search_binary(char *);

extern char *sym_perm(mode_t);

extern char *display_ascii_date(time_t);

extern int exec_program(char **);

extern char *display_current_time(void);

extern void clean_path_str(char *);

extern char *dirname(char *);

extern void runas_daemon(void);

extern char *loop_pgrep(char *);

extern int sub_str(char *, char *, char *);

struct uperm {
    mode_t mode;
    char flag;
    int indice;
};

#endif
