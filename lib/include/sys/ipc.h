
/* 	@(#)ipc.h	4.1.1.1	(ULTRIX)	8/12/88 	*/

#include <sys/types.h>

/*
 *
 *   Modification history:
 *
 * 01 Mar 85 -- depp
 *	New file derived from System V IPC
 *
 */

/* Common IPC Access Structure */
struct ipc_perm {
	ushort	uid;	/* owner's user id */
	ushort	gid;	/* owner's group id */
	ushort	cuid;	/* creator's user id */
	ushort	cgid;	/* creator's group id */
	ushort	mode;	/* access modes */
	ushort	seq;	/* slot usage sequence number */
	long	key;	/* key */
};

/* Common IPC Definitions. */
/* Mode bits. */
#define	IPC_ALLOC	0100000		/* entry currently allocated */
#define	IPC_CREAT	0001000		/* create entry if key doesn't exist */
#define	IPC_EXCL	0002000		/* fail if key exists */
#define	IPC_NOWAIT	0004000		/* error if request must wait */
#define IPC_SYSTEM      0040000         /* share memory between kern/user */

/* Keys. */
#define	IPC_PRIVATE	(long)0	/* private key */

/* Control Commands. */
#define	IPC_RMID	0	/* remove identifier */
#define	IPC_SET		1	/* set options */
#define	IPC_STAT	2	/* get options */
