/* 
 * devSCSIC90Mach.h --
 *
 *	Def'ns specific to the SCSI NCR 53C9X Host Adaptor which
 *	depend on the machine architecture.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *$Header$ SPRITE (Berkeley)
 */

#ifndef _DEVSCSIC90MACH
#define _DEVSCSIC90MACH

#include "devAddrs.h"

/* Board offsets of the dma and scsi registers accessed directly by driver */
typedef struct DMARegs {
    unsigned int	ctrl;	/* Control/status register. */
    unsigned int	addr;	/* Address register. */
    unsigned int	count;	/* Byte count register. */
    unsigned int	diag;	/* Diagnostic register. */
} DMARegs;

extern volatile DMARegs	*dmaRegsPtr;

/* Format of DMA Control/Status register */
#define	DMA_DEV_ID_MASK		0xF0000000	/* Device ID bits. */
#define	DMA_TERM_CNT		0x00004000	/* Byte counter has expired. */
#define	DMA_EN_CNT		0x00002000	/* Enable byte count reg. */
#define	DMA_BYTE_ADDR		0x00001800	/* Next byte number. */
#define	DMA_REQ_PEND		0x00000400	/* Request pending. */
#define	DMA_EN_DMA		0x00000200	/* Enable DMA. */
#define	DMA_READ		0x00000100	/* 0==WRITE. */
#define	DMA_RESET		0x00000080	/* Like hardware reset. */
#define	DMA_DRAIN		0x00000040	/* Drain remaining pack regs. */
#define	DMA_FLUSH		0x00000020	/* Force pack cnt, err, to 0. */
#define	DMA_INT_EN		0x00000010	/* Enable interrupts. */
#define	DMA_PCK_CNT		0x00000006	/* # of bytes in pack reg. */
#define	DMA_ERR_PEND		0x00000002	/* Error pending. */
#define	DMA_INT_PEND		0x00000001	/* Interrupt pending. */

/*
 * MAX_SCSIC90_CTRLS - Maximum number of SCSIC90 controllers attached to the
 *		     system. Right now I set this to 1, since we'll just
 *		     use the default built-in on-board slot.
 */
#define	MAX_SCSIC90_CTRLS	1

#define CLOCKCONV 4

extern int	dmaControllerActive;

/* parameter x is unused in the sun4c implementation */
#define FLUSH_BYTES(x,y,z)  VmMach_FlushByteRange((y), (z))

/* EMPTY_BUFFER is NULL in the sun4c implementation */
#define EMPTY_BUFFER() ;

#endif
