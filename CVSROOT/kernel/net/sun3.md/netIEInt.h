/*
 * netIEInt.h --
 *
 *	External definitions for the Intel on-board Ethernet controller.  See
 *      the Intel "LAN Components User's Manual" from 1984 for a 
 *	description of the definitions here.  One note is that the Intel chip 
 *	is wired in byte swapped order.  Therefore all definitions in here are 
 *	byte swapped from the ones in the user's manual.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _NETIEINT
#define _NETIEINT

#include "netEther.h"
#include "net.h"
#include "netInt.h"

/*
 * Defined constants:
 *
 * NET_IE_CONTROL_REG_ADDR	The address of the control register for the 
 *				ethernet chip.
 * NET_IE_SYS_CONF_PTR_ADDR 	Place where the system configuration pointer 
 *				must start. 
 * NET_IE_CHUNK_SIZE		The number of bytes that memory is allocated
 *				in.
 * NET_IE_MEM_SIZE	      	Amount of memory to set aside for the control 
 *				blocks.
 * NET_IE_NUM_RECV_BUFFERS  	The number of buffers that we have to receive
 *			   	packets in.
 * NET_IE_NUM_XMIT_BUFFERS  	The number of buffer descriptors that are used
 *			   	for a transmitted packet.
 * NET_IE_RECV_BUFFER_SIZE  	The size of each receive buffer.
 * NET_IE_MIN_DMA_SIZE		The smallest buffer that can be use for DMA.
 *				If a piece of a message is smaller than this
 *				then it gets copied to other storage and
 *				made the minimum size.
 * NET_IE_NULL_RECV_BUFF_DESC 	The value that is used by the controller to 
 *				indicate that a header points to no data.
 * NET_IE_NUM_XMIT_ELEMENTS 	The number of elements to preallocate for the 
 *			   	retransmission queue.
 */

#ifdef sun3
#define NET_IE_CONTROL_REG_ADDR		0xfe0c000
#define NET_IE_SYS_CONF_PTR_ADDR	0xffffff6
#endif
#ifdef sun2
#define NET_IE_CONTROL_REG_ADDR		0xee3000
#define NET_IE_SYS_CONF_PTR_ADDR	0xfffff6
#endif

#define	NET_IE_CHUNK_SIZE		32
#define	NET_IE_MEM_SIZE			2048
#define	NET_IE_NUM_RECV_BUFFERS		18
#define	NET_IE_NUM_XMIT_BUFFERS		20
#define	NET_IE_RECV_BUFFER_SIZE		NET_ETHER_MAX_BYTES
#define	NET_IE_MIN_DMA_SIZE		12
#define	NET_IE_NULL_RECV_BUFF_DESC	0xffff
#define	NET_IE_NUM_XMIT_ELEMENTS	32

/*
 * Macros to manipulate the chip.
 *
 * NET_IE_CHIP_RESET		Reset the chip.
 * NET_IE_CHANNEL_ATTENTION	Get the attention of the chip.
 * NET_IE_DELAY			Delay until the microsecond limit is up or
 *				until the condition is true.
 * NET_IE_CHECK_SCB_CMD_ACCEPT	Check to see if the command has been accepted.
 * NET_IE_ADDR_FROM_68000_ADDR	Change a 68000 address to an intel address.
 * NET_IE_ADDR_TO_68000_ADDR	Change an intel address to a 68000 address.
 */

#define NET_IE_CHIP_RESET *(char *) netIEState.controlReg = 0;
#define NET_IE_CHANNEL_ATTENTION \
	{ \
	    netIEState.controlReg->channelAttn = 1; \
	    netIEState.controlReg->channelAttn = 0; \
	}

#ifdef sun3
#define NET_IE_DELAY(condition) \
	{ \
	    register int i = (400000); \
	    while (i > 0 && !(condition)) { \
		    i--; \
	    } \
	}
#else
#define NET_IE_DELAY(condition) \
	{ \
	    register int i = (400000 >> 2); \
	    while (i > 0 && !(condition)) { \
		    i--; \
	    } \
	}
#endif

#define	NET_IE_CHECK_SCB_CMD_ACCEPT(scbPtr) \
    if (*(short *) &(scbPtr->cmdWord) != 0) { \
	NetIECheckSCBCmdAccept(scbPtr); \
    }

#define NET_IE_ADDR_FROM_68000_ADDR(src, dest) { \
    union { \
        int     i; \
        char    ch[4]; \
    } addrTo, addrFrom; \
    addrFrom.i = src; \
    addrTo.ch[0] = addrFrom.ch[3]; \
    addrTo.ch[1] = addrFrom.ch[2]; \
    addrTo.ch[2] = addrFrom.ch[1]; \
    addrTo.ch[3] = 0; \
    dest = addrTo.i; \
}

