/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	7.3 (Berkeley) 6/20/87
 *  $Header: /r3/kupfer/spriteserver/include/user/sys/RCS/types.h,v 1.2 91/11/11 23:13:51 kupfer Exp $
 */

#ifndef _TYPES
#define	_TYPES
/*
 * Basic system types and major/minor device constructing/busting macros.
 *
 * Note that these are also defined as unix_major and unix_minor to
 * avoid conflict with the major and minor fields in the Fs_FileID struct.
 * That structure is defined in <fs.h>, which undefines the major() and
 * minor() macros if you aren't using an ANSI C compiler like gcc.
 */
#ifndef KERNEL
/* major part of a device */
#define	major(x)	((int)(((unsigned)(x)>>8)&0377))
#endif
#define unix_major(x)	((int)(((unsigned)(x)>>8)&0377))

#ifndef KERNEL
/* minor part of a device */
#define	minor(x)	((int)((x)&0377))
#endif
#define	unix_minor(x)	((int)((x)&0377))

/* make a device number */
#define	makedev(x,y)	((dev_t)(((x)<<8) | (y)))

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* sys III compat */

#if defined(vax) || defined(tahoe)
typedef	struct	_physadr { int r[1]; } *physadr;
typedef	struct	label_t	{
	int	val[14];
} label_t;
#endif
#if defined(mc68000)
typedef	struct	_physadr { short r[1]; } *physadr;
typedef	struct	label_t	{
	int	val[13];
} label_t;
#endif

#if defined(mips)
typedef struct  _physadr { int r[1]; } *physadr;
typedef struct  label_t {
        int     val[12];
} label_t;
#endif mips

/* 
 * See <quad.h> for operations on these data types.
 */
typedef struct _quad {long val[2]; } quad;
typedef	struct	_uquad {unsigned long val[2]; } u_quad;

typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	long *	qaddr_t;	/* should be typedef quad * qaddr_t; */
typedef	u_long	ino_t;
typedef	long	swblk_t;
#ifndef _SIZE_T
#define _SIZE_T
typedef	int	size_t;
#endif
#ifndef _TIME_T
#define _TIME_T
typedef	long	time_t;
#endif
#ifndef _CLOCK_T
#define _CLOCK_T
typedef	long	clock_t;
#endif
typedef	short	dev_t;
typedef	long	off_t;
typedef	short	uid_t;
typedef	short	gid_t;
/* 
 * A Sprite Proc_PID is an unsigned int, but UNIX code expects to be able 
 * to check for pid's that are less than 0 (e.g., error return from 
 * fork()), so we make pid_t be signed.
 */
typedef int	pid_t;
typedef long	key_t;		/* sys V compat */

#define	NBBY	8		/* number of bits in a byte */
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))

#endif /* _TYPES */
