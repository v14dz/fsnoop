/* util.c -- Contains utility functions.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <dirent.h>
#include <ctype.h>

#include "util.h"
#include "global.h"

static struct uperm perms[] = {

    { S_IRUSR, 'r', 1 },
    { S_IWUSR, 'w', 2 },
    { S_IXUSR, 'x', 3 },
    { S_IRGRP, 'r', 4 },
    { S_IWGRP, 'w', 5 },
    { S_IXGRP, 'x', 6 },
    { S_IROTH, 'r', 7 },
    { S_IWOTH, 'w', 8 },
    { S_IXOTH, 'x', 9 },
    { 0, '\0', 0 }
};

/* Basic hash table to retrieve a file type character from a mode thanks to
 * the following hash function "((mode & S_IFMT) >> 12)". */
static char filetype_c[13] = {

    '\0',
    'p',                        /* 1: FIFO */
    'c',                        /* 2: character device */
    '\0',
    'd',                        /* 4: directory */
    '\0',
    'b',                        /* 6: block device */
    '\0',
    '-',                        /* 8: regular file */
    '\0',
    'l',                        /* 10: symbolic link */
    '\0',
    's'                         /* 12: socket */
};

/* Open filename and return a file descriptor.  If file already exists it's
 * renamed with the '~' suffix.  Function return -1 on failure. */
int open_output_file(char *filename) {

    int fd, flags;
    mode_t mode;
    struct stat st;
    char *filename_save;

    flags = O_CREAT | O_EXCL | O_WRONLY | O_TRUNC;
    mode = S_IRUSR | S_IWUSR;

    /* test if the file already exists. */
    if (stat(filename, &st) == 0) {

        /* if yes, we rename it with the '~' suffix, removing the existing
         * backup file if any. */

        if ((filename_save = malloc(strlen(filename) + 2)) == NULL) {

            fprintf(stderr, "open_output_file(): malloc() failed.\n");
            return -1;
        }

        sprintf(filename_save, "%s~", filename);
        unlink(filename_save);

        if (link(filename, filename_save) != 0) {

            fprintf(stderr,
                    "open_output_file(): failed to create backup file.\n");
            return -1;
        }

        unlink(filename);
    }

    fd = open(filename, flags, mode);

    return fd;
}

/* Checks if file exists or not.  It returns 0 if the file exists and -1 if
 * it not exists. 
 */
int file_exists(char *file) {

    struct stat st;

    if (stat(file, &st) < 0)
        return -1;

    return 0;
}

/* Converts file into a relative path by adding "./" at the beginning.  If
 * file is an absolute path starting with "/", nothing changes and return
 * value is equal to file.
 */
char *get_path(char *file) {

    char *result;

    if ((strncmp(file, "./", 2) == 0) || (file[0] == '/')) {

        result = file;

    } else {

        if ((result = malloc(strlen(file) + 3)) == NULL) {
            fprintf(stderr, "Error: malloc() failed.\n");
            exit(1);
        }

        sprintf(result, "./%s", file);
    }

    return result;
}

/* It checks recursively parent directory existence and returns the first
 * one that exists.
 */
char *existing_parent_dir(char *file) {

    char *path, *ptr = NULL;
    int len;

    path = strdup(file);
    len = strlen(file);

    /* if file doesn't end with a '/', we want to start with the parent
     * directory. */
    if (path[len] != '/') {
        path = dirname(file);
    }

    while ((file_exists(path) < 0) && (ptr = strrchr(path, '/')))
        *ptr = 0;

    return (*path == 0) ? "/" : path;
}

/* Returns the pathname of the executable file exe.  It searches in the
 * PATH.  If it is unable to find it, it returns NULL. 
 */
char *search_binary(char *exe) {

    char *ptr, *path, *file;

    file = malloc(PATH_MAX);

    if (!file) {
        fprintf(stderr, "search_binary(): malloc failed.\n");
        exit(1);
    }

    path = getenv("PATH");

    while ((ptr = strchr(path, ':')) != NULL) {
        *ptr = '\0';

        sprintf(file, "%s/%s", path, exe);
        if (file_exists(file) == 0)
            return file;

        path = ptr + 1;
    }

    sprintf(file, "%s/%s", path, exe);
    if (file_exists(file) == 0)
        return file;

    return NULL;
}

/* Return the symbolic notation of the octal permission mode. 
 */
char *sym_perm(mode_t mode) {

    char *p;
    int i;

    /* "----------" + '\0' */
    if (!(p = malloc(11))) {
        printf("malloc() failed.\n");
        return NULL;
    }

    *(p + 10) = '\0';

    /* determining file type. */
    *p = filetype_c[(mode & S_IFMT) >> 12];

    /* determining file permissions. */
    for (i = 0; perms[i].flag; i++) {
        p[perms[i].indice] = (mode & perms[i].mode) ? perms[i].flag : '-';
    }

    /* determining special permissions. */
    if (mode & S_ISUID)
        p[3] = (p[3] == 'x' ? 's' : 'S');

    if (mode & S_ISGID)
        p[6] = (p[6] == 'x' ? 's' : 'S');

    if (mode & S_ISVTX)
        p[9] = (p[9] == 'x' ? 't' : 'T');

    return p;
}

