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
 * $Header: /sprite/src/lib/include/dev/RCS/vmelink.h,v 1.4 92/07/09 18:28:31 elm Exp Locker: elm $ SPRITE (Berkeley)
 */

#ifndef _VMELINK
#define _VMELINK

/*
 * Sun4 address space the link occupies (2=d16, 3=d32)
 */
#define DEV_VMELINK_ADDR_SPACE		2

/*
 * Size of the VME link useful address space in pages.
 */
#define DEV_VMELINK_VME_START_ADDR	0xff400000
#define DEV_VMELINK_VME_ADDR_SIZE	0x00020000
#define DEV_VMELINK_NUM_PAGES		(VMELINK_VME_ADDR_SIZE / PAGSIZ)

/*
 * Flags used to set the local command register.
 */
#define DEV_VMELINK_CLEAR_LOCAL_ERRS	0x80
#define DEV_VMELINK_CLEAR_LOCAL_PF_INT	0x40
#define DEV_VMELINK_SET_REMOTE_PT_INT	0x20
#define DEV_VMELINK_LOCAL_DISABLE_INT	0x04

/*
 * Flags in the status registers.
 */
#define DEV_VMELINK_PARITY_ERROR		0x80
#define DEV_VMELINK_REMOTE_BUSERR		0x40
#define DEV_VMELINK_LOCAL_PR_INT		0x20
#define DEV_VMELINK_LOCAL_PT_INT		0x02
#define DEV_VMELINK_REMOTE_DOWN			0x01

/*
 * Flags used to set remote command register 1.
 */
#define DEV_VMELINK_REMOTE_RESET		0x80
#define DEV_VMELINK_CLEAR_REMOTE_PT_INT		0x40
#define DEV_VMELINK_SET_REMOTE_PR_INT		0x20
#define DEV_VMELINK_LOCK_VME			0x10
#define DEV_VMELINK_USE_PAGE_REG		0x08

/*
 * Flags used to set remote command register 2.
 */
#define	DEV_VMELINK_REMOTE_PAUSE_16		0x80
#define DEV_VMELINK_REMOTE_USE_ADDRMOD		0x40
#define DEV_VMELINK_REMOTE_DMA_BLOCK_MODE	0x20
#define DEV_VMELINK_REMOTE_DISABLE_INT		0x10

/*
 * Window size flags (for IOC_VMELINK_SET_WINDOW_SIZE IO control).
 */
#define DEV_VMELINK_WINDOW_SIZE_64K		0x00
#define DEV_VMELINK_WINDOW_SIZE_128K		0x01
#define DEV_VMELINK_WINDOW_SIZE_256K		0x03
#define DEV_VMELINK_WINDOW_SIZE_512K		0x07
#define DEV_VMELINK_WINDOW_SIZE_1M		0x0f

#define	DEV_VMELINK_MIN_DMA_SIZE	256
#define	DEV_VMELINK_DMA_START		0x80
#define	DEV_VMELINK_DMA_LONGWORD	0x10
#define	DEV_VMELINK_DMA_LOCAL_PAUSE	0x08
#define	DEV_VMELINK_DMA_ENABLE_INTR	0x04
#define	DEV_VMELINK_DMA_DONE		0x02
#define	DEV_VMELINK_DMA_BLOCK_MODE	0x01
#define	DEV_VMELINK_DMA_LOCAL_TO_REMOTE	0x20
#define	DEV_VMELINK_DMA_REMOTE_TO_LOCAL	0x00

/*
 *	This is the control register structure for the VME link
 *	boards.  All of the rsvX fields are listed as reserved in
 *	the manual.
 *
 */