#define	NET_IE_ADDR_TO_68000_ADDR(src, dest) { \
    union { \
	int 	i; \
	char	ch[4]; \
    } addrTo, addrFrom; \
    addrFrom.i = src; \
    addrTo.ch[0] = 0; \
    addrTo.ch[1] = addrFrom.ch[2]; \
    addrTo.ch[2] = addrFrom.ch[1]; \
    addrTo.ch[3] = addrFrom.ch[0]; \
    dest = addrTo.i; \
}

/*
 * System configuration pointer.  Must be at 0xfffff6 in chip's address space.
 */

typedef struct {
    char busWidth;	/* Bus width.  0 => 16 bits.  1 => 8 bits. */
    char filler[5];	/* Unused. */
    int	 intSysConfPtr;	/* Address of intermediate system configuration 
			   pointer. */
} NetIESysConfPtr;

/*
 * Intermediate system configuration pointer.  This specifies the base of the 
 * control blocks and the offset of the System Control Block (SCB).
 */

typedef struct {
    char  busy;			/* 1 if initialization in progress. */
    char  filler;		/* Unused. */
    short scbOffset;		/* Offset of the scb. */
    int	  base;			/* Base of all control blocks. */
} NetIEIntSysConfPtr;

/*
 * The system control block status.
 */

typedef struct {
    unsigned char 		    :1;	/* Must be zero */
    unsigned char recvUnitStatus    :3;	/* Receive unit status */
    unsigned char 	            :4;	/* Must be zero */
    unsigned char cmdDone	    :1;	/* A command which has its interrupt
					   bit set completed. */
    unsigned char frameRecvd	    :1;	/* A frame received interrupt has been
					   given. */
    unsigned char cmdUnitNotActive  :1;	/* The command unit has left the active
					   state. */
    unsigned char recvUnitNotReady  :1;	/* The command unit has left the ready
					   state. */
    unsigned char                   :1;	/* Must be zero. */

    unsigned char cmdUnitStatus	    :3; /* Command unit status. */
} NetIESCBStatus;

/* 
 * The system control block command word.
 */

typedef struct {
    unsigned char reset		     :1;  /* Reset the chip. */
    unsigned char recvUnitCmd        :3;  /* The command for the receive unit */
    unsigned char 		     :4;  /* Unused. */

    unsigned char ackCmdDone         :1;  /* Ack the command completed bit in
					     the status word. */
    unsigned char ackFrameRecvd      :1;  /* Ack the frame received bit in the
					     the status word. */
    unsigned char ackCmdUnitNotActive:1;  /* Ack that the command unit became
					     not active. */
    unsigned char ackRecvUnitNotReady:1;  /* Ack that the receive unit became
					     not ready */
    unsigned char		     :1;  /* Unused. */
    unsigned char cmdUnitCmd         :3;  /* The command for the command unit */
} NetIESCBCommand;

/*
 * Define the macros to Check and acknowledge the status of the chip.
 *
 * NET_IE_CHECK_STATUS	Extract the 4 status bits out of the scb status word.
 * NET_IE_ACK		Set the bits in the scb command word that acknowledge 
 *			the status bits in the scb status word.
 * NET_IE_TRANSMITTED	Return true if a transit command finished.
 * NET_IE_RECEIVED	Return true if a packet was received.
 */

#define	NET_IE_CHECK_STATUS(scbStatus) ((*(short *) &(scbStatus)) & 0xF0)
#define	NET_IE_ACK(scbCommand, status) ((*(short *) &(scbCommand)) |= status)
#define	NET_IE_TRANSMITTED(status) status & 0xA0
#define	NET_IE_RECEIVED(status) status & 0x50

/*
 * The system control block.
 */

typedef struct {
    NetIESCBStatus	statusWord;
    NetIESCBCommand	cmdWord;
    short	        cmdListOffset;
    short	        recvFrameAreaOffset;
    short	        crcErrors;		/* Count of crc errors. */
    short		alignErrors;		/* Count of alignment errors. */
    short		resourceErrors;		/* Count of correct incoming 
						   packets discarded because
						   of lack of buffer space */
    short		overrunErrors;		/* Count of overrun packets */
} NetIESCB;

/*
 * Values for that status of the receive unit (recvUnitStatus).
 */

#define NET_IE_RUS_IDLE	        	0
#define NET_IE_RUS_SUSPENDED		1
#define NET_IE_RUS_NO_RESOURCES		2
#define NET_IE_RUS_READY	        4

