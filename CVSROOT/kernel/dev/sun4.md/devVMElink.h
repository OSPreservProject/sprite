/*
 * devVMElink.h --
 *
 *	Internal declarations of interface to the Bit-3 VME
 *	link driver routines.
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

#ifndef _DEVVMELINK
#define _DEVVMELINK

#include <list.h>
#include <sys/param.h>
#include "devTypes.h"
#include "devQueue.h"
#include "devBlockDevice.h"
#include "dev/vmelink.h"

/*
 * This is the maximum number of VME link boards in a system.
 */
#define DEV_VMELINK_MAX_BOARDS		8

/*
 * Flags for the state word in the device driver information block.
 */
#define	DEV_VMELINK_STATE_DMA_IN_USE	0x1
#define	DEV_VMELINK_STATE_PAGE_MODE	0x2
#define	DEV_VMELINK_STATE_SAFE_COPY	0x4

#define	DEV_VMELINK_INTVEC0		220

#define	DEV_VMELINK_DMA_BUFSIZE_MASK	0xffffff00

#define DEV_VMELINK_MAX_TRANSFER_SIZE	(1 << 22)	/* 4 MB max */

#define DEV_VMELINK_HANDLE_MAGIC	0xface6789

typedef DevVMElinkCtrlRegs CtrlRegs;
typedef DevVMElinkDmaRegs DmaRegs;

/*
 * Structure that's returned to an attach.
 */
typedef struct DevVMElinkHandle {
    DevBlockDeviceHandle blockHandle;
    struct VMELinkInfo *linkInfo;
    unsigned int magic;
} DevVMElinkHandle;

/*
 *	This is the info stored for each VME link board.
 */
typedef struct VMELinkInfo {
    int unit;			/* unit # for this link */
    unsigned int state;
    CtrlRegs	*regArea;
    DmaRegs	*dmaRegs;
    unsigned int addrMsb;	/* MSbit of VME addresses, since offsets */
    				/* into a file can only be 31 bits long */
    unsigned char LocalFlags;	/* flags used to set the local and remote */
    unsigned char RemoteFlags1;	/* command registers.  They are kept here */
    unsigned char RemoteFlags2;	/* so the driver can fool with the link. */
    int vectorNumber;
    Sync_Semaphore mutex;
    Address smallMap;		/* 64K window for use with window register */
    int minDmaSize;		/* minimum size to DMA instead of bcopy */
    unsigned int position;	/* current position for read/write */
    List_Links		reqHdr;	/* queue of requests to this link board */
    DevVMElinkHandle handle;	/* handle to return to attach() */
    int numAttached;		/* # of attach calls (w/o releasing) */
    struct DevVMElinkReq *curReq;
} VMELinkInfo;

typedef struct DevVMElinkReq {
    List_Links		links;
    VMELinkInfo		*linkInfo;	/* -> link info for this request */
    int			operation;	/* FS_READ or FS_WRITE */
    unsigned int	startAddress;	/* start addr in remote memory */
    int			length;		/* length of xfer */
    Address		buffer;		/* addr of local buffer */
    Address		dmaSpace;	/* addr of DMA space (NIL if no DMA) */
    int			dmaSize;	/* # of bytes DMAed */
    DevBlockDeviceRequest *origReq;	/* -> original request block */
    ReturnStatus	status;
    ClientData		clientData;	/* data to pass to callback */
} DevVMElinkReq;

extern ClientData DevVMElinkInit ();
extern ReturnStatus DevVMElinkOpen ();
extern ReturnStatus DevVMElinkRead ();
extern ReturnStatus DevVMElinkWrite ();
extern ReturnStatus DevVMElinkIOControl ();
extern Boolean DevVMElinkIntr ();
extern DevBlockDeviceHandle *DevVMElinkAttach ();
extern ReturnStatus DevVMElinkRelease ();
extern ReturnStatus DevVMElinkBlockIO ();
extern ReturnStatus DevVMElinkBlockIOControl ();
extern Address VmMach_MapInDevicePages ();

#endif /* _DEVVMELINK */
