/*
 * vmelink.h --
 *
 *	Declarations of interface to the Bit-3 VME link driver routines.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/vmelink.h,v 1.1 90/12/05 12:25:40 elm Exp $ SPRITE (Berkeley)
 */

#ifndef _VMELINK
#define _VMELINK

/*
 * Size of the VME link useful address space in pages.
 */
#define VMELINK_VME_START_ADDR		0xff400000
#define VMELINK_VME_ADDR_SIZE		0x00020000
#define VMELINK_NUM_PAGES		(VMELINK_VME_ADDR_SIZE / PAGSIZ)

/*
 * Flags used to set the local command register.
 */
#define VMELINK_CLEAR_LOCAL_ERRS	0x80
#define VMELINK_CLEAR_LOCAL_PF_INT	0x40
#define VMELINK_SET_REMOTE_PT_INT	0x20
#define VMELINK_DISABLE_LOCAL_INT	0x04

/*
 * Flags in the status registers.
 */
#define VMELINK_PARITY_ERROR		0x80
#define VMELINK_REMOTE_BUSERR		0x40
#define VMELINK_LOCAL_PR_INT		0x20
#define VMELINK_LOCAL_PT_INT		0x02
#define VMELINK_REMOTE_DOWN		0x01

/*
 * Flags used to set remote command register 1.
 */
#define VMELINK_REMOTE_RESET		0x80
#define VMELINK_CLEAR_REMOTE_PT_INT	0x40
#define VMELINK_SET_REMOTE_PR_INT	0x20
#define VMELINK_LOCK_VME		0x10
#define VMELINK_USE_PAGE_REG		0x08

/*
 * Flags used to set remote command register 2.
 */
#define VMELINK_REMOTE_USE_ADDRMOD	0x40
#define VMELINK_REMOTE_BLKMODE_DMA	0x20
#define VMELINK_DISABLE_REMOTE_INT	0x10

/*
 * Window size flags (for IOC_VMELINK_SET_WINDOW_SIZE IO control).
 */
#define VMELINK_WINDOW_SIZE_64K		0x00
#define VMELINK_WINDOW_SIZE_128K	0x01
#define VMELINK_WINDOW_SIZE_256K	0x03
#define VMELINK_WINDOW_SIZE_512K	0x07
#define VMELINK_WINDOW_SIZE_1M		0x0f

/*
 * Used in the IOControl status call.
 */
typedef struct VMElinkStatus {
    int LocalStatus;
    int RemoteStatus;
} VMElinkStatus;

/*
 * Structure used to map VME bus addresses into kernel memory.
 */
typedef struct VMElinkMapRequest {
    void *VMEAddress;		/* VME address to map */
    int mapSize;		/* number of bytes to map in */
    void *kernelAddress;	/* resulting kernel address */
} VMElinkMapRequest;

#define IOC_VMELINK			(13 << 16)

#define IOC_VMELINK_STATUS		(IOC_VMELINK | 0x1)
#define IOC_VMELINK_PING_REMOTE		(IOC_VMELINK | 0x3)
#define IOC_VMELINK_MAP_MEMORY		(IOC_VMELINK | 0x4)
#define IOC_VMELINK_NO_ADDRMOD		(IOC_VMELINK | 0x5)
#define IOC_VMELINK_SET_ADDRMOD		(IOC_VMELINK | 0x6)
#define IOC_VMELINK_NO_WINDOW		(IOC_VMELINK | 0x7)
#define IOC_VMELINK_SET_WINDOW		(IOC_VMELINK | 0x8)
#define IOC_VMELINK_SET_WINDOW_SIZE	(IOC_VMELINK | 0x9)

#endif /* _VMELINK */