/*
 * Values for that status of the command unit (cmdUnitStatus).
 */

#define NET_IE_CUS_IDLE	        	0
#define NET_IE_CUS_SUSPENDED		1
#define NET_IE_CUS_ACTIVE        	2

/*
 * Values for the command unit command (cmdUnitCommand).
 */

#define	NET_IE_CUC_NOP			0
#define	NET_IE_CUC_START		1
#define	NET_IE_CUC_RESUME		2
#define	NET_IE_CUC_SUSPEND		3
#define	NET_IE_CUC_ABORT		4

/*
 * Values for the receive unit command (recvUnitCommand).
 */

#define	NET_IE_RUC_NOP			0
#define	NET_IE_RUC_START		1
#define	NET_IE_RUC_RESUME		2
#define	NET_IE_RUC_SUSPEND		3
#define	NET_IE_RUC_ABORT	  	4

/*
 * Generic command block
 */

typedef struct {
    unsigned char 		:8;	/* Low order bits of status. */
    unsigned char cmdDone	:1;	/* Command done. */
    unsigned char cmdBusy	:1;	/* Command busy. */
    unsigned char cmdOK		:1;	/* Command completed successfully. */
    unsigned char cmdAborted	:1;	/* The command aborted. */
    unsigned char		:4;	/* High order bits of status. */
    unsigned char		:5;	/* Unused. */
    unsigned char cmdNumber	:3;	/* Command number. */
    unsigned char endOfList	:1;	/* The end of the command list. */
    unsigned char suspend	:1;	/* Suspend when command completes. */
    unsigned char interrupt	:1;	/* Interrupt when the command 
					   completes. */
    unsigned char		:5;	/* Unused. */
    short	   nextCmdBlock;	/* The offset of the next command 
					   block. */
} NetIECommandBlock;

/*
 * Command block commands.
 */

#define	NET_IE_NOP		0
#define	NET_IE_IA_SETUP  	1
#define	NET_IE_CONFIG		2
#define	NET_IE_MC_SETUP		3
#define	NET_IE_TRANSMIT		4
#define	NET_IE_TDR      	5
#define	NET_IE_DUMP     	6
#define	NET_IE_DIAGNOSE		7

/*
 * The nop command block.
 */

typedef struct {
    NetIECommandBlock	cmdBlock;
} NetIENOPCB;

/*
 * The individual address setup command block.
 */

typedef	struct {
    NetIECommandBlock	cmdBlock;	/* The command block. */
    Net_EtherAddress	etherAddress;	/* The ethernet address. */
} NetIEIASetupCB;

/*
 * The configure command block.
 */

typedef struct {
    NetIECommandBlock	cmdBlock;	/* The command block. */

    unsigned char		:4;	/* Unused. */
    unsigned char byteCount	:4;	/* Number of configuration bytes. */

    unsigned char          	:4;	/* Number of configuration bytes. */
    unsigned char fifoLimit	:4;	/* The fifo limit. */

    unsigned char saveBadFrames :1;	/* Save bad frames. */
    unsigned char srdyArdy	:1;	/* srdy/ardy. */
    unsigned char 		:6;	/* Unused. */

    unsigned char excLoopback	:1;	/* External loop back. */
    unsigned char intLoopback	:1;	/* Internal loop back. */
    unsigned char preamble	:2;	/* Preamble length code. */
    unsigned char atLoc 	:1;	/* Address and type fields are part */
    unsigned char addrLen	:3;	/* The number of address bytes. */

    unsigned char backOff	:1;	/* Backoff method. */
    unsigned char expPrio	:3;	/* Exponential priority. */
    unsigned char 		:1;	/* Unused. */
    unsigned char linPrio	:3;	/* Linear priority. */

    unsigned char interFrameSpace:8;	/* Interframe spacing. */

    unsigned char slotTimeLow	:8;	/* Low bits of slot time. */

    unsigned char numRetries	:4;	/* Number of transmit retries. */
    unsigned char 		:1;	/* Unused. */	
    unsigned char slotTimeHigh	:3;	/* High bits of the slot time. */

    unsigned char pad		:1;	/* Padding. */
    unsigned char bitStuff	:1;	/* Hdlc bit stuffing. */
    unsigned char crc16		:1;	/* CRC 16 bits or 32. */
    unsigned char noCrcInsert	:1;	/* No crc insertion. */
    unsigned char xmitOnNoCarr  :1;	/* Transmit even if no carrier sense. */
    unsigned char manch		:1;	/* Manchester or NRZ encoding. */
    unsigned char noBroadcast	:1;	/* Disable broadcasts. */
    unsigned char promisc	:1;	/* Promiscuous mode. */

    unsigned char collDetectSrc	:1;	/* Collision detect source. */
    unsigned char cdFilter	:3;	/* Collision detect filter bits. */
    unsigned char carrSenseSrc	:1;	/* Carrier sense source. */
    unsigned char carrSenseFilter:3;	

    unsigned char minFrameLength:8;	/* Minimum frame length. */

    unsigned char		:8;	/* Unused. */
} NetIEConfigureCB;

