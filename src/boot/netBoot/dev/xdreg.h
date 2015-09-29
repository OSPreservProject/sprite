/*	@(#)xdreg.h 1.5 86/04/21 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _XDREG_
#define _XDREG_

/*
 * Xylogics 751 declarations
 */

/*
 * IOPB - in real memory so we can use bit fields
 */
struct xdiopb {
		/* BYTE 0 */
	u_char	xd_iserr	:1;	/* error indicator */
	u_char	xd_complete	:1;	/* completion code valid */
	u_char	xd_chain	:1;	/* command chaining */
	u_char	xd_sgm		:1;	/* scatter/gather mode */
	u_char	xd_cmd		:4;	/* command */
		/* BYTE 1 */
	u_char	xd_errno;		/* error number */
		/* BYTE 2 */
	u_char	xd_dstat;		/* drive status */
		/* BYTE 3 */
	u_char	xd_resvd;		/* reserved */
		/* BYTE 4 */
	u_char	xd_subfunc;		/* subfunction code */
		/* BYTE 5 */
	u_char	xd_fixd		:1;	/* fixed/removable media */
	u_char	xd_hdp		:1;	/* hold dual-port drive */
	u_char	xd_psel		:1;	/* priority select (dual-port) */
	u_char	xd_bht		:1;	/* black hole transfer */
	u_char			:1;
	u_char	xd_unit		:3;	/* unit number */
		/* BYTE 6 */
	u_char	xd_llength	:5;	/* link length (scatter/gather) */
#define xd_drparam xd_llength		/* used for set drive params */
#define xd_interleave xd_llength	/* used for set format params */
	u_char	xd_intpri	:3;	/* interrupt priority */
		/* BYTE 7 */
	u_char	xd_intvec;		/* interrupt vector */
	u_short	xd_nsect;		/* 8,9: sector count */
	u_short	xd_cylinder;		/* a,b: cylinder number */
#define xd_throttle xd_cylinder		/* used for write ctlr params */
	u_char	xd_head;		/* c: head number */
	u_char	xd_sector;		/* d: sector number */
	u_char	xd_bufmod;		/* e: buffer address modifier */
#define xd_hsect xd_bufmod		/* used for read drive status */
#define xd_ctype xd_bufmod		/* used for read ctlr params */
		/* BYTE F */
	u_char	xd_prio		:1;	/* high priority iopb */
	u_char			:1;
	u_char	xd_nxtmod 	:6;	/* next iopb addr modifier */
	u_int	xd_bufaddr;		/* 10-13: buffer address */
#define xd_promrev xd_bufaddr		/* used for read ctlr params */
	u_int	xd_nxtaddr;		/* 14-17: next iopb address */
	u_short	xd_cksum;		/* 18,19: iopb checksum */
	u_short	xd_eccpat;		/* 1a,1b: ECC pattern */
	u_short	xd_eccaddr;		/* 1c,1d: ECC address */
};

/*
 * Commands -- the values are shifted by a byte so they can be folded
 * with the subcommands into a single variable.
 */
#define XD_NOP		0x000	/* nop */
#define XD_WRITE	0x100	/* write */
#define XD_READ		0x200	/* read */
#define XD_SEEK		0x300	/* seek */
#define XD_RESTORE	0x400	/* drive reset */
#define XD_WPAR		0x500	/* write params */
#define XD_RPAR		0x600	/* read params */
#define XD_WEXT		0x700	/* extended write */
#define XD_REXT		0x800	/* extended read */
#define XD_DIAG		0x900	/* diagnostics */
#define XD_ABORT	0xa00	/* abort */

/*
 * Subcommands
 */
	/*
	 * seek
	 */
#define XD_RCA		0x00	/* report current address */
#define XD_SRCA		0x01	/* seek and report current address */
#define XD_SSRCI	0x02	/* start seek and report completion */
	/*
	 * read and write parameters
	 */
#define XD_CTLR		0x00	/* controller parameters */
#define XD_DRIVE	0x80	/* drive parameters */
#define XD_FORMAT	0x81	/* format parameters */
#define XD_DSTAT	0xa0	/* drive status (read only) */
	/*
	 * extended read and write
	 */
