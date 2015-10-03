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
 * NET_IE_DELAY_CONST		Number of loops to poll the ethernet chip
 *				before giving up. (see NET_IE_DELAY())
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
#define	NET_IE_DELAY_CONST		400000
#endif /* sun3 */


#ifdef sun2
#define NET_IE_CONTROL_REG_ADDR		0xee3000
#define NET_IE_SYS_CONF_PTR_ADDR	0xfffff6
#define	NET_IE_DELAY_CONST		(400000>>2)
#endif /* sun2 */

#ifdef sun4
#define NET_IE_CONTROL_REG_ADDR		0xffd0c000
#define NET_IE_SYS_CONF_PTR_ADDR	0xfffffff4
#define	NET_IE_DELAY_CONST		(400000<<2)
#endif /* sun4 */

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
 * NET_IE_ADDR_FROM_SUN_ADDR	Change a SUN address to an intel address.
 * NET_IE_ADDR_TO_SUN_ADDR	Change an intel address to a SUN address.
 */

#define NET_IE_CHIP_RESET (*(volatile char *) netIEState.controlReg = 0)
#define NET_IE_CHANNEL_ATTENTION \
	{ \
	    netIEState.controlReg->channelAttn = 1; \
	    netIEState.controlReg->channelAttn = 0; \
	}

#define NET_IE_DELAY(condition) \
	{ \
	    register int i = (NET_IE_DELAY_CONST); \
	    while (i > 0 && !(condition)) { \
		    i--; \
	    } \
	}

#define	NET_IE_CHECK_SCB_CMD_ACCEPT(scbPtr) \
    if (*(short *) &(scbPtr->cmdWord) != 0) { \
	NetIECheckSCBCmdAccept(scbPtr); \
    }

#define NET_IE_ADDR_FROM_SUN_ADDR(src, dest) { \
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

#define	NET_IE_ADDR_TO_SUN_ADDR(src, dest) { \
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
#ifdef sun4
    short padding;	/* Since structures can't start on a half-word boundry
			 * on the sun4, we start the structure on a word 
			 * boundry and include 2 bytes of padding.
			 */
#endif /* sun4 */
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
    unsigned short 		    :1;	/* Must be zero */
    unsigned short recvUnitStatus   :3;	/* Receive unit status */
    unsigned short 	            :4;	/* Must be zero */
    unsigned short cmdDone	    :1;	/* A command which has its interrupt
					   bit set completed. */
    unsigned short frameRecvd	    :1;	/* A frame received interrupt has been
					   given. */
    unsigned short cmdUnitNotActive :1; /* The command unit has left the active
					   state. */
    unsigned short recvUnitNotReady :1; /* The command unit has left the ready
					   state. */
    unsigned short                  :1;	/* Must be zero. */

    unsigned short cmdUnitStatus    :3; /* Command unit status. */
} NetIESCBStatus;

/* 
 * The system control block command word.
 */

