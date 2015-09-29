/*
 * atcreg.h - definitions internal to the driver
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun4.md/atcreg.h,v 9.1 92/08/24 15:48:15 alc Exp $
 */

#ifndef _ATCREG
#define _ATCREG

#include "iopb.h"

#define	MAX_SG	41	/* allows for 164k transfers */

#define	NBPP	PAGESIZE

#define	MAX_BCOUNT	(MAX_SG * NBPP)

#ifndef KERNEL
#define KERNEL
#endif KERNEL
#if defined(KERNEL) || defined(INKERNEL)
/*
 * IOPB header used internally by the driver
 *
 * Each IOPB comes with a buffer header. This is only used by special
 * commands as a convenience to avoid allocating buffers from the global
 * pool.
 */
struct iopbhdr {
/*	struct	buf	spbuf;	*/	/* for special commands */
	struct	iopb	iopb;		/* must be first */
	struct	sg	sg_list[MAX_SG];/* scatter/gather list */
	struct	buf	*bp;		/* buffer this request is for */
	int             ticks;          /* used for timeouts */
	struct	iopbhdr	*next_free;	/* links free IOPBs */
	u_long	index;			/* index of this iopb */
	double  pad;
	struct pass_thru_block pass_thru;         /* contains info for pass-thru cmd */
};

/*
 * Define the FIFO mapping in VME A16 space.
 */
 volatile struct FIFO	{
	u_short	read_fifo;				/* 0x0   */
	u_short	fill1;					/* 0x2   */
	u_short	write_fifo;				/* 0x4   */
} FIFO;


typedef struct FIFO ATCRegs;     /*note:  not in latest version from ATC */
#endif


/*
 * Structure of driver statistics
 */
struct dstats {
	int	npq;		/* number of entries in the process queue */
	int	ndq;		/* number of entries in the delayed queue */
	int	max_pq;		/* maximum size of the process queue so far */
	int	n_started;	/* # of IOPBs started so far */
	int	n_completed;	/* # of IOPBs completed */
	int     n_ints;         /* # of interrupts taken */
};

/*
 * ioctl functions
 *
 * Ioctl's come in two kinds. Local ioctl's affect the operation of the
 * driver. Command ioctl's translate directly into an IOPB that is created
 * and sent to the array, usually to transfer control information to or
 * from it. The application fills in a command buffer and passes its
 * address via the ioctl system call. Only certain combinations of command
 * and option bytes are supported by the driver.
 */
struct cmdbuf {
	char	command;
	char	option;
	char	flags;
	char	*buffer;
	u_short	id;
	u_long	block;

	u_long	status;		/* status returned here on error */
};

#define	DBUFSZ	512	/* size of the per-controller diagnostics buffer */


#define	ATC_LOCAL_SET	0x10000		/* ioctl's the affect the driver only */
#define	ATC_CMD_SET	0x20000		/* commands passed through the driver */
#define	ATC_DIAG_SET	0x40000		/* ioctl's for diagnostics */

#define	ATC_DEBUG	ATC_LOCAL_SET|0	/* enable/disable debug code */
#define	ATC_SINGLE	ATC_LOCAL_SET|1	/* enable/disable multiple requests */
#define	ATC_TIMEOUTS	ATC_LOCAL_SET|2	/* enable/disable timeouts */
#define	ATC_STIMEOUTS	ATC_LOCAL_SET|3	/* show timeouts */
#define	ATC_SSTATS	ATC_LOCAL_SET|4	/* show stats */
#define	ATC_CSTATS	ATC_LOCAL_SET|5	/* clear stats */
#define	ATC_RESET	ATC_LOCAL_SET|6	/* re-init the table descriptor */
#define ATC_SWITCHOVER  ATC_LOCAL_SET|7 /* force controller switchover */

#define	ATC_DIRECT	ATC_CMD_SET|0	/* send a command to the array */

#define	ATC_WFIFO	ATC_DIAG_SET|0	/* write to the FIFO */
#define	ATC_RFIFO	ATC_DIAG_SET|1	/* read from the FIFO */
#define	ATC_BRFIFO	ATC_DIAG_SET|2	/* blocking read from the FIFO */
#define	ATC_WBUFFER	ATC_DIAG_SET|3	/* write to the kernel diag. buffer */
#define	ATC_RBUFFER	ATC_DIAG_SET|4	/* read from the kernel diag. buffer */
#define	ATC_BUFADDR	ATC_DIAG_SET|5	/* return the diag. buffer's address */
#define	ATC_ILEVEL	ATC_DIAG_SET|6	/* return the interrupt level being used */
#define	ATC_IVECTOR	ATC_DIAG_SET|7	/* return the interrupt vector being used */

extern ClientData DevATCInit();
extern Boolean DevATCIntr();

#endif /* _ATCREG */