#define XD_THEAD	0x80	/* track headers */
#define XD_FORMVER	0x81	/* format (write), verify (read) */
#define XD_HDE		0x82	/* header, data and ecc */
#define XD_DEFECT	0xa0	/* defect map */
#define XD_EXTDEF	0xa1	/* extended defect map */

/*
 * Error codes
 */
#define	XDE_OK		0x00		/* command succeeded */
#define	XDE_CADR	0x10		/* cylinder addr error */
#define	XDE_HADR	0x11		/* head addr error */
#define	XDE_SADR	0x12		/* sector addr error */
#define	XDE_0CNT	0x13		/* zero sector count */
#define	XDE_ILLC	0x14		/* unimplemented command */
#define	XDE_IFL1	0x15		/* illegal field length 1 */
#define	XDE_IFL2	0x16		/* illegal field length 2 */
#define	XDE_IFL3	0x17		/* illegal field length 3 */
#define	XDE_IFL4	0x18		/* illegal field length 4 */
#define	XDE_IFL5	0x19		/* illegal field length 5 */
#define	XDE_IFL6	0x1a		/* illegal field length 6 */
#define	XDE_IFL7	0x1b		/* illegal field length 7 */
#define	XDE_ISGL	0x1c		/* illegal scatter/gather length */
#define	XDE_NSEC	0x1d		/* not enough sectors/track */
#define	XDE_ALGN	0x1e		/* iopb alignment error */
#define	XDE_LINT	0x20		/* lost interrupt */
#define	XDE_FERR	0x21		/* fatal error */
#define	XDE_FECC	0x30		/* fixed ecc error */
#define	XDE_ECCI	0x31		/* ecc error ignored */
#define	XDE_SRTY	0x32		/* seek retry */
#define	XDE_SFTR	0x33		/* soft retry */
#define	XDE_HECC	0x40		/* hard ecc error */
#define	XDE_HDNF	0x41		/* header not found */
#define	XDE_NRDY	0x42		/* drive not ready */
#define	XDE_OPTO	0x43		/* operation timeout */
#define	XDE_DMAT	0x44		/* DMAC timeout */
#define	XDE_DSEQ	0x45		/* disk dequencer error */
#define	XDE_BPER	0x46		/* buffer parity error */
#define	XDE_DPBY	0x47		/* dual port busy */
#define	XDE_HDEC	0x48		/* header ecc error */
#define	XDE_RVER	0x49		/* read verify error */
#define	XDE_FDMA	0x4a		/* fatal DMAC error */
#define	XDE_VMEB	0x4b		/* vmebus error */
#define	XDE_DFLT	0x60		/* drive faulted */
#define	XDE_CHER	0x61		/* cylinder header error */
#define	XDE_HHER	0x62		/* head header error */
#define	XDE_OFCL	0x63		/* drive not on cylinder */
#define	XDE_SEEK	0x64		/* drive seek error */
#define	XDE_SSIZ	0x70		/* illegal sector size */
#define	XDE_FIRM	0x71		/* firmware failure */
#define	XDE_SECC	0x80		/* soft ecc error */
#define	XDE_IRAM	0x81		/* iram checksum error */
#define XDE_ABRT	0x82		/* abort by command */
#define XDE_PROT	0x90		/* write protect error */
#define	XDE_UNKN	0xff		/* unknown error */

/*
 * Miscellaneous defines.
 */
#define XD_THROTTLE	32		/* 32 words/transfer */
#define XDUNPERC	2		/* max # of units per controller */

/*
 * Structure definition and macros used for a sector header.
 */
#define XD_HDRSIZE	4		/* bytes/sector header */

struct xdhdr {
	/* Byte 0 */
	u_char	xdh_cyl_lo;
	/* Byte 1 */
	u_char	xdh_cyl_hi;
	/* Byte 2 */
	u_char	xdh_head;
	/* Byte 3 */
	u_char	xdh_sec;
};

#endif _XDREG_
