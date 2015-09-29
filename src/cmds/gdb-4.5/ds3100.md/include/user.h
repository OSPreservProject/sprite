/* 	@(#)user.h	4.1.1.5	(ULTRIX)	7/7/88 	*/

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *	Modification History
 *
 *	Fred Glover - Jan 12, 1988
 *		Add UF_FDLOCK flag for Sys-V file lock
 *
 *	Tim Burke -- Dec 28, 1987
 *	Removed u_ttyp field.  It is now in the proc struct.
 *
 * 	15 Sep 87 -- Mark Parenti
 *	Added oldsig to record which signals were invoked using the
 *	POSIX signal() interface which requires handler to be reset
 *	to SIG_DFL ala System V. Added u.narg field to contain
 *	number of arguments to a syscall.
 *
 *
 * 	15 dec 86 -- depp
 *	Changed u_smodsize -> u_osmsize.  The former was not being
 *	used and the later is required for process SM page table 
 *	swap allocation.  The size or related offsets in the user
 *	table is unchanged.
 *	
 *	Depp -- April 2, 1986
 *		Removed a.out header data from struct user.
 *
 *	Stephen Reilly - Oct 14, 1985
 *		Reomved fields that are no longer needed
 *
 *      Depp -- September 30, 1985
 *		Added u_lock for locking memory segments
 *
 *      Stephen Reilly - Sept 9, 1985
 *		Modified to handle the new 4.3BSD namei code.
 *
 *      Depp -- April 4, 1985
 *		Removed SHMEM ifdefs
 *
 *	001 - Larry Cohen - April 4, 1985
 *		add UF_INUSE flag to mark file in use.  supports open
 *		block if in use capability
 *
 *
 * 01 Mar 85 -- depp
 *	Added System V Shared memory data elements to user structure.
 *
 */

#ifdef KERNEL
#include "../machine/pcb.h"
#include "../h/dmap.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/namei.h"
#ifdef mips
#include "../machine/debug.h"
#endif mips
#else KERNEL
#include <machine/pcb.h>
#include <machine/sys/dmap.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <machine/sys/namei.h>
#ifdef mips
#include <machine/sys/ptrace.h>
#include <machine/debug.h>
#endif mips
#endif KERNEL

/*
 * Per process structure containing data that
 * isn't needed in core when the process is swapped out.
 */
 
#define	SHSIZE		32
#define	MAXCOMLEN	16		/* <= MAXNAMLEN, >= sizeof(a_comm) */

struct ucred {
	u_short	cr_ref;			/* reference count */
	short   cr_uid;			/* effective user id */
	short   cr_gid;			/* effective group id */
	int     cr_groups[NGROUPS];	/* groups, 0 terminated */
	short   cr_ruid;		/* real user id */
	short   cr_rgid;		/* real group id */
};
 
struct	user {
	struct	pcb u_pcb;
	struct	proc *u_procp;		/* pointer to proc structure */
	int	*u_ar0;			/* address of users saved R0 */
	char	u_comm[MAXNAMLEN + 1];

/* syscall parameters, results and catches */
	int	u_arg[8];		/* arguments to current system call */
	int	*u_ap;			/* pointer to arglist */
	label_t	u_qsave;		/* for non-local gotos on interrupts */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		off_t	r_off;
		time_t	r_time;
	} u_r;
	char	u_error;		/* return error code */
	char	u_eosys;		/* special action on end of syscall */
	char	u_narg;			/* number of arguments in u_arg above */

/* 1.1 - processes and protection */
	struct ucred *u_cred;		/* user credentials (uid, gid, etc) */
#define	u_uid		u_cred->cr_uid
#define	u_ruid		u_cred->cr_ruid
#define	u_gid		u_cred->cr_gid
#define	u_rgid		u_cred->cr_rgid
#define	u_groups 	u_cred->cr_groups

/* 1.2 - memory management */
	size_t	u_tsize;		/* text size (clicks) */
	size_t	u_dsize;		/* data size (clicks) */
	size_t	u_ssize;		/* stack size (clicks) */
	struct	dmap u_dmap;		/* disk map for data segment */
	struct	dmap u_smap;		/* disk map for stack segment */
	struct	dmap u_cdmap, u_csmap;	/* shadows of u_dmap, u_smap, for
					   use of parent during fork */
	label_t u_ssave;		/* label variable for swapping */
	size_t	u_odsize, u_ossize;	/* for (clumsy) expansion swaps */
	time_t	u_outime;		/* user time at last sample */

