/*
 * netIEInt.h --
 *
 *	External definitions for the Intel Ethernet controller.  See
 *      the Intel "LAN Components User's Manual" from 1984 for a 
 *	description of the definitions here.  The SPUR machine uses
 *	a TI implemented board that is documented in Texas Instruments'
 *	"Explorer NuBus Ethernet Controller General Desciption" part
 *	number (2243161-0001).
 *
 *	Much of this code and header file was taken from the Sprite
 *	SUN ie device driver code.
 *
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

#define	NET_ETHER_BAD_ALIGNMENT

#include "netEther.h"
#include "net.h"
#include "netInt.h"
#include "sync.h"
/*
 * Defined constants:
 *
 * NET_IE_MEM_SIZE	      	Amount of buffer memory on the board. 
 *	
 * NET_IE_FREE_MEM_SIZE		Amount of buffer memory available for use.
 *				This is 10 bytes less than the amount of buffer
 *				memory memory due to the SCP located at offset
 *				0x7ff6.
 * NET_IE_NUM_RECV_BUFFERS  	The number of buffers that we have to receive
 *			   	packets in.
 * NET_IE_NUM_XMIT_BUFFERS  	The number of buffer descriptors that are used
 *			   	for a transmitted packet.
 * NET_IE_RECV_BUFFER_SIZE  	The size of each receive buffer.
 * NET_IE_XMIT_BUFFER_SIZE  	The size of each transmit buffer.
 * NET_IE_MIN_DMA_SIZE		The smallest buffer that can be use for DMA.
 *				If a piece of a message is smaller than this
 *				then it gets copied to other storage and
 *				made the minimum size.
 * NET_IE_MAX_CMD_BLOCK_SIZE	Maximum size of IE command block.
 * NET_IE_NULL_RECV_BUFF_DESC 	The value that is used by the controller to 
 *				indicate that a header points to no data.
 * NET_IE_NUM_XMIT_ELEMENTS 	The number of elements to preallocate for the 
 *			   	retransmission queue.
 * NET_IE_SLOT_ID		Default Nubus slot of controller.
 * NET_IE_SLOT_SPACE_SIZE	The size of the slot space for this device.
 */


#define	NET_IE_MEM_SIZE			(32*1024)
#define NET_IE_FREE_MEM_SIZE		(NET_IE_MEM_SIZE-10)
#define	NET_IE_NUM_RECV_BUFFERS		18
#define	NET_IE_NUM_XMIT_BUFFERS		1
#define	NET_IE_RECV_BUFFER_SIZE		NET_ETHER_MAX_BYTES
#define	NET_IE_XMIT_BUFFER_SIZE		NET_ETHER_MAX_BYTES
#define	NET_IE_MIN_DMA_SIZE		12
#define	NET_IE_MAX_CMD_BLOCK_SIZE	64
#define	NET_IE_NULL_RECV_BUFF_DESC	0xffff
#define	NET_IE_NUM_XMIT_ELEMENTS	32
#define	NET_IE_SLOT_ID			0xe
#define	NET_IE_SLOT_SPACE_SIZE		0x1000000

/*
 * Buffer sizes:
 *
 * sizeof(NetIEIntSysConfPtr) 					8
 * sizeof(NetIESCB)					       16
 * sizeof(NetIECommandBlock)					8
 * NET_IE_NUM_RECV_BUFFERS * NET_IE_RECV_BUFFER_SIZE		18 * 1516
 * NET_IE_NUM_RECV_BUFFERS * sizeof(NetIERecvBufDesc)		18 * 20
 * NET_IE_NUM_RECV_BUFFERS-1 * sizeof(NetIERecvFrameDesc)	17 * 28
 * sizeof(NetIETransmitBufDesc)					8
 * NET_IE_XMIT_BUFFER_SIZE					1516
 *								---
 *								29680
 */

