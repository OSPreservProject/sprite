/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)fcntl.h	5.2 (Berkeley) 1/8/86
 * $Header: /sprite/src/lib/include/RCS/fcntl.h,v 1.6 91/12/05 10:40:45 ouster Exp $
 */

#ifndef _FCNTL
#define _FCNTL

#include <cfuncproto.h>

/*
 * Flag values accessible to open(2) and fcntl(2)-- copied from
 * <sys/file.h>.  (The first three can only be set by open.)
 */
#define	O_RDONLY	000		/* open for reading */
#define	O_WRONLY	001		/* open for writing */
#define	O_RDWR		002		/* open for read & write */
#define	O_NDELAY	FNDELAY		/* non-blocking open */
					/* really non-blocking I/O for fcntl */
#define	O_APPEND	FAPPEND		/* append on each write */
#define	O_CREAT		FCREAT		/* open with file create */
#define	O_TRUNC		FTRUNC		/* open with truncation */
#define	O_EXCL		FEXCL		/* error on create if file exists */

#ifndef	F_DUPFD
/* fcntl(2) requests */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags */
#define	F_SETFD		2	/* Set fildes flags */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define	F_GETOWN	5	/* Get owner */
#define	F_SETOWN	6	/* Set owner */
#define	F_GETLK		7	/* Get record-lock info */
#define	F_SETLK		8	/* Set or clear a record-lock */
#define	F_SETLKW	9	/* Set or clear a record-lock (blocking) */
#define	F_RGETLK	10	/* Test if lock blocked */
#define	F_RSETLK	11	/* Set or unlock lock */
#define	F_CNVT		12	/* Convert fhandle to fd */
#define	F_RSETLKW	13	/* Set or clear remote lock (blocking) */

/* flags for F_GETFL, F_SETFL-- copied from <sys/file.h> */
#define	FNDELAY		00004		/* non-blocking reads */
#define	FAPPEND		00010		/* append on each write */
#define	FASYNC		00100		/* signal pgrp when data ready */
#define	FCREAT		01000		/* create if nonexistant */
#define	FTRUNC		02000		/* truncate to zero length */
#define	FEXCL		04000		/* error if already created */
#endif

extern int open _ARGS_((_CONST char *name, int flags, ...));

#endif /* _FCNTL */
