/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)stat.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _STAT
#define _STAT

struct	stat
{
	dev_t	st_dev;
	ino_t	st_ino;
	unsigned short st_mode;
	short	st_nlink;
	uid_t	st_uid;
	gid_t	st_gid;
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	int	st_spare1;
	time_t	st_mtime;
	int	st_spare2;
	time_t	st_ctime;
	int	st_spare3;
	long	st_blksize;
	long	st_blocks;
	long	st_serverID;
	long	st_version;
};

#define	S_IFMT	0170000		/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK 0140000/* socket */
#define		S_IFIFO	0010000	/* fifo */
/* Extra Sprite types */
#define		S_IFPDEV 0150000 /* pseudo-device */
#define		S_IFRLNK 0160000 /* remote link */
#define	S_ISUID	0004000		/* set user id on execution */
#define	S_ISGID	0002000		/* set group id on execution */
#define	S_ISVTX	0001000		/* save swapped text even after use */
#define	S_IREAD	0000400		/* read permission, owner */
#define	S_IWRITE 0000200	/* write permission, owner */
#define	S_IEXEC	0000100		/* execute/search permission, owner */
/*
 * User-defined file types.  A number of types are standardized, but others
 * may be defined by the user.
 *
 *	 S_TYPE_UNDEFINED	- no type set
 *	 S_TYPE_TMP      	- temporary file
 *	 S_TYPE_SWAP     	- swap file
 *	 S_TYPE_OBJECT   	- ".o" file
 *	 S_TYPE_BINARY   	- executable
 *	 S_TYPE_OTHER   	- file that doesn't correspond to any
 *				  specific type.  This is distinct
 *				  from undefined, which says the type
 *				  is uninitialized and may be inferred
 *				  by parent directory or file name.
 */
#define S_TYPE_UNDEFINED 0
#define S_TYPE_TMP 1
#define S_TYPE_SWAP 2
#define S_TYPE_OBJECT 3
#define S_TYPE_BINARY 4
#define S_TYPE_OTHER 5
#endif /* _STAT */
