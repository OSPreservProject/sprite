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

/*
 * The address register for DMA transfers.
 */
typedef int DMARegister;

/*
 * If this bit is set in the DMA Register then the transfer is a write.
 */
#define DMA_WRITE	0x80000000

/*
 * MAX_SCSIC90_CTRLS - Maximum number of SCSIC90 controllers attached to the
 *		     system. 
 */
#define	MAX_SCSIC90_CTRLS	4

#define REG_OFFSET	0
#define	DMA_OFFSET	0x40000
#define BUFFER_OFFSET	0x80000
#define ROM_OFFSET	0xc0000

#define CLOCKCONV 5

#define FLUSH_BYTES(x,y,z)  bcopy((x),(y),(z));

#define EMPTY_BUFFER() Mach_EmptyWriteBuffer()
#endif
