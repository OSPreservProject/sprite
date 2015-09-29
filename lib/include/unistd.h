/*
 * unistd.h --
 *
 *      Macros, constants and prototypes for Posix conformance.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/unistd.h,v 1.12 92/04/21 14:00:06 kupfer Exp $
 */

#ifndef _UNISTD
#define _UNISTD

#include <cfuncproto.h>
#include <sys/types.h>

#ifdef __STDC__
#define VOLATILE volatile
#else
#define VOLATILE
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef int size_t;
#endif

/* 
 * Strict POSIX stuff goes here.  Extensions go down below, in the 
 * ifndef _POSIX_SOURCE section.
 */

extern void VOLATILE _exit _ARGS_((int status));
extern int access _ARGS_((const char *path, int mode));
extern int chdir _ARGS_((const char *path));
extern int chown _ARGS_((const char *path, uid_t owner, gid_t group));
extern int close _ARGS_((int fd));
extern int dup _ARGS_((int oldfd));
extern int dup2 _ARGS_((int oldfd, int newfd));
extern int execl _ARGS_((const char *path, ...));
extern int execle _ARGS_((const char *path, ...));
extern int execlp _ARGS_((const char *file, ...));
extern int execv _ARGS_((const char *path, char **argv));
extern int execve _ARGS_((const char *path, char **argv, char **envp));
extern int execvp _ARGS_((const char *file, char **argv));
extern int fork _ARGS_((void));
extern char *getcwd _ARGS_((char *buf, int size));
extern gid_t getegid _ARGS_((void));
extern uid_t geteuid _ARGS_((void));
extern gid_t getgid _ARGS_((void));
extern int getgroups _ARGS_((int bufSize, int *buffer));
extern int getpid _ARGS_((void));
extern uid_t getuid _ARGS_((void));
extern int isatty _ARGS_((int fd));
extern long lseek _ARGS_((int fd, long offset, int whence));
extern int pipe _ARGS_((int *fildes));
extern int read _ARGS_((int fd, char *buf, size_t size));
extern int setgid _ARGS_((gid_t group));
extern int setuid _ARGS_((uid_t user));
extern unsigned sleep _ARGS_ ((unsigned seconds));
extern char *ttyname _ARGS_((int fd));
extern int unlink _ARGS_((const char *path));
extern int write _ARGS_((int fd, const char *buf, size_t size));

#ifndef	_POSIX_SOURCE
extern char *crypt _ARGS_((const char *, const char *));
extern int fchown _ARGS_((int fd, uid_t owner, gid_t group));
extern int flock _ARGS_((int fd, int operation));
extern int ftruncate _ARGS_((int fd, unsigned long length));
extern int readlink _ARGS_((const char *path, char *buf, int bufsize));
extern int setegid _ARGS_((gid_t group));
extern int seteuid _ARGS_((uid_t user));
extern int setreuid _ARGS_((int ruid, int euid));
extern int symlink _ARGS_((const char *, const char *));
extern int ttyslot _ARGS_((void));
extern int truncate _ARGS_((const char *path, unsigned long length));
extern int umask _ARGS_((int cmask));
extern _VoidPtr	valloc _ARGS_((size_t bytes));
extern int vfork _ARGS_((void));
#endif /* _POSIX_SOURCE */

#endif /* _UNISTD */