typedef struct DevVMElinkCtrlRegs {
    volatile unsigned char rsv0;
    volatile unsigned char LocalCmd;		/* local command register */
    volatile unsigned char rsv1;
    volatile unsigned char LocalStatus;		/* local status reg */
    volatile unsigned char LocalAddrMod;	/* local addr modifier reg */
    volatile unsigned char rsv3;
    volatile unsigned char rsv4;
    volatile unsigned char LocalIntrVec;	/* local intr vector reg */
    volatile unsigned char RemoteCmd2;		/* remote command register 2 */
    volatile unsigned char RemoteCmd1;		/* remote command register 1 */
    volatile unsigned char RemotePageAddrHigh;	/* hi 8 bits remote pg addr */
    volatile unsigned char RemotePageAddrLow;	/* low 8 bits remote pg addr */
    volatile unsigned char RemoteAddrMod;	/* remote address modifier */
    volatile unsigned char rsv5;
    volatile unsigned char RemoteIackReadHigh;	/* intr acknowledge read hi */
    volatile unsigned char RemoteIackReadLow;	/* intr acknowledge read low */
} DevVMElinkCtrlRegs;

typedef struct DevVMElinkDmaRegs {
    volatile unsigned char localDmaCmdReg;
    volatile unsigned char rsv0;
    volatile unsigned char localDmaAddr3;
    volatile unsigned char localDmaAddr2;
    volatile unsigned char localDmaAddr1;
    volatile unsigned char localDmaAddr0;
    volatile unsigned char dmaLength2;
    volatile unsigned char dmaLength1;
    volatile unsigned char rsv1;
    volatile unsigned char rsv2;
    volatile unsigned char remoteDmaAddr3;
    volatile unsigned char remoteDmaAddr2;
    volatile unsigned char remoteDmaAddr1;
    volatile unsigned char remoteDmaAddr0;
    volatile unsigned char rsv3;
    volatile unsigned char rsv4;
} DevVMElinkDmaRegs;

/*
 * Used in the IOControl status call.
 */
typedef struct DevVMElinkStatus {
    int LocalStatus;
    int RemoteStatus;
} DevVMElinkStatus;

/*
 * Structure used to map VME bus addresses into kernel memory.
 */
typedef struct DevVMElinkMapRequest {
    void *VMEAddress;		/* VME address to map */
    int mapSize;		/* number of bytes to map in */
    void *kernelAddress;	/* resulting kernel address */
} DevVMElinkMapRequest;

#define	DEV_VMELINK_TO_REMOTE	0xf5
#define	DEV_VMELINK_TO_LOCAL	0x3d

/*
 * Used to access memory over the link board.
 */
typedef struct DevVMElinkAccessMem {
    Address	destAddress;
    int		size;
    int		direction;
    int		data[1];
} DevVMElinkAccessMem;

#define IOC_VMELINK			(16 << 16)

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
#define IOC_VMELINK_DEBUG_ON		(IOC_VMELINK | 2)
#define IOC_VMELINK_DEBUG_OFF		(IOC_VMELINK | 3)
#define IOC_VMELINK_PING_REMOTE		(IOC_VMELINK | 4)
#define IOC_VMELINK_NO_ADDRMOD		(IOC_VMELINK | 5)
#define IOC_VMELINK_SET_ADDRMOD		(IOC_VMELINK | 6)
#define IOC_VMELINK_LOW_VME		(IOC_VMELINK | 7)
#define IOC_VMELINK_HIGH_VME		(IOC_VMELINK | 8)
#define	IOC_VMELINK_READ_BOARD_STATUS	(IOC_VMELINK | 9)
#define	IOC_VMELINK_RESET		(IOC_VMELINK | 10)
#define	IOC_VMELINK_READ_REG		(IOC_VMELINK | 11)
#define	IOC_VMELINK_WRITE_REG		(IOC_VMELINK | 12)
#define	IOC_VMELINK_ACCESS_REMOTE_MEMORY (IOC_VMELINK | 13)
#define	IOC_VMELINK_SAFE_COPY_ON	(IOC_VMELINK | 14)
#define	IOC_VMELINK_SAFE_COPY_OFF	(IOC_VMELINK | 15)
#define	IOC_VMELINK_REMOTE_BLOCK_MODE_ON  (IOC_VMELINK | 16)
#define	IOC_VMELINK_REMOTE_BLOCK_MODE_OFF (IOC_VMELINK | 17)
#define	IOC_VMELINK_SET_MIN_DMA_SIZE	(IOC_VMELINK | 18)
#define	IOC_VMELINK_BLOCK_IO		(IOC_VMELINK | 19)

#endif /* _VMELINK */
