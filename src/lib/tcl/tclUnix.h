/*
 * tclUnix.h --
 *
 *	This file reads in UNIX-related header files and sets up
 *	UNIX-related macros for Tcl's UNIX core.  It should be the
 *	only file that contains #ifdefs to handle different flavors
 *	of UNIX.  This file sets up the union of all UNIX-related
 *	things needed by any of the Tcl core files.  This file
 *	depends on configuration #defines in tclConfig.h
 *
 *	The material in this file was originally contributed by
 *	Karl Lehenbauer, Mark Diekhans and Peter da Silva.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/tcl/RCS/tclUnix.h,v 1.16 91/09/22 16:06:33 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _TCLUNIX
#define _TCLUNIX

/*
 * The following #defines are used to distinguish between different
 * UNIX systems.  These #defines are normally set by the "config" script
 * based on information it gets by looking in the include and library
 * areas.  The defaults below are for BSD-based systems like SunOS
 * or Ultrix.
 *
 * TCL_GETTOD -			1 means there exists a library procedure
 *				"gettimeofday" (e.g. BSD systems).  0 means
 *				have to use "times" instead.
 * TCL_GETWD -			1 means there exists a library procedure
 *				"getwd" (e.g. BSD systems).  0 means
 *				have to use "getcwd" instead.
 * TCL_SYS_ERRLIST -		1 means that the array sys_errlist is
 *				defined as part of the C library.
 * TCL_SYS_TIME_H -		1 means there exists an include file
 *				<sys/time.h>.
 * TCL_SYS_WAIT_H -		1 means there exists an include file
 *				<sys/wait.h> that defines constants related
 *				to the results of "wait".
 * TCL_UNION_WAIT -		1 means that the "wait" system call returns
 *				a structure of type "union wait" (e.g. BSD
 *				systems).  0 means "wait" returns an int
 *				(e.g. System V and POSIX).
 */

#define TCL_GETTOD 1
#define TCL_GETWD 1
#define TCL_SYS_ERRLIST 1
#define TCL_SYS_TIME_H 1
#define TCL_SYS_WAIT_H 1
#define TCL_UNION_WAIT 1

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#if TCL_SYS_TIME_H
#   include <sys/time.h>
#endif
#if TCL_SYS_WAIT_H
#   include <sys/wait.h>
#endif

#ifdef sprite
#include <sys/dir.h>
#define dirent direct
#endif

/*
 * Not all systems declare the errno variable in errno.h. so this
 * file does it explicitly.  The list of system error messages also
 * isn't generally declared in a header file anywhere.
 */

extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];

/*
 * The type of the status returned by wait varies from UNIX system
 * to UNIX system.  The macro below defines it:
 */

#if TCL_UNION_WAIT
#   define WAIT_STATUS_TYPE union wait
#else
#   define WAIT_STATUS_TYPE int
#endif

/*
 * Supply definitions for macros to query wait status, if not already
 * defined in header files above.
 */

#ifndef WIFEXITED
#   define WIFEXITED(stat)  (((*((int *) &(stat))) & 0xff) == 0)
#endif

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat) (((*((int *) &(stat))) >> 8) & 0xff)
#endif

#ifndef WIFSIGNALED
#   define WIFSIGNALED(stat) (((*((int *) &(stat)))) && ((*((int *) &(stat))) == ((*((int *) &(stat))) & 0x00ff)))
#endif

#ifndef WTERMSIG
#   define WTERMSIG(stat)    ((*((int *) &(stat))) & 0x7f)
#endif

#ifndef WIFSTOPPED
#   define WIFSTOPPED(stat)  (((*((int *) &(stat))) & 0xff) == 0177)
#endif

#ifndef WSTOPSIG
#   define WSTOPSIG(stat)    (((*((int *) &(stat))) >> 8) & 0xff)
#endif

/*
 * Supply macros for seek offsets, if they're not already provided by
 * an include file.
 */

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif

#ifndef SEEK_END
#   define SEEK_END 2
#endif

/*
 * The stuff below is needed by the "time" command.  If this
 * system has no gettimeofday call, then must use times and the
 * CLK_TCK #define (from sys/param.h) to compute elapsed time.
 * Unfortunately, some systems only have HZ and no CLK_TCK, and
 * some might not even have HZ.
 */

#if ! TCL_GETTOD
#   include <sys/times.h>
#   include <sys/param.h>
#   ifndef CLK_TCK
#       ifdef HZ
#           define CLK_TCK HZ
#       else
#           define CLK_TCK 60
#       endif
#   endif
#endif

/*
 * Define access mode constants if they aren't already defined.
 */

#ifndef F_OK
#    define F_OK 00
#endif
#ifndef X_OK
#    define X_OK 01
#endif
#ifndef W_OK
#    define W_OK 02
#endif
#ifndef R_OK
#    define R_OK 04
#endif

/*
 * Make sure that MAXPATHLEN is defined.
 */

#ifndef MAXPATHLEN
#   ifdef _POSIX_PATH_MAX
#       define MAXPATHLEN _POSIX_PATH_MAX
#   else
#       define MAXPATHLEN 2048
#   endif
#endif

/*
 * Variables provided by the C library:
 */

extern char **environ;

/*
 * Library procedures used by Tcl but not declared in a header file:
 */

extern int	access	_ANSI_ARGS_((char *path, int mode));
extern int	chdir	_ANSI_ARGS_((char *path));
extern int	close	_ANSI_ARGS_((int fd));
extern int	dup2	_ANSI_ARGS_((int src, int dst));
extern int	execvp	_ANSI_ARGS_((char *name, char **argv));
extern int	_exit 	_ANSI_ARGS_((int status));
extern int	fork	_ANSI_ARGS_((void));
extern int	geteuid	_ANSI_ARGS_((void));
extern int	getpid	_ANSI_ARGS_((void));
extern char *	getcwd  _ANSI_ARGS_((char *buffer, int size));
extern char *	getwd   _ANSI_ARGS_((char *buffer));
extern int	kill	_ANSI_ARGS_((int pid, int sig));
extern long	lseek	_ANSI_ARGS_((int fd, int offset, int whence));
extern char *	mktemp	_ANSI_ARGS_((char *template));
extern int	open	_ANSI_ARGS_((char *path, int flags, int mode));
extern int	pipe	_ANSI_ARGS_((int *fdPtr));
extern int	read	_ANSI_ARGS_((int fd, char *buf, int numBytes));
extern int	unlink	_ANSI_ARGS_((char *path));
extern int	write	_ANSI_ARGS_((int fd, char *buf, int numBytes));

#endif /* _TCLUNIX */
