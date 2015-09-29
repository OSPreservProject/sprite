/*
 * param.h --
 *
 *	This file is modelled after the UNIX include file <sys/param.h>,
 *	but it only contains information that is actually used by user
 *	processes running under Sprite (the UNIX file contains mostly
 *	stuff for the kernel's use).  This file started off empty, and
 *	will gradually accumulate definitions as the needs of user
 *	processes become clearer.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/sys/RCS/param.h,v 1.13 89/07/14 09:15:24 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _PARAM
#define _PARAM

#include <sys/types.h>
#include <signal.h>
#include <machparam.h>

/*
 * MAXPATHLEN defines the longest permissable path length
 * after expanding symbolic links.
 */
#define MAXPATHLEN      1024

/*
 * MAXBSIZE defines the largest block size stored in the file system.
 */
#define MAXBSIZE	4096

/*
 * The macro below converts from the "st_blocks" field of a "struct stat"
 * to the number of bytes allocated to a file.  The "st_blocks" field
 * is currently measured in (archaic) 512-byte units.
 */
#define dbtob(blocks) ((unsigned) (blocks) << 9)

/*
 * MAXHOSTNAMELEN defines the maximum length of a host name in the
 * network.
 */
#define MAXHOSTNAMELEN 64

/*
 * Maximum number of characters in the argument list for exec.  Is this
 * really a limit in Sprite, or is it here just for backward compatibility?
 */
#define NCARGS 10240

/*
 * Maximum number of open files per process.  This is not really a limit
 * for Sprite;  it's merely there for old UNIX programs that expect it.
 */
#define NOFILE 64

/*
 * The maximum number of groups that a process can be in at once
 * (the most stuff ever returned by getgroups).  Note:  this needs
 * to be coordinated with FS_NUM_GROUPS, which it isn't right now.
 */
#define NGROUPS 16

/*
 * Macros for fast min/max.
 */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/*
 * Round up an object of size x to one that's an integral multiple
 * of size y.
 */

#define roundup(x, y)	((((x) + ((y) -1))/(y))*(y))

/*
 * Miscellanous.
 */
#ifndef NULL
#define NULL	0
#endif /* NULL */

#endif /* _PARAM */
