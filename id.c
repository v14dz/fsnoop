/* id.c -- Contains functions to perform basic requests on the password and
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

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id.h"

/* Search in a the root linked list the name for the corresponding id.  If
 * id isn't find (orphan file), return the numeric ID as a string. 
 */
char *name_from_id(struct id_name *root, uid_t id) {

    char *str;

    while (root->next) {

        if (root->id == id)
            return root->name;

        root = root->next;
    }

    str = malloc(12);

    if (!str) {
        fprintf(stderr, "name_from_id(): malloc failed.\n");
        exit(1);
    }

    sprintf(str, "%d", id);

    return str;
}

/* Builds a linked list of id_name structures from the passwd database.
 * Returns a pointer to the first node. 
 */
struct id_name *load_passwd() {

    struct passwd *p;
    struct id_name *root, *u, *u_save;

    u = malloc(sizeof(struct id_name));

    root = u;
    u->next = NULL;

    while ((p = getpwent()) != NULL) {

        u->name = strdup(p->pw_name);
        u->id = p->pw_uid;

        u_save = u;

        u = malloc(sizeof(struct id_name));

        if (!u) {
            fprintf(stderr, "load_passwd(): malloc failed.\n");
            exit(1);
        }

        u_save->next = u;
        u->next = NULL;
    }

    endpwent();

    return root;
}

/* Builds a linked list of id_name structures from the group database.
 * Returns a pointer to the first node. 
 */
struct id_name *load_group() {

    struct group *g;

    struct id_name *root, *u, *u_save;

    u = malloc(sizeof(struct id_name));

    root = u;
    u->next = NULL;

    while ((g = getgrent()) != NULL) {

        u->name = strdup(g->gr_name);
        u->id = g->gr_gid;

        u_save = u;

        u = malloc(sizeof(struct id_name));

        if (!u) {
            fprintf(stderr, "load_group(): malloc failed.\n");
            exit(1);
        }

        u_save->next = u;
        u->next = NULL;
    }

    endgrent();

    return root;
}
