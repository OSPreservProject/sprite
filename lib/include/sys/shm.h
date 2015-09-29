/* 	@(#)shm.h	4.1.1.2	(ULTRIX)	12/9/88 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 * 3 Mar 88 -- jaa
 *	added smsmat to sminfo, which is now the configurable
 *	highest attachable address for shared mem
 *
 * 27 Jan 87 -- depp
 *	Cleaned up some of the incompatibilities between SYSTEM V and
 *	ULTRIX-32 naming conventions
 *
 * 29 Apr 86 -- depp
 *	Moved some shared memory constants from param.h to here, except 
 *	SMMIN, SMMAX, SMBRK which are configurable.  The defaults
 *	for these 3 constants is in /sys/conf/param.c.  Plus, the data
 *	element "small" removed from struct sminfo.
 *
 * 18 Mar 86 -- depp
 *	Changed struct smem.  The sm_daddr member is now a pointer rather
 *	than an array, so that an array can be dynamically allocated to
 *	handle any size request.
 *
 * 18 Sep 85 -- depp
 *	Added SMNOSW flag and sm_lcount for page locking
 *
 * 08 April 85 -- depp
 *	New file for System V shared memory
 *
 */

/*
**	IPC Shared Memory Facility.
*/

/*
**	Implementation Constants.
*/

			/* segment low boundary address multiple */
			/* (SMLBA must be a power of 2) */
#ifdef vax
#define	SMLBA	ctob(CLSIZE)
#endif vax
#ifdef mips
#define SMLBA   ctob(NPTEPG)
#endif mips


			/* for SYSTEM V compatibility		*/
#define	SHMLBA		SMLBA

/*
**	Permission Definitions.
*/

#define	SM_R	0400	/* read permission */
#define	SM_W	0200	/* write permission */

			/* for SYSTEM V compatibility		*/
#define SHM_R	SM_R
#define SHM_W	SM_W

/*
**	ipc_perm Mode Definitions.
*/

#define	SM_CLEAR	01000	/* clear segment on next attach */
#define	SM_DEST		02000	/* destroy seg when # attached = 0 */

			/* for SYSTEM V compatibility		*/
#define SHM_INIT	SM_CLEAR
#define SHM_DEST	SM_DEST

/*
**	Message Operation Flags.
*/

#define	SM_RDONLY	010000	/* attach read-only (else read-write) */
#define	SM_RND		020000	/* round attach address to SHMLBA */

			/* for SYSTEM V compatibility		*/
#define SHM_RDONLY	SM_RDONLY
#define SHM_RND		SM_RND

/*
 *  sizing constant (the balance are in /sys/h/param.h and /sys/conf/param.c.
 */
#define	SMMNI	100	   /* Max number SM segments in system */

/*
**	Structure Definitions.
*/


/*
**	There is a shared mem id struct for each segment in the system.
*/


struct smem {
				/* SM_PERM must be the first	*/
				/* element in the structure.	*/
	struct ipc_perm	sm_perm;	/* permission struct */
	swblk_t	*sm_daddr; 	/* disk addrs of DMTEXT page segs */
	swblk_t	sm_ptdaddr;	/* disk address of page table */
	size_t	sm_size;	/* segment size (bytes) */
	struct proc *sm_caddr;	/* ptr to linked proc, if loaded */
	struct pte *sm_ptaddr;	/* ptr to assoc page table */
	short	sm_rssize;	/* SM resource set size (pages)	*/
	ushort sm_lpid;		/* pid of last smop */
	ushort sm_cpid;		/* pid of creator */
	char	sm_count;	/* reference count */
	char	sm_ccount;	/* number of loaded references */
	short	sm_lcount;	/* number of processes locking SMS */
	short	sm_flag;	/* traced, written flags */
	short	sm_poip;	/* page out in progress count */
	time_t	sm_atime;	/* last smat time */
	time_t	sm_dtime;	/* last smdt time */
	time_t	sm_ctime;	/* last change time */
};
			/* for SYSTEM V compatibility		*/
#define shmid_ds	smem
#define key_t		long
#define shm_perm	sm_perm
#define shm_segsz	sm_size
#define shm_cpid	sm_cpid
#define shm_lpid	sm_lpid
#define shm_nattch	sm_count
#define shm_atime	sm_atime
#define shm_dtime	sm_dtime
#define shm_ctime	sm_ctime


/* NOTE:  These values must align with X* flag values in text.h */
#define	SMLOCK	010		/* Being swapped in or out */
#define	SMWANT	020		/* Wanted for swapping */
#define SMNOSW	0100		/* Lock segment in memory */

/* shared memory specific commands for shmctl syscall */
#define SHM_LOCK	3	/* Lock segment in memory */
#define SHM_UNLOCK	4	/* Unlock segment in memory */

/*
 * sminfo structure is used by the system for bounds checking.  All of 
 * the elements in this structure are initialized in /sys/conf/param.c.
 *
 * Three of the elements are configurable via the "config" program
 * (smmax, smmin, smbrk).  If they are not config'ed, then they all
 * have defaults.
 */  
struct sminfo {
	int	smmax,	/* max shared memory segment size */
		smmin,	/* min shared memory segment size */
		smmni,	/* # of shared memory identifiers */
		smseg,	/* max attached shared memory segs per proc */
		smbrk,	/* gap (in clicks) used between data and SM */
	        smsmat; /* max shmem attach addr (clicks) */
};

			/* for SYSTEM V compatibility */
#define shminfo		sminfo
#define shmmax		smmax
#define shmmin		smmin
#define shmmni		smmni
#define shmseg		smseg
#define shmbrk		smbrk