typedef struct {
    unsigned short reset	     :1;  /* Reset the chip. */
    unsigned short recvUnitCmd       :3;  /* The command for the receive unit */
    unsigned short 		     :4;  /* Unused. */

    unsigned short ackCmdDone         :1;  /* Ack the command completed bit in
					     the status word. */
    unsigned short ackFrameRecvd      :1;  /* Ack the frame received bit in the
					     the status word. */
    unsigned short ackCmdUnitNotActive:1;  /* Ack that the command unit became
					     not active. */
    unsigned short ackRecvUnitNotReady:1;  /* Ack that the receive unit became
					     not ready */
    unsigned short		    :1;  /* Unused. */
    unsigned short cmdUnitCmd       :3;  /* The command for the command unit */
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
    unsigned int 		:8;	/* Low order bits of status. */
    unsigned int cmdDone	:1;	/* Command done. */
    unsigned int cmdBusy	:1;	/* Command busy. */
    unsigned int cmdOK		:1;	/* Command completed successfully. */
    unsigned int cmdAborted	:1;	/* The command aborted. */
    unsigned int		:4;	/* High order bits of status. */
    unsigned int		:5;	/* Unused. */
    unsigned int cmdNumber	:3;	/* Command number. */
    unsigned int endOfList	:1;	/* The end of the command list. */
    unsigned int suspend	:1;	/* Suspend when command completes. */
    unsigned int interrupt	:1;	/* Interrupt when the command 
					   completes. */
    unsigned int		:5;	/* Unused. */
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

    unsigned int		:4;	/* Unused. */
    unsigned int byteCount	:4;	/* Number of configuration bytes. */

    unsigned int          	:4;	/* Number of configuration bytes. */
    unsigned int fifoLimit	:4;	/* The fifo limit. */

    unsigned int saveBadFrames :1;	/* Save bad frames. */
    unsigned int srdyArdy	:1;	/* srdy/ardy. */
    unsigned int 		:6;	/* Unused. */

    unsigned int excLoopback	:1;	/* External loop back. */
    unsigned int intLoopback	:1;	/* Internal loop back. */
    unsigned int preamble	:2;	/* Preamble length code. */
    unsigned int atLoc 	:1;	/* Address and type fields are part */
    unsigned int addrLen	:3;	/* The number of address bytes. */

    unsigned int backOff	:1;	/* Backoff method. */
    unsigned int expPrio	:3;	/* Exponential priority. */
    unsigned int 		:1;	/* Unused. */
    unsigned int linPrio	:3;	/* Linear priority. */

    unsigned int interFrameSpace:8;	/* Interframe spacing. */

    unsigned int slotTimeLow	:8;	/* Low bits of slot time. */

    unsigned int numRetries	:4;	/* Number of transmit retries. */
    unsigned int 		:1;	/* Unused. */	
    unsigned int slotTimeHigh	:3;	/* High bits of the slot time. */

    unsigned int pad		:1;	/* Padding. */
    unsigned int bitStuff	:1;	/* Hdlc bit stuffing. */
    unsigned int crc16		:1;	/* CRC 16 bits or 32. */
    unsigned int noCrcInsert	:1;	/* No crc insertion. */
    unsigned int xmitOnNoCarr  :1;	/* Transmit even if no carrier sense. */
    unsigned int manch		:1;	/* Manchester or NRZ encoding. */
    unsigned int noBroadcast	:1;	/* Disable broadcasts. */
    unsigned int promisc	:1;	/* Promiscuous mode. */

    unsigned int collDetectSrc	:1;	/* Collision detect source. */
    unsigned int cdFilter	:3;	/* Collision detect filter bits. */
    unsigned int carrSenseSrc	:1;	/* Carrier sense source. */
    unsigned int carrSenseFilter:3;	

    unsigned int minFrameLength:8;	/* Minimum frame length. */

    unsigned int		:8;	/* Unused. */
} NetIEConfigureCB;

/*
 * The transmit command block.
 */

typedef struct {
    unsigned int xmitDeferred	:1;	/* Transmission deferred. */
    unsigned int heartBeat	:1;	/* Heart beat. */
    unsigned int tooManyCollisions:1;	/* Too many transmit collisions. */
    unsigned int 		:1;	/* Unused. */
    unsigned int numCollisions	:4;	/* The number of collisions 
					   experienced */
    unsigned int cmdDone	:1;	/* Command done. */
    unsigned int cmdBusy	:1;	/* Command busy. */
    unsigned int cmdOK		:1;	/* Command completed successfully. */
    unsigned int cmdAborted	:1;	/* The command aborted. */
    unsigned int 		:1;	/* Unused. */
    unsigned int noCarrSense	:1;	/* No carrier sense. */
    unsigned int noClearToSend	:1;	/* Transmission unsuccessful because
					   of loss of clear to send signal. */
    unsigned int underRun	:1; 	/* DMA underrun. */

    unsigned int		:5;	/* Unused. */
    unsigned int cmdNumber	:3;	/* Command number. */

    unsigned int endOfList	:1;	/* The end of the command list. */
    unsigned int suspend	:1;	/* Suspend when command completes. */
    unsigned int interrupt	:1;	/* Interrupt when the command 
    unsigned int 		:5;	/* Unused. */

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
    unsigned	int countLow	:8;	/* Low order 8 bits of count of bytes */
    unsigned	int eof	:1;	/* Last buffer in the packet. */
    unsigned	int		:1;	/* Unused. */
    unsigned	int countHigh	:6;	/* High order 6 bits of the count. */

    short	nextTBD;		/* Offset of the next transmit 
					   buffer descriptor. */
    int		bufAddr;		/* Address of buffer of data. */
} NetIETransmitBufDesc;

/*
 * The receive frame descriptor.
 */

