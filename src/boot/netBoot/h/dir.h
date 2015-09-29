/*	@(#)dir.h 1.1 86/09/27 SMI; from UCB 4.5 82/11/13	*/


/*
 * A directory consists of some number of blocks each of which is
 * less than or equal to the filesystem block size number of
 * bytes.
 *
 * Each block contains some number of directory entry structures,
 * which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its file number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary
 * with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN, plus
 * a null byte.
 *
 * The macro DIRSIZ(dp) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > DIRSIZ(dp).
 *
 * All the bytes in a directory block are claimed by the directory entries.
 * This usually results in the last entry in a directory having a large
 * dp->d_reclen.  Free entries have their dp->d_fileno set to 0.
 */
#define	MAXNAMLEN	255

struct	direct {
	u_long	d_fileno;		/* file number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name (up to MAXNAMLEN + 1) */
};

#ifndef KERNEL
#define d_ino	d_fileno		/* compatablity */

/*
 * The DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 */
#undef DIRSIZ
#define DIRSIZ(dp) \
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))

/*
 * Definitions for library routines operating on directories.
 */
typedef struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	long	dd_bbase;
	long	dd_entno;
	long	dd_bsize;
	char	*dd_buf;
} DIR;

#ifndef NULL
#define NULL 0
#endif
extern	DIR *opendir();
extern	struct direct *readdir();
extern	long telldir();
extern	void seekdir();
#define rewinddir(dirp)	seekdir((dirp), (long)0)
extern	void closedir();
#endif