/*
 * The transmit command block.
 */

typedef struct {
    unsigned char xmitDeferred	:1;	/* Transmission deferred. */
    unsigned char heartBeat	:1;	/* Heart beat. */
    unsigned char tooManyCollisions:1;	/* Too many transmit collisions. */
    unsigned char 		:1;	/* Unused. */
    unsigned char numCollisions	:4;	/* The number of collisions 
					   experienced */
    unsigned char cmdDone	:1;	/* Command done. */
    unsigned char cmdBusy	:1;	/* Command busy. */
    unsigned char cmdOK		:1;	/* Command completed successfully. */
    unsigned char cmdAborted	:1;	/* The command aborted. */
    unsigned char 		:1;	/* Unused. */
    unsigned char noCarrSense	:1;	/* No carrier sense. */
    unsigned char noClearToSend	:1;	/* Transmission unsuccessful because
					   of loss of clear to send signal. */
    unsigned char underRun	:1; 	/* DMA underrun. */

    unsigned char		:5;	/* Unused. */
    unsigned char cmdNumber	:3;	/* Command number. */

    unsigned char endOfList	:1;	/* The end of the command list. */
    unsigned char suspend	:1;	/* Suspend when command completes. */
    unsigned char interrupt	:1;	/* Interrupt when the command 
    unsigned char 		:5;	/* Unused. */

    short	  nextCmdBlock;		/* The offset of the next command 
					   block */
    short	  bufDescOffset;	/* The offset of the buffer descriptor */
    Net_EtherAddress  destEtherAddr;	/* The ethernet address of the 
					   destination machine. */
    short	  type;			/* Ethernet packet type field. */
} NetIETransmitCB;

/* 
 * The transmit buffer descriptor.
 */

typedef struct {
    unsigned	char countLow	:8;	/* Low order 8 bits of count of bytes */
    unsigned	char eof	:1;	/* Last buffer in the packet. */
    unsigned	char		:1;	/* Unused. */
    unsigned	char countHigh	:6;	/* High order 6 bits of the count. */

    short	nextTBD;		/* Offset of the next transmit 
					   buffer descriptor. */
    int		bufAddr;		/* Address of buffer of data. */
} NetIETransmitBufDesc;

/*
 * The receive frame descriptor.
 */

typedef struct NetIERecvFrameDesc {
    unsigned char shortFrame	:1;	/* Was a short frame. */
    unsigned char noEOF		:1;	/* No EOF (bitstuffing mode only). */
    unsigned char 		:6;	/* Unused. */

    unsigned char done		:1;	/* Frame completely stored. */
    unsigned char busy		:1;	/* Busy storing frame. */
    unsigned char ok		:1;	/* Frame received OK */
    unsigned char 		:1;	/* Unused. */
    unsigned char crcError	:1;	/* Received packet had a crc error */
    unsigned char alignError	:1;	/* Received packet had an alignment 
					   error */
    unsigned char outOfBufs	:1;	/* Receive unit ran out of memory */
    unsigned char overrun	:1;	/* DMA overrun. */

    unsigned char 		:8;	/* Unused. */

    unsigned char endOfList	:1;	/* End of list. */
    unsigned char suspend	:1;	/* Suspend when done receiving. */
    unsigned char 		:6;	/* Unused. */

    short nextRFD;			/* Next receive frame descriptor. */
    short recvBufferDesc;		/* Offset of the first receive buffer
					   descriptor. */

    Net_EtherAddress destAddr;		/* Destination ethernet address. */
    Net_EtherAddress srcAddr;		/* Source ethernet address. */
    short 	 type;			/* Ethernet packet type. */
    struct NetIERecvFrameDesc *realNextRFD; /* The 68000 address of the next
					       receive frame descriptor. */
} NetIERecvFrameDesc;

/*
 * Receive buffer descriptor.
 */

