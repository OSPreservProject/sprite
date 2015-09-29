/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)readdir.c	5.2 (Berkeley) 3/9/86";
#endif

#include <stddef.h>
#include <assert.h>

#include <sys/param.h>
#include <sys/dir.h>

static long swapLong();
static short swapShort();

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
    register DIR *dirp;
{
    register struct direct *dp;

    for (;;) {
	if (dirp->dd_loc == 0) {
	    dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, 
	    DIRBLKSIZ);
	    if (dirp->dd_size <= 0) {
		return NULL;
	    }
	}
	if (dirp->dd_loc >= dirp->dd_size) {
	    dirp->dd_loc = 0;
	    continue;
	}
	dp = (struct direct *)(dirp->dd_buf + dirp->dd_loc);

	/*
	 * Filesystems on big-endian and little-endian systems
	 * store the information in the directory in different
	 * byte order.  It is kind of tricky to detect which
	 * byte order this directory uses.  The record length
	 * (d_reclen) must be at least 8 (the offset of d_name
	 * in the direct structure) and can not be more than
	 * 512 (DIRBLKSIZ).  None of the numbers in the set [8-512]
	 * map back onto the same set under byte-swapping, so this
	 * is a reliable way of checking.
	 */
	if ((unsigned) dp->d_reclen > DIRBLKSIZ ||
	    (unsigned) dp->d_reclen < offsetof(struct direct, d_name[0])) {

	    /*
	     * Try byte swapping this entry.
	     */
	    dp->d_ino = swapLong(dp->d_ino);
	    dp->d_reclen = swapShort(dp->d_reclen);
	    dp->d_namlen = swapShort(dp->d_namlen);
	}
	if (dp->d_reclen == 0 || dp->d_reclen > DIRBLKSIZ + 1 - dirp->dd_loc) {
	    return NULL;
	}
	dirp->dd_loc += dp->d_reclen;
	if (dp->d_ino != 0) {
	    assert(dp->d_namlen <= MAXNAMLEN);
	    return dp;
	}
    }
}

static long
swapLong(x)
    long x;
{	    
    union {
	long l;
	char c[4];
    } in, out;

    in.l = x;
    out.c[0] = in.c[3];
    out.c[1] = in.c[2];
    out.c[2] = in.c[1];
    out.c[3] = in.c[0];
    return out.l;
}

static short
swapShort(x)
    short x;
{
    union {
	short s;
	char c[2];
    } in, out;

    in.s = x;
    out.c[0] = in.c[1];
    out.c[1] = in.c[0];
    return out.s;
}

