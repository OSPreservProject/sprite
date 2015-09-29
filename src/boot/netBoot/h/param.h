
/*	@(#)param.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Machine dependent constants for Sun-3.
 */
#define	NBPG	8192		/* bytes/page */
#define	PGOFSET	(NBPG-1)	/* byte offset into page */
#define	PGSHIFT	13		/* LOG2(NBPG) */

#define	NBSG	131072		/* bytes/segment */
#define	SGOFSET	(NBSG-1)	/* byte offset into segment */
#define	SGSHIFT	17		/* LOG2(NBSG) */

#define	CLSIZE		1
#define	CLSIZELOG2	0

#define	SSIZE		1	/* initial stack size/NBPG */
#define	SINCR		1	/* increment of stack/NBPG */

/*
 * Define UADDR as a 32 bit address so that the compiler will
 * generate short absolute references to access the u-area.
 */
#define	UADDR		(0-4*NBPG) /* u-area virtual address - 4th page down */
#define	UPAGES		1	/* pages of u-area, NOT including red zone */

#define	KERNSTACK	0x800	/* size of kernel stack in u-area */

/*
 * KERNELBASE is the virtual address which
 * the kernel mapping starts in all contexts.
 */
#define	KERNELBASE	0x0F000000

/*
 * Some macros for units conversion
 */
/* Core clicks (8192 bytes) to segments and vice versa */
#define	ctos(x)	(((x)+15)>>(SGSHIFT-PGSHIFT))
#define	stoc(x)	((x)<<(SGSHIFT-PGSHIFT))

/* Page number to segment number */
#define	ptos(x)	((x)>>(SGSHIFT-PGSHIFT))

/* Core clicks (8192 bytes) to disk blocks and vice versa */
#define	ctod(x)	((x)<<4)
#define	dtoc(x)	(((x)+15)>>4)
#define	dtob(x)	((x)<<9)

/* clicks to bytes */
#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+(NBPG-1))>>PGSHIFT))

/*
 * Macros to decode processor status word.
 */
#define	USERMODE(ps)	(((ps) & SR_SMODE) == 0)
#define	BASEPRI(ps)	(((ps) & SR_INTPRI) == 0)

