/*
 * netHppiInt.h --
 *
 *	Definitions for the Thinking Machines HPPI boards.  The Ultra
 *	include files are also needed, as many of the data structures
 *	are the same.  This is an "additional" file.  Eventually,
 *	there will be a device-independent set of files and a device-
 *	dependent file for the VME and TMC Ultranet connections.
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

#ifndef _NETHPPIINT
#define _NETHPPIINT

#include <dev/hppi.h>

/*
 *	These are the hardware registers for the HPPI-S board.
 */
typedef struct NetHppiSrcReg {
    volatile uint32 config;	/* configuration register */
    volatile uint32 inputFifo;	/* data input fifo */
    volatile uint32 outputFifo;	/* data output fifo */
    volatile uint32 status;	/* status and interrupt bits */
    volatile uint32 reset;	/* poke this register to reset */
} NetHppiSrcReg;

typedef struct NetHppiDestReg {
    volatile uint32 config;
    volatile uint32 inputFifo;
    volatile uint32 outputFifo;
    volatile uint32 status;
    volatile uint32 reset;
    volatile uint32 command;
    volatile uint32 commandData;
    volatile uint32 response;
    volatile uint32 responseData;
} NetHppiDestReg;

/*
 * Expand this later.
 */
typedef struct Net_HppiStats {
    int foo;
} Net_HppiStats;


#define NET_HPPI_MAX_ERROR_INFO	256
#define NET_HPPI_ROM_MAX_REPLY 4096

typedef struct NetHppiState {
    int magic;				/* for debugging purposes */
    int flags;				/* see below */
    NetHppiSrcReg *hppisReg;		/* ptr to source board registers */
    NetHppiDestReg *hppidReg;		/* ptr to dest board registers */
    Address		firstToHostVME;    /* VMEaddr of toHost XRB queue */
    Address		firstToAdapterVME; /* VMEaddr of toAdapter XRB queue */
    struct NetUltraXRB	*firstToHostPtr;  /* First XRB in queue to host. */
    struct NetUltraXRB	*nextToHostPtr;   /* Next XRB to be filled by
					   * the adapter. */
    struct NetUltraXRB	*lastToHostPtr;	  /* Last XRB in queue to host. */
    struct NetUltraXRB	*firstToAdapterPtr; /* First XRB in queue to adapter */
    struct NetUltraXRB	*nextToAdapterPtr;  /* Next XRB to be filled by host */
    struct NetUltraXRB	*lastToAdapterPtr;  /* Last XRB in queue to adapter.*/
    int			adapterVersion;	/* Version of adapter software. */
    int			priority;	/* Interrupt priority. */
    int			vector;		/* Interrupt vector. */
    int			requestLevel;	/* VME bus request level. */
    int			addressSpace;	/* Address space for queues and
					 * buffers. */
    int			maxReadPending;	/* Maximum pending datagram
					 * reads on the interface. */
    int			numReadPending;	/* Number of pending reads. */
    int			readBufferSize;	/* Size of pending read buffers. */
    int			maxWritePending; /* Maximum pending datagram writes.*/
    int			numWritePending; /* Number of pending writes. */
    Address		buffersDVMA;	/* DVMA buffers for pending reads
					 * and writes. */
    Address		buffers;	/* Address of DVMA buffers in kernel
					 * address space. */
    int			bufferSize;	/* Size of a DVMA buffer. */
    int			numBuffers;	/* Number of DVMA buffers. */
    List_Links		freeBufferListHdr; /* List of free DVMA buffers*/
    List_Links		*freeBufferList; /* Ptr to list of free DVMA
					  * buffers*/
    Sync_Condition	bufferAvail; /* Condition to wait for a free
					  * DVMA buffer. */
    Sync_Condition	toAdapterAvail; /* Condition to wait for an XRB to
					 * the adapter to become available. */
    Boolean		queuesInit;	/* TRUE => XRB queues have been 
					 * allocated. */
    List_Links		pendingXRBInfoListHdr; /* List of info about pending 
						* XRBs.*/
    List_Links		*pendingXRBInfoList;   /* Pointer to list of info about
						* pending XRBs. */
    List_Links		freeXRBInfoListHdr;/* List of free XRB info 
					    * structures. */
    List_Links		*freeXRBInfoList;  /* Pointer to list of info about
					    * pending XRBs. */
    Net_Interface	*interPtr;	   /* Interface structure associated
					    * with this board set. */
    int			boardFlags;	/* flags passed to boards */
    NetUltraTraceInfo	*tracePtr;
    NetUltraTraceInfo	traceBuffer[TRACE_SIZE];
    NetUltraXRBInfo	*tagToXRBInfo[32];
    int			srcErrorBuffer[NET_HPPI_MAX_ERROR_INFO];
    int			dstErrorBuffer[NET_HPPI_MAX_ERROR_INFO];
    int			traceSequence;
    Net_UltraStats	stats;
} NetHppiState;

typedef struct NetHppiSGElement {
    Address	addr;
    int		size;
} NetHppiSGElement;

#define	NET_HPPI_STATE_MAGIC	0x29495969

/*
 * flags for use in flags field of NetHppiState
 */