typedef struct NetIERecvFrameDesc {
    unsigned int shortFrame	:1;	/* Was a short frame. */
    unsigned int noEOF		:1;	/* No EOF (bitstuffing mode only). */
    unsigned int 		:6;	/* Unused. */

    unsigned int done		:1;	/* Frame completely stored. */
    unsigned int busy		:1;	/* Busy storing frame. */
    unsigned int ok		:1;	/* Frame received OK */
    unsigned int 		:1;	/* Unused. */
    unsigned int crcError	:1;	/* Received packet had a crc error */
    unsigned int alignError	:1;	/* Received packet had an alignment 
					   error */
    unsigned int outOfBufs	:1;	/* Receive unit ran out of memory */
    unsigned int overrun	:1;	/* DMA overrun. */

    unsigned int 		:8;	/* Unused. */

    unsigned int endOfList	:1;	/* End of list. */
    unsigned int suspend	:1;	/* Suspend when done receiving. */
    unsigned int 		:6;	/* Unused. */

    short nextRFD;			/* Next receive frame descriptor. */
    short recvBufferDesc;		/* Offset of the first receive buffer
					   descriptor. */

    Net_EtherAddress destAddr;		/* Destination ethernet address. */
    Net_EtherAddress srcAddr;		/* Source ethernet address. */
    short 	 type;			/* Ethernet packet type. */
    volatile struct NetIERecvFrameDesc *realNextRFD; /* The SUN address of 
                                        the next receive frame descriptor. */
} NetIERecvFrameDesc;

/*
 * Receive buffer descriptor.
 */

typedef struct NetIERecvBufDesc {
    unsigned int countLow	:8;	/* Low order bits of the count of bytes
					   in this buffer. */
    unsigned int eof		:1;	/* Last buffer for this packet. */
    unsigned int countValid	:1;	/* The value in the count field is 
					   valid. */
    unsigned int countHigh	:6;	/* High order bits of the count. */

    short nextRBD;			/* Next receive buffer descriptor. */
    int	  bufAddr;			/* The address of the buffer that
					   this descriptor puts its data. */

    unsigned int bufSizeLow	:8;	/* Low order bits of amount of bytes
					   this buffer is capable of holding */
    unsigned int endOfList	:1;	/* This is the end of the RBD list. */
    unsigned int		:1;	/* Unused. */
    unsigned int bufSizeHigh	:6;	/* High order 6 bits of the buffer 
					   size. */
    Address	realBufAddr;		/* The SUN address of the buffer 
					   where this descriptor puts its data*/
    volatile struct NetIERecvBufDesc *realNextRBD; /* The 6800 Address of the
                                            next receive buffer descriptor. */
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
    unsigned int	memBase;	/* Address of control block memory. */
    volatile NetIESysConfPtr *sysConfPtr;
                                        /* Where the system configuration
					   pointer is at. */
    volatile NetIEIntSysConfPtr	*intSysConfPtr;
                                        /* Where the intermediate system
					   configuration pointer is at. */
    volatile NetIESCB	*scbPtr;	/* Pointer to system control block. */
    volatile NetIECommandBlock *cmdBlockPtr;/* Head of command block list */
    volatile NetIERecvFrameDesc	*recvFrDscHeadPtr; /* Head of receive frame
                                            descriptor list. */
    volatile NetIERecvFrameDesc	*recvFrDscTailPtr; /* Tail of receive frame
                                            descriptor list. */
    volatile NetIERecvBufDesc *recvBufDscHeadPtr; /* Head of receive buffer
                                            descriptor list. */
    volatile NetIERecvBufDesc *recvBufDscTailPtr; /* Tail of receive buffer
                                            descriptor list. */
    List_Links		*xmitList;	/* Pointer to the front of the list of
					   packets to be transmited. */
    List_Links      	*xmitFreeList;	/* Pointer to a list of unused 
					   transmission queue elements. */
    volatile NetIETransmitCB *xmitCBPtr; /* Pointer to the single command block
					   for transmitting packets. */
    Boolean		transmitting;	/* Set if are currently transmitting a
					   packet. */
    Boolean		running;	/* Is the chip currently active. */
    volatile NetIEControlRegister *controlReg;/* The onboard device register.*/
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
 * Buffers for output.
 */
extern	char	*netIEXmitFiller;
extern	char	*netIEXmitTempBuffer;

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

extern	int	NetIEAddrFromSUNAddr();
extern	int	NetIEAddrToSUNAddr();
extern	int	NetIEOffsetFromSUNAddr();
extern	int	NetIEOffsetToSUNAddr();
extern	int	NetIEShortSwap();

#endif /* _NETIEINT */