/*
 * Macros to manipulate the chip.
 *
 * NET_IE_CHIP_RESET		Reset the chip.
 * NET_IE_CHANNEL_ATTENTION	Get the attention of the chip.
 * NET_IE_DELAY			Delay until the microsecond limit is up or
 *				until the condition is true.
 * NET_IE_CHECK_SCB_CMD_ACCEPT	Check to see if the command has been accepted.
 * NET_IE_SLOT_OFFSET		Change a slot offset in to a SPUR address.
 */

#define NET_IE_CHIP_RESET  {*netIEState.configAndFlagsReg = NET_IE_CONFIG_RESET;}

#define NET_IE_CHANNEL_ATTENTION \
	{ \
	    (*netIEState.channelAttnReg) = 1; \
	}

#ifdef DOWNLOADER
#define NET_IE_DELAY(condition) \
	{ \
	    register int i = (400000); \
	    while (i > 0 && !(condition)) { \
		    i--; \
		    asm("cmp_trap always,r0,r0,$3"); \
	    } \
	}
#else
#define NET_IE_DELAY(condition) \
	{ \
	    register int i = (400000); \
	    while (i > 0 && !(condition)) { \
		    i--; \
	    } \
	}
#endif
#define	NET_IE_CHECK_SCB_CMD_ACCEPT(scbPtr) \
    if (*(((short *) scbPtr)+1) != 0) { \
	NetIECheckSCBCmdAccept(scbPtr); \
    }
#define	NET_IE_SLOT_OFFSET(offset)	(netIEState.deviceBase + (offset))
/*
 * System configuration pointer.  Must be at 0xfs007ff4 in NuBus address space.
 * where s is the slot number of the controller board.
 */

typedef struct {
    int	padding			:16;	/* Padding for alignment. */
    int	busWidth		:1;	/* Bus width.  0 => 16 , 1 => 8 bits. */
    int				:15;	/* not defined */
    int				:16;	/* not defined */
    int				:16;	/* not defined */
    unsigned int intSysConfPtr	:32;	/* Address of intermediate system
					 * configuration pointer. 
					 * Bits a32-a16 of the intSysConfPtr
					 * not used on this implementation.
					 */
} NetIESysConfPtr;

/*
 * Intermediate system configuration pointer.  This specifies the base of the 
 * control blocks and the offset of the System Control Block (SCB).
 */

typedef struct {
    char  	busy		:8;	/* 1 if initialization in progress. */
    char  	filler		:8;	/* Unused. */
    unsigned short scbOffset	:16;	/* Offset of the scb. */
    unsigned int base		:32;	/* Base of all control blocks. 
					 * bits a32-a16 of the base not
					 * wired on this implementation.
					 */
} NetIEIntSysConfPtr;


/*
 * Define the macros to Check and acknowledge the status of the chip.
 *
 * NET_IE_CHECK_STATUS	Extract the 4 status bits out of the scb.
 * NET_IE_ACK		Set the bits in the command word of the scb that ack 
 *			the status bits.
 * NET_IE_TRANSMITTED	Return true if a transit command finished.
 * NET_IE_RECEIVED	Return true if a packet was received.
 */

#define	NET_IE_CHECK_STATUS(scbPtr) ((*(short *) (scbPtr)) & 0xF000)
#define	NET_IE_ACK(scbPtr, status) ((*(((short *) (scbPtr))+1)) |= status)
#define	NET_IE_TRANSMITTED(status) ((status) & 0xA000)
#define	NET_IE_RECEIVED(status) ((status) & 0x5000)

/*
 * The system control block.
 */

