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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMELINK
#define _VMELINK

/*
 * Sun4 address space the link occupies (2=d16, 3=d32)
 */
#define VMELINK_ADDR_SPACE		2

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

/*
 * These are the special VME link IO control calls.  They
 * perform the following functions (and all constants start with
 * IOC_VMELINK_).
 *
 * STATUS	return the link's status in the VMElinkStatus structure.
 * PING_REMOTE	return SUCCESS if the remote link card is up
 * MAP_MEMORY	map an area of memory specified by a VMElinkMapRequest struct
 * NO_ADDRMOD	turn off the address modifier register
 * SET_ADDRMOD	set the address modifier to use instead of the VME bus default
 * NO_WINDOW	turn off the paging register on the link card
 * SET_WINDOW	set the paging register on the link card and turn on paging
 * SET_WINDOW_SIZE set the size of the paging register window
 * LOW_VME	seeks refer to addresses 0-7fffffff
 * HIGH_VME	seeks refer to addresses 8000000-ffffffff
 * DEBUG_ON	turns on debugging statements for the VME link driver
 * DEBUG_OFF	turns off debugging statements
 */
#define IOC_VMELINK_STATUS		(IOC_VMELINK | 1)
#define IOC_VMELINK_PING_REMOTE		(IOC_VMELINK | 3)
#define IOC_VMELINK_MAP_MEMORY		(IOC_VMELINK | 4)
#define IOC_VMELINK_NO_ADDRMOD		(IOC_VMELINK | 5)
#define IOC_VMELINK_SET_ADDRMOD		(IOC_VMELINK | 6)
#define IOC_VMELINK_NO_WINDOW		(IOC_VMELINK | 7)
#define IOC_VMELINK_SET_WINDOW		(IOC_VMELINK | 8)
#define IOC_VMELINK_SET_WINDOW_SIZE	(IOC_VMELINK | 9)
#define IOC_VMELINK_LOW_VME		(IOC_VMELINK | 10)
#define IOC_VMELINK_HIGH_VME		(IOC_VMELINK | 11)
#define IOC_VMELINK_DEBUG_ON		(IOC_VMELINK | 12)
#define IOC_VMELINK_DEBUG_OFF		(IOC_VMELINK | 13)

#endif /* _VMELINK */
