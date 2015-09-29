/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dirent.h	7.2 (Berkeley) 12/22/86
 */

#ifndef _DIRENT
#define _DIRENT

#include <sys/types.h>

#ifndef MAXNAMLEN
#define	MAXNAMLEN	255
#endif

struct dirent {
    off_t	d_off;		/* Offset to lseek for next entry. */
    u_long	d_fileno;	/* Unique file identifier. */
    u_short	d_reclen;	/* Length of the record. */
    u_short	d_namlen;	/* Length of the name */
    char	d_name[MAXNAMLEN+1];
};

#endif
