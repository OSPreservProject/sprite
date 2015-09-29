/*	@(#)sysinfo.h	4.4	(ULTRIX)	11/10/88	*/

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *
 *   Modification history:
 *
 *  9 Mar 88 -- chet
 *	Created this file.
 *
 */

/*
 *	This file contains constants used with the getsysinfo() and
 *	setsysinfo() system calls.
 *
 *	Both of these calls are operation driven; particular
 *	flavors of operation may use arguments, identifiers, flags, etc.
 *	to define the actual result.
 *
 */

/*
 *	getsysinfo() operation types
 */

#define GSI_PROG_ENV	1	/* Return process compatibility mode of */
                                /* the process as defined in <sys/exec.h> */

#define GSI_MAX_UPROCS	2	/* Return the maximum number of processes */
                                /* allowed per user id */

#define	GSI_TTYP	3	/* Return the device number of the */
                                /* controlling terminal */

#define GSI_NETBLK	4	/* Return the entire netblk structure */
				/* which is used for network install */

#define GSI_BOOTDEV	5	/* Return the bootdev string */
				/* which is used for install */

#ifdef mips
/*
 * these return value of the flag
 * that turns on/off printing the
 * fixed up unaligned access message
 */
#define GSI_UACSYS      6       /* get system wide flag */

#define GSI_UACPARNT    7	/* get parents */

#define GSI_UACPROC     8	/* get current proc */
#endif /* mips */

/*
 *	setsysinfo() operation types
 */

#define	SSI_NVPAIRS	1	/* Use a list of name value pairs to modify */
                                /* pre-defined system variables */

#define	SSI_ZERO_STRUCT	2	/* Zero a pre-defined system structure */

#define	SSI_SET_STRUCT	3	/* Set a pre-defined system structure to */
                                /* supplied values */

/*
 *	setsysinfo() SSI_NVPAIRS variable names
 */

#define	SSIN_NFSPORTMON 1	/* A boolean which determines whether */
                                /* incoming NFS traffic is originating */
                                /* at a privileged port or not */

#define	SSIN_NFSSETLOCK	2	/* A boolean which determines whether NFS */
                                /* (daemon) style file and record locking */
                                /* is enabled or not */

#define SSIN_PROG_ENV	3	/* set prog environment, BSD, SYSV, POSIX */

#ifdef mips
/* see GSI_UACxxx */
#define SSIN_UACSYS	4	/* set system printing on/off */
#define SSIN_UACPARNT	5	/* set parent proc on/off */
#define SSIN_UACPROC	6	/* set current proc on/off */
#endif /* mips */

/*
 *	setsysinfo() SSI_ZERO_STRUCT and SSI_SET_STRUCT structure types
 */