typedef struct NetIERecvBufDesc {
    unsigned char countLow	:8;	/* Low order bits of the count of bytes
					   in this buffer. */
    unsigned char eof		:1;	/* Last buffer for this packet. */
    unsigned char countValid	:1;	/* The value in the count field is 
					   valid. */
    unsigned char countHigh	:6;	/* High order bits of the count. */

    short nextRBD;			/* Next receive buffer descriptor. */
    int	  bufAddr;			/* The address of the buffer that
					   this descriptor puts its data. */

    unsigned char bufSizeLow	:8;	/* Low order bits of amount of bytes
					   this buffer is capable of holding */
    unsigned char endOfList	:1;	/* This is the end of the RBD list. */
    unsigned char		:1;	/* Unused. */
    unsigned char bufSizeHigh	:6;	/* High order 6 bits of the buffer 
					   size. */
    Address	realBufAddr;		/* The 68000 address of the buffer 
					   where this descriptor puts its data*/
    struct NetIERecvBufDesc *realNextRBD; /* The 6800 Address of the next
					     receive buffer descriptor. */
} NetIERecvBufDesc;

/*
 * The intel ethernet register.
 */

typedef struct {
    unsigned char noReset	:1;	/* R/W: 0 => Ethernet reset, 
						1 => Normal. */
    unsigned char noLoopback	:1;	/* R/W: 0 => Loopback, 
						1 => Normal. */
    unsigned char channelAttn	:1;	/* R/W: Channel Attention. */
    unsigned char intrEnable	:1;	/* R/W: Interrupt enable. */
    unsigned char 		:2;	/* Reserved. */
    unsigned char busError	:1;	/* R/O: DMA bus error. */
    unsigned char intrPending	:1;	/* R/O: Got an interrupt request. */
} NetIEControlRegister;

/*
 * Structure to hold all state information associated with one of these
 * chips.
 */

typedef struct {
    int			memBase;	/* Address of control block memory. */
    NetIESysConfPtr	*sysConfPtr;	/* Where the system configuration 
					   pointer is at. */
    NetIEIntSysConfPtr	*intSysConfPtr;	/* Where the intermediate system 
					   configuration pointer is at. */
    NetIESCB		*scbPtr;	/* Pointer to system control block. */
    NetIECommandBlock	*cmdBlockPtr;	/* Head of command block list */
    NetIERecvFrameDesc	*recvFrDscHeadPtr;/* Head of receive frame descriptor
					   list. */
    NetIERecvFrameDesc	*recvFrDscTailPtr;/* Tail of receive frame descriptor
					   list. */
    NetIERecvBufDesc	*recvBufDscHeadPtr;/* Head of receive buffer descriptor
					   list. */
    NetIERecvBufDesc	*recvBufDscTailPtr;/* Tail of receive buffer descriptor
					   list. */
    List_Links		*xmitList;	/* Pointer to the front of the list of
					   packets to be transmited. */
    List_Links      	*xmitFreeList;	/* Pointer to a list of unused 
					   transmission queue elements. */
    NetIETransmitCB	*xmitCBPtr;	/* Pointer to the single command block
					   for transmitting packets. */
    Boolean		transmitting;	/* Set if are currently transmitting a
					   packet. */
    Boolean		running;	/* Is the chip currently active. */
    NetIEControlRegister *controlReg;	/* The onboard device register. */
    Net_EtherAddress	etherAddress;	/* The ethernet address in reverse
					   byte order. */
} NetIEState;

/*
 * The state of all of the interfaces. 
 */
  
extern	NetIEState	netIEState;

/*
 * The table of receive buffer addresses.
 */

extern	Address	netIERecvBuffers[];

/*
 * General routines.
 */

extern	Boolean	NetIEInit();
extern	void	NetIEOutput();
extern	void	NetIEIntr();
extern	void	NetIERestart();

extern	void	NetIEReset();

/*
 * Routines for transmitting.
 */

extern	void	NetIEXmitInit();
extern	void	NetIEXmitDone();
extern	void	NetIEXmitRestart();

/*
 * Routines for the command unit.
 */

extern	void	NetIECheckSCBCmdAccept();
extern	void	NetIEExecCommand();

/*
 * Routines for the receive unit.
 */

extern	void	NetIERecvUnitInit();
extern	void	NetIERecvProcess();

/*
 * Memory routines.
 */

extern	void	NetIEMemInit();
extern	Address	NetIEMemAlloc();
extern	void	NetIEMemFree();

/*
 * Routines to convert to addresses and offsets.
 */

extern	int	NetIEAddrFrom68000Addr();
extern	int	NetIEAddrTo68000Addr();
extern	int	NetIEOffsetFrom68000Addr();
extern	int	NetIEOffsetTo68000Addr();
extern	int	NetIEShortSwap();

#endif _NETIEINT