/* 1.3 - signal management */
	void	(*u_signal[NSIG])();	/* disposition of signals */
	int	u_sigmask[NSIG];	/* signals to be blocked */
	int	u_sigonstack;		/* signals to take on sigstack */
	int	u_sigintr;		/* signals that interrupt syscalls */
	int	u_oldsig;		/* POSIX "old style" signals */
	int	u_oldmask;		/* saved mask from before sigpause */
	int	u_code;			/* ``code'' to trap */
	struct	sigstack u_sigstack;	/* sp & on stack state variable */
#ifdef mips
	int	(*u_sigtramp)();	/* signal trampoline code */
	int	u_trapcause;		/* cause for SIGTRAP */
	int	u_trapinfo;		/* extra info concerning SIGTRAP */
#endif mips
#define	u_onstack	u_sigstack.ss_onstack
#define	u_sigsp		u_sigstack.ss_sp

/* 1.4 - descriptor management */
	struct	file *u_ofile[NOFILE];	/* file structures for open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
#define	UF_INUSE 	0x4		/* mutex semaphore */
#define UF_FDLOCK	0x8		/* sys-V file locked */
	int	u_lastfile;		/* highest numbered open file */
#define u_omax	u_lastfile
	struct	gnode *u_cdir;		/* current directory */
	struct	gnode *u_rdir;		/* root directory of current process */
#define u_ttyp	u_procp->p_ttyp
	dev_t	u_ttyd;			/* controlling tty dev */
	short	u_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	struct	rusage u_ru;		/* stats for this proc */
	struct	rusage u_cru;		/* sum of stats for reaped children */
	struct	itimerval u_timer[3];
	int	u_tracedev;		/* for syscall tracer */
	int	u_XXX[3];
	struct	timeval	u_start;	/* whole timeval instead of secs */
	short	u_acflag;

/* 1.6 - resource controls */
	struct	rlimit u_rlimit[RLIM_NLIMITS];
	struct	quota *u_quota;		/* user's quota structure */
	int	u_qflags;		/* per process quota flags */

	struct uprof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		unsigned pr_size;	/* buffer size */
		unsigned pr_off;	/* pc offset */
		unsigned pr_scale;	/* pc scaling */
	} u_prof;

/* 1.7 - System V related elements */
	size_t	u_smsize;	/* size of SM space (clicks)	*/
	size_t	u_osmsize;	/* old SM size for expansion swaps */
	int	u_lock;		/* memory locking flags (see ../h/lock.h) */

/* 1.9 - Namei caching */
	struct nameicache {		/* last successful directory search */
		int nc_prevoffset;	/* offset at which last entry found */
		ino_t nc_inumber;	/* inum of cached directory */
		dev_t nc_dev;		/* dev of cached directory */
		time_t nc_time;		/* time stamp for cache entry */
	} u_ncache;
	struct nameidata u_nd;
/* 1.10 - User stack */
#ifdef mips
/* kernel stack */
#endif mips
	int	u_stack[1];
};

#ifdef KERNEL
struct ucred *crget();
struct ucred *crcopy();
struct ucred *crdup();
#endif KERNEL

/* u_eosys values */
#ifdef vax
#define	JUSTRETURN	0
#define	RESTARTSYS	1
#define	SIMULATERTI	2
#define	REALLYRETURN	3
#endif vax
#ifdef mips
#define	FULLRESTORE	1
#define	RESTARTSYS	2
#define NORMALRETURN	3
#endif mips

/* u_error codes */
#ifdef KERNEL
#include "../h/errno.h"
#else KERNEL
#include <errno.h>
#endif KERNEL

#ifdef KERNEL
#ifdef vax
extern	struct user u;
extern	struct user swaputl;
extern	struct user forkutl;
extern	struct user xswaputl;
extern	struct user xswap2utl;
extern	struct user pushutl;
extern	struct user vfutl;
#endif vax

#ifdef mips
/*
 * This "declaration" tells front end it can use r0 relative addressing.
 */
#define	u	(*(struct user *)UADDR)
/*
 * Since u is strange, use up for debugging purposes.
 */
struct user *up;

extern char VA_swaputl[];
#define	swaputl	(*(struct user *)VA_swaputl)

extern char VA_forkutl[];
#define	forkutl	(*(struct user *)VA_forkutl)

extern char VA_vfutl[];
#define	vfutl	(*(struct user *)VA_vfutl)
#endif mips

#endif KERNEL