#define	NET_HPPI_STATE_EXIST 		0x1	/* board set exists */
#define	NET_HPPI_STATE_NORMAL		0x2	/* boards in normal state */
#define NET_HPPI_STATE_START		0x4	/* adapter started */
#define NET_HPPI_STATE_ECHO		0x10	/* boards echoing packets */
#define NET_HPPI_STATE_SINK		0x20	/* boards sinking packets */
#define NET_HPPI_STATE_STATS		0x40	/* collect statistics */
#define NET_HPPI_STATE_DSND_TEST	0x80
#define	NET_HPPI_STATE_SRC_EPROM	0x100	/* src board running EPROM */
#define NET_HPPI_STATE_DST_EPROM	0x200	/* dst board running EPROM */
#define NET_HPPI_STATE_NOT_SETUP	0x400	/* boards not set up */
#define NET_HPPI_STATE_SRC_ERROR	0x1000	/* src board error occurred */
#define NET_HPPI_STATE_DST_ERROR	0x2000	/* dest board error occurred */
#define NET_HPPI_OWN_SRC_FIFO		0x10000	/* server owns HPPI-S FIFO */

#define NET_HPPI_RESET_BOARD		0x1	/* reset TMC board */
#define NET_HPPI_RESET_CPU		0x2	/* reset TMC board CPU */

#define NET_HPPI_NUM_TO_HOST		4	/* Number of XRBs to host */
#define NET_HPPI_NUM_TO_ADAPTER		4	/* Number of XRBs to HPPI */

#define NET_HPPI_PENDING_READS		NET_HPPI_NUM_TO_HOST
#define NET_HPPI_PENDING_WRITES		NET_HPPI_NUM_TO_ADAPTER


#define NET_HPPI_DST_CTRL_OFFSET	0xc000	/* offset from base address
						 * of HPPI-D control regs */
#define NET_HPPI_SRC_CTRL_OFFSET	0x8000	/* offset from base address
						 * of HPPI-S control regs */
#define NET_HPPI_MAX_INTERFACES		2
#define NET_HPPI_INTERRUPT_PRIORITY	3
#define NET_HPPI_VME_REQUEST_LEVEL	3
#define NET_HPPI_INTERRUPT_VECTOR	0xc9
#define NET_HPPI_CONTROL_REG_ADDR	0xff800000
#define NET_HPPI_DUAL_PORT_RAM		0xff900000
#define NET_HPPI_VME_ADDRESS_SPACE	0
#define NET_HPPI_MAX_BYTES		0x8000
#define NET_HPPI_MIN_BYTES		0x100

#define NET_HPPI_RESET_DELAY		1000000
#define NET_HPPI_DELAY			60000000 /* 20 seconds, so prints
						  * on the board can finish */

/*
 * Flags for call to NetHppiSendCmd
 */
#define	NET_HPPI_SRC_CMD		0x1
#define NET_HPPI_KEEP_SRC_FIFO		0x2

#define NET_HPPI_SRC_STATUS_ALIVE_MASK	0x18000000
#define NET_HPPI_SRC_STATUS_ERROR	0x08000000
#define NET_HPPI_DST_STATUS_ALIVE_MASK	0x18000000
#define NET_HPPI_DST_STATUS_ERROR	0x08000000
#define NET_HPPI_SRC_STATUS_INTR	0x3e
#define NET_HPPI_DST_STATUS_INTR	0x7f

#define NET_HPPI_INTERRUPT_CONFIG	((NET_HPPI_VME_REQUEST_LEVEL << 8) | \
					 NET_HPPI_INTERRUPT_VECTOR |	\
					 DEV_HPPI_INTR_ENB_29K |	\
					 DEV_HPPI_INTR_ENB |		\
					 DEV_HPPI_INTR_ROAK)
#define NET_HPPI_BUS_CONFIG		(3 << DEV_HPPI_BUS_REQ_SHIFT)

#define NET_HPPI_SRC_CONFIG_VALUE	(NET_HPPI_INTERRUPT_CONFIG)

#define NET_HPPI_DST_CONFIG_VALUE	(NET_HPPI_INTERRUPT_CONFIG | \
					 NET_HPPI_BUS_CONFIG)

extern ReturnStatus	NetHppiInfo _ARGS_ ((NetHppiState *statePtr));
extern ReturnStatus	NetHppiHardReset _ARGS_((Net_Interface *interPtr));
extern ReturnStatus	NetHppiInit _ARGS_((Net_Interface *interPtr));
extern void		NetHppiIntr _ARGS_((Net_Interface *interPtr,
					    Boolean polling));
extern ReturnStatus	NetHppiIOControl _ARGS_((Net_Interface *interPtr,
						 Fs_IOCParam *ioctlPtr,
						 Fs_IOReply *replyPtr));
extern ReturnStatus	NetHppiReset _ARGS_((Net_Interface *interPtr));
extern void		Net_HppiReset _ARGS_((Net_Interface *interPtr));
extern ReturnStatus	NetHppiOutput _ARGS_((Net_Interface *interPtr,
			      Address hdrPtr,
			      Net_ScatterGather *scatterGatherPtr,
			      int scatterGatherLength, Boolean rpc,
			      ReturnStatus *statusPtr));

#endif /* _NETHPPIINT */