typedef struct {

    /*
     * The system control block status.
     */
    unsigned char 	            :4;	/* Must be zero */
    unsigned char recvUnitStatus    :3;	/* Receive unit status */
    unsigned char 		    :1;	/* Must be zero */
    unsigned char cmdUnitStatus	    :3; /* Command unit status. */
    unsigned char                   :1;	/* Must be zero. */
    unsigned char recvUnitNotReady  :1;	/* The command unit has left the ready
					   state. */
    unsigned char cmdUnitNotActive  :1;	/* The command unit has left the active
					   state. */
    unsigned char frameRecvd	    :1;	/* A frame received interrupt has been
					   given. */
    unsigned char cmdDone	    :1;	/* A command which has its interrupt
					   bit set completed. */

    /* 
     * The system control block command word.
     */

    unsigned char 		     :4;  /* Unused. */
    unsigned char recvUnitCmd        :3;  /* The command for the receive unit */
    unsigned char reset		     :1;  /* Reset the chip. */

    unsigned char cmdUnitCmd         :3;  /* The command for the command unit */
    unsigned char		     :1;  /* Unused. */
    unsigned char ackRecvUnitNotReady:1;  /* Ack that the receive unit became
					     not ready */
    unsigned char ackCmdUnitNotActive:1;  /* Ack that the command unit became
					     not active. */
    unsigned char ackFrameRecvd      :1;  /* Ack the frame received bit in the
					     the status word. */
    unsigned char ackCmdDone         :1;  /* Ack the command completed bit in
					     the status word. */
    unsigned short	cmdListOffset;
    unsigned short      recvFrameAreaOffset;
    unsigned short      crcErrors;		/* Count of crc errors. */
    unsigned short 	alignErrors;		/* Count of alignment errors. */
    unsigned short	resourceErrors;		/* Count of correct incoming 
						   packets discarded because
						   of lack of buffer space */
    unsigned short	overrunErrors;		/* Count of overrun packets */
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
    unsigned int 		:12;	/* Bits of status. */
    unsigned char cmdAborted	:1;	/* The command aborted. */
    unsigned char cmdOK		:1;	/* Command completed successfully. */
    unsigned char cmdBusy	:1;	/* Command busy. */
    unsigned char cmdDone	:1;	/* Command done. */
    unsigned char cmdNumber	:3;	/* Command number. */
    unsigned int		:10;	/* Unused. */
    unsigned char interrupt	:1;	/* Interrupt when the command 
					   completes. */
    unsigned char suspend	:1;	/* Suspend when command completes. */
    unsigned char endOfList	:1;	/* The end of the command list. */
    unsigned short nextCmdBlock;	/* The offset of the next command 
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
 * The individual address setup command block.
 */

typedef	struct {
    unsigned int 		:12;	/* Bits of status. */
    unsigned char cmdAborted	:1;	/* The command aborted. */
    unsigned char cmdOK		:1;	/* Command completed successfully. */
    unsigned char cmdBusy	:1;	/* Command busy. */
    unsigned char cmdDone	:1;	/* Command done. */
    unsigned char cmdNumber	:3;	/* Command number. */
    unsigned int		:10;	/* Unused. */
    unsigned char interrupt	:1;	/* Interrupt when the command 
					   completes. */
    unsigned char suspend	:1;	/* Suspend when command completes. */
    unsigned char endOfList	:1;	/* The end of the command list. */
    unsigned short nextCmdBlock;	/* The offset of the next command 
					   block. */
    /* NetIECommandBlock	cmdBlock; */	/* The command block. */
    Net_EtherAddress	etherAddress;	/* The ethernet address. */ 
} NetIEIASetupCB;

/*
 * The configure command block.
 */

typedef struct {
    unsigned int 		:12;	/* Bits of status. */
    unsigned char cmdAborted	:1;	/* The command aborted. */
    unsigned char cmdOK		:1;	/* Command completed successfully. */
    unsigned char cmdBusy	:1;	/* Command busy. */
    unsigned char cmdDone	:1;	/* Command done. */
    unsigned char cmdNumber	:3;	/* Command number. */
    unsigned int		:10;	/* Unused. */
    unsigned char interrupt	:1;	/* Interrupt when the command 
					   completes. */
    unsigned char suspend	:1;	/* Suspend when command completes. */
    unsigned char endOfList	:1;	/* The end of the command list. */
    unsigned short nextCmdBlock;	/* The offset of the next command 
					   block. */
    /* NetIECommandBlock	cmdBlock; */	/* The command block. */

    unsigned char byteCount	:4;	/* Number of configuration bytes. */
    unsigned char		:4;	/* Unused. */

    unsigned char fifoLimit	:4;	/* The fifo limit. */
    unsigned char          	:4;	/* Number of configuration bytes. */

    unsigned char 		:6;	/* Unused. */
    unsigned char srdyArdy	:1;	/* srdy/ardy. */
    unsigned char saveBadFrames :1;	/* Save bad frames. */
    unsigned char addrLen	:3;	/* The number of address bytes. */
    unsigned char atLoc 	:1;	/* Address and type fields are part */
    unsigned char preamble	:2;	/* Preamble length code. */
    unsigned char intLoopback	:1;	/* Internal loop back. */
    unsigned char excLoopback	:1;	/* External loop back. */

    unsigned char linPrio	:3;	/* Linear priority. */
    unsigned char 		:1;	/* Unused. */
    unsigned char expPrio	:3;	/* Exponential priority. */
    unsigned char backOff	:1;	/* Backoff method. */
    unsigned char interFrameSpace:8;	/* Interframe spacing. */

    unsigned int slotTime	:11;	/*  slot time. */
    unsigned char 		:1;	/* Unused. */	
    unsigned char numRetries	:4;	/* Number of transmit retries. */

    unsigned char promisc	:1;	/* Promiscuous mode. */
    unsigned char noBroadcast	:1;	/* Disable broadcasts. */
    unsigned char manch		:1;	/* Manchester or NRZ encoding. */
    unsigned char xmitOnNoCarr  :1;	/* Transmit even if no carrier sense. */
    unsigned char noCrcInsert	:1;	/* No crc insertion. */
    unsigned char crc16		:1;	/* CRC 16 bits or 32. */
    unsigned char bitStuff	:1;	/* Hdlc bit stuffing. */
    unsigned char pad		:1;	/* Padding. */

    unsigned char carrSenseFilter:3;	
    unsigned char carrSenseSrc	:1;	/* Carrier sense source. */
    unsigned char cdFilter	:3;	/* Collision detect filter bits. */
    unsigned char collDetectSrc	:1;	/* Collision detect source. */

    unsigned char minFrameLength:8;	/* Minimum frame length. */

    unsigned char		:8;	/* Unused. */
} NetIEConfigureCB;

/*
 * The transmit command block.
 */

typedef struct {
    unsigned char numCollisions	:4;	/* The number of collisions 
					   experienced */
    unsigned char 		:1;	/* Unused. */
    unsigned char tooManyCollisions:1;	/* Too many transmit collisions. */
    unsigned char heartBeat	:1;	/* Heart beat. */
    unsigned char xmitDeferred	:1;	/* Transmission deferred. */

    unsigned char underRun	:1; 	/* DMA underrun. */
    unsigned char noClearToSend	:1;	/* Transmission unsuccessful because
					   of loss of clear to send signal. */
    unsigned char noCarrSense	:1;	/* No carrier sense. */
    unsigned char 		:1;	/* Unused. */
    unsigned char cmdAborted	:1;	/* The command aborted. */
    unsigned char cmdOK		:1;	/* Command completed successfully. */
    unsigned char cmdBusy	:1;	/* Command busy. */
    unsigned char cmdDone	:1;	/* Command done. */

    unsigned char cmdNumber	:3;	/* Command number. */
    unsigned int		:10;	/* Unused. */
    unsigned char interrupt	:1;	/* Interrupt when the command */
    unsigned char suspend	:1;	/* Suspend when command completes. */
    unsigned char endOfList	:1;	/* The end of the command list. */

    unsigned short nextCmdBlock;	/* The offset of the next command 
					   block */
    unsigned short bufDescOffset;	/* The offset of the buffer descriptor*/
    Net_EtherAddress  destEtherAddr; 	/* The ethernet address of the 
				         * destination machine. Would
					 * like to use this but struct
					 * messes up alignment of 
					 * fields. */
    unsigned short	  type;		/* Ethernet packet type field. */
} NetIETransmitCB;

/* 
 * The transmit buffer descriptor.
 */

typedef struct {
    unsigned	int count	:14;	/* Count of bytes */
    unsigned	char		:1;	/* Unused. */
    unsigned	char eof	:1;	/* Last buffer in the packet. */

    unsigned short nextTBD	:16;	/* Offset of the next transmit 
					   buffer descriptor. */
    unsigned int  bufAddr	:32;	/* Address of buffer of data..
					 * Bits a23-a16 not wired on this
					 * and bits a31-a24 not used.. */

} NetIETransmitBufDesc;

/*
 * The receive frame descriptor.
 */

typedef struct NetIERecvFrameDesc {
    unsigned char 		:6;	/* Unused. */
    unsigned char noEOF		:1;	/* No EOF (bitstuffing mode only). */
    unsigned char shortFrame	:1;	/* Was a short frame. */

    unsigned char overrun	:1;	/* DMA overrun. */
    unsigned char outOfBufs	:1;	/* Receive unit ran out of memory */
    unsigned char alignError	:1;	/* Received packet had an alignment 
					   error */
    unsigned char crcError	:1;	/* Received packet had a crc error */
    unsigned char 		:1;	/* Unused. */
    unsigned char ok		:1;	/* Frame received OK */
    unsigned char busy		:1;	/* Busy storing frame. */
    unsigned char done		:1;	/* Frame completely stored. */

    unsigned int 		:14;	/* Unused. */

    unsigned char suspend	:1;	/* Suspend when done receiving. */
    unsigned char endOfList	:1;	/* End of list. */

    unsigned short nextRFD	:16;	/* Next receive frame descriptor. */
    unsigned short recvBufferDesc:16;	/* Offset of the first receive buffer
					   descriptor. */
    Net_EtherHdr	etherHdr;	/* Ethernet header of packet. */
    struct NetIERecvFrameDesc *realNextRFD; /* The address of the next
					     *  receive frame descriptor. */
} NetIERecvFrameDesc;

/*
 * Receive buffer descriptor.
 */

typedef struct NetIERecvBufDesc {
    unsigned int  count 	:14;	/* Low order bits of the count of bytes
					   in this buffer. */
    unsigned char countValid	:1;	/* The value in the count field is 
					   valid. */
    unsigned char eof		:1;	/* Last buffer for this packet. */

    unsigned short  nextRBD	:16;	/* Next receive buffer descriptor. */
    unsigned int  bufAddr	:32;	/* The address of the buffer that
					 * this descriptor puts its data. 
					 * bits a23-a16 of the bufAddr not
					 * wired on this implementation.
					 */

    unsigned int bufSize	:14;	/* Amount of bytes
					   this buffer is capable of holding */
    unsigned char		:1;	/* Unused. */
    unsigned char endOfList	:1;	/* This is the end of the RBD list. */
    Address	realBufAddr;		/* The address of the buffer 
					   where this descriptor puts its data*/
    struct NetIERecvBufDesc *realNextRBD; /* The Address of the next
					     receive buffer descriptor. */
} NetIERecvBufDesc;

/*
 * The TI ethernet board configuration and flags register.
 */

typedef unsigned int NetTIConfigAndFlagsReg;

/*
 * Definition of bits of config and flag register.
 */

/*
 * Config bits.
 */

#define	NET_IE_CONFIG_RESET		0x1	/* Reset board. */
#define	NET_IE_CONFIG_INTR_ENABLE	0x2	/* Enable interrupts. */
#define	NET_IE_CONFIG_LED_ON		0x4	/* Set LED on. */
#define	NET_LE_CONFIG_LOOPBACK		0x100	/* Set lookback mode. */

/*
 * Flag bits.
 */
#define	NET_IE_FLAG_BUS_ERROR		0x100 	/* NuBus error occurred while 
						 * posting event. */
#define	NET_IE_FLAG_HOLD_LED_ON		0x200	/* Memory handshake LED on */
#define	NET_IE_FLAG_RTS_LED_ON		0x400	/* Request to send  LED on */
#define	NET_IE_FLAG_CRS_LED_ON		0x800	/* Carrier sensed LED on */
#define	NET_IE_FLAG_CDT_LED_ON		0x1000	/* Collision dectected LED on*/
#define	NET_IE_FLAG_32K_MEMORY		0x2000	/* Memory size = 32k. */


/*
 * Structure to hold all state information associated with one of these
 * chips.
 */

typedef struct {
    Address		deviceBase;	/* Starting virtual address of 
					 * controller.	 */
    Address		memBase;	/* Address of buffer memory. */
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
    Boolean		mapped;		/* Is the device mapped. */
    int			*channelAttnReg;/* Channel Attention register */
    NetTIConfigAndFlagsReg 
		*configAndFlagsReg; 	/* Board's config  and flag register.*/
    Net_EtherAddress	etherAddress;	/* The ethernet address.  */

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
 * Pointer to scatter gather element for current packet being sent.
 */
extern Net_ScatterGather *curScatGathPtr;

/*
 * Semaphore protecting chip and driver.
 */
extern Sync_Semaphore	netIEMutex;

/*
 * NuBus ethernet controller board configuration ROM. All location are
 * offsets from the start of the device's slot. 
 * Note that the ROM data is stored one byte per word.
 */

/*
 * IEROM Layout version used for coding.
 */

#define	IEROM_LAYOUT_NUMBER	2

/* 
 * board serial number - 9 bytes.
 */
#define	IEROM_SERIAL_NUMBER		0xffffdc
/* 
 * Revision level -  6 byte ascii format.
 */
#define	IEROM_REVSION_LEVEL	0xffffc0
/*
 * CRC signature of IEROM - 2 bytes binary data.
 */
#define	IEROM_CRC_SIGNATURE	0xffffb8
/*
 * IEROM size - 1 byte - log2 of IEROM size in bytes.
 */
#define	IEROM_SIZE		0xffffb4
/*
 * Vendor ID - 4 bytes ascii.
 */
#define	IEROM_VENDOR_ID		0xffffa4
/*
 * Board Type - 8 bytes ascii.
 */
#define	IEROM_BOARD_TYPE	0xffff84
/*
 * part number - 16 bytes ascii.
 */
#define	IEROM_PART_NUMBER	0xffff44
/*
 * Configuration register offset - 3 bytes.
 */
#define	IEROM_CONFIG_REG_ADDR	0xffff38
/*
 * Device driver offset - 3 bytes..
 */
#define	IEROM_DEVICE_DRIVER_ADDR	0xffff2c
/*
 * diagnostic offset  - 3 bytes.
 */
#define	IEROM_DIAG_ADDR		0xffff20
/*
 * flag register offset - 3 bytes ( value better be IEROM_CONFIG_REG_ADDR+2) )
 */
#define	IEROM_FLAGS_REG_ADDR	0xffff14
/*
 * IEROM flags - 1 byte
 */
#define	IEROM_FLAGS		0xffff10
/*
 * IEROM layout byte 1 byte ( should be 2 )
 */
#define	IEROM_LAYOUT		0xffff0c
/*
 * IEROM test time - 1 byte
 */
#define	IEROM_TEST_TIME		0xffff08
/*
 * Resource type.
 */
#define	IEROM_RESOURCE_TYPE	0xffff00
/*
 * Board's ethernet address.
 */
#define	IEROM_ETHERNET_ADDRESS	0xfffee0

/*
 * Offset of registers that aren't stored in the ROM.
 */
#define	CHANNEL_ATTN_REG_OFFSET	0x8000
#define	EVENT_ADDR_REG_OFFSET	0xA000
#define SYS_CONF_PTR_OFFSET	0x7ff4

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
extern	void	NetIEMemMap();

/*
 * Routines to convert to addresses and offsets.
 */

extern	unsigned short	NetIEAddrFromSPURAddr();
extern	Address		NetIEAddrToSPURAddr();
/*
 * In this driver, the offset base register is set to zero so offsets
 * and addresses are the same.
 */

#define	NetIEOffsetFromSPURAddr(a)	NetIEAddrFromSPURAddr(a)	
#define	NetIEOffsetToSPURAddr(a)	NetIEAddrToSPURAddr(a)	

/*
 * SPUR doesn't need to swap bytes.
 */

#define	NetIEShortSwap(s)	(s)

#endif /* _NETIEINT */

