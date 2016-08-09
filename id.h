/* id.h -- Contains functions to perform basic requests on the password and
 * group databases.  The aim was avoid using getpwent() and getgrent()
 * because they open passwd/group files, and may produce an infinite loop
 * while monitoring /etc with "-e" option.  Now {user,group}names and IDs
 * are loaded and treated in memory, good for optimization.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
*/

#ifndef _FSNOOP_ID_H
#define _FSNOOP_ID_H

struct id_name {

  char *name;
  uid_t id;
  struct id_name *next;
};

extern char *name_from_id(struct id_name *, uid_t);

extern struct id_name *load_passwd(void);

extern struct id_name *load_group(void);

#endif