/* Converts the calendar time t into a string of the form "Wed Jun 30
 * 21:49:08 1993". 
 */
char *display_ascii_date(time_t t) {

    char *date, *ptr;

    date = asctime(localtime((const time_t *) &t));

    /* remove the newline character */
    ptr = strchr(date, '\n');
    *ptr = '\0';

    return date;
}

/* Waits 1 second and starts the command cmd with the lowest scheduling
 * priority.  On success, it returns the PID of the child process,
 * otherwise it returns -1. 
 */
int exec_program(char **cmd) {

    int child_pid;

    if ((child_pid = fork()) == 0) {

        nice(19);
        umask(0);

        sleep(1);

        execve(cmd[0], cmd, NULL);
        _exit(0);
    }

    return child_pid;
}

/* Converts the current time into a string of the form "HH:MM:SS".
 */
char *display_current_time(void) {

    struct timeval now;
    struct tm *st_tm;
    char *result;

    gettimeofday(&now, NULL);

    st_tm = localtime(&now.tv_sec);

    result = malloc(16);

    if (!result) {
        fprintf(stderr, "display_current_time(): malloc failed.\n");
        exit(1);
    }

    sprintf(result, "%02d:%02d:%02d.%06ld", st_tm->tm_hour, st_tm->tm_min,
            st_tm->tm_sec, now.tv_usec);

    return result;
}

/* Removes consecutive and ending slashes in path. 
 */
void clean_path_str(char *path) {

    char *ptr, *ptr_r;
    int already = 0;

    ptr = path;
    ptr_r = path;

    while (*ptr) {

        if (*ptr == '/' && already) {
            ptr++;
            continue;
        }

        already = (*ptr == '/') ? 1 : 0;

        *ptr_r++ = *ptr++;
    }

    if (*(ptr_r - 1) == '/')
        *(ptr_r - 1) = '\0';
    else
        *ptr_r = '\0';
}

/* Returns the parent directory pathname.  This is my own version of
 * dirname.
 */
char *dirname(char *pathname) {

    char *ptr = NULL;
    char *dir_path = NULL;

    dir_path = malloc(PATH_MAX);

    if (!dir_path) {
        fprintf(stderr, "dirname(): malloc failed.\n");
        exit(1);
    }

    bzero(dir_path, PATH_MAX);
    strncpy(dir_path, pathname, PATH_MAX - 1);

    if ((ptr = strrchr(dir_path, '/')) == NULL) {
        free(dir_path);
        return strdup(".");
    }

    *ptr = 0;

    clean_path_str(dir_path);

    return dir_path;
}

/* Puts the current process in background.  This was heavily inspired from
 * the Linux daemon HOWTO.
 */
void runas_daemon(void) {

    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "In runas_daemon(), fork() failed.\n");
        exit(1);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(0);
    }

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        fprintf(stderr, "In runas_daemon(), setsid() failed.\n");
        exit(1);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        fprintf(stderr, "In runas_daemon(), chdir() failed.\n");
        exit(1);
    }

    close(STDIN_FILENO);
    close(STDERR_FILENO);
}

/* Replaces null chars by spaces (blank) on len bytes starting at address
 * src.
 */
static void replace_null_char(char *src, int len) {

    while (--len) {
        if (src[len] == 0)
            src[len] = ' ';
    }
}

/* Looks up process based on pattern and returns its PID as a string.
 * Inspired from the getrunners() function definition of procps.
 */
char *loop_pgrep(char *pattern) {

    struct dirent *ent;
    DIR *proc;

    char tbuf[PATH_MAX + ARG_MAX];
    int fd, len;

    if ((proc = opendir("/proc")) == NULL) {
        printf("Error: opendir failed.\n");
        exit(1);
    }

    for (;;) {

        rewinddir(proc);

        while ((ent = readdir(proc))) {

            /* ignore non-digit directories. */
            if (!isdigit(ent->d_name[0]))
                continue;

            sprintf(tbuf, "/proc/%s/cmdline", ent->d_name);

            if ((fd = open(tbuf, O_RDONLY, 0)) < 0)
                continue;

            memset(tbuf, '\0', sizeof tbuf);
            len = read(fd, tbuf, sizeof tbuf - 1);

            close(fd);

            if (!len)
                continue;

            /* cmdline contains '\0' instead of spaces.  We replace so bytes
             * before comparing. */
            replace_null_char(tbuf, len - 1);

            if (strncmp(tbuf, pattern, strlen(pattern)) == 0)
                return ent->d_name;
        }
    }
}

/* Finds pattern in the string src and substitute with the string s.
 * Returns -1 on failure.
 */
int sub_str(char *pattern, char *s, char *src) {

    char *ptr, *p;
    char *end = NULL;

    if (strlen(s) > strlen(pattern)) {
        fprintf(stderr, "sub_dir() failed (len(s) > len(pattern)).\n");
        return -1;
    }

    ptr = strstr(src, pattern);

    if (!ptr)
        return -1;

    /* we backup the end of the string if any (i.e. after the pattern). */
    if (*(ptr + strlen(pattern)))
        end = strdup(ptr + strlen(pattern));

    p = s;

    while (*p)
        *ptr++ = *p++;

    if (end)
        while(*end)
            *ptr++ = *end++;

    *ptr = 0;

    return 0;
}
