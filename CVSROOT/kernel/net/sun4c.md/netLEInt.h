/*
 * netLEInt.h --
 *
 *	External definitions for the Am7990 (LANCE) ethernet controller.
 * The description of the definitions here come from AMD Am7990 LANCE
 * data sheet (Publication 05698 Rev B) and the Am7990 Techincal Manual.

 * Copyright 1988 Regents of the University of California
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

#ifndef _NETLEINT
#define _NETLEINT

#include <netInt.h>
#include <mach.h>
#include <netLEMachInt.h>

/*
 * Defined constants:
 *
 * NET_LE_NUM_RECV_BUFFERS  	The number of buffers that we have to receive
 *			   	packets in. Also, number of receive ring 
 *				descriptors. It must be between 1 and 128 and
 *				be a power of TWO.
 * NET_LE_NUM_RECV_BUFFERS_LOG2	log base 2 of NET_LE_NUM_RECV_BUFFERS.
 * NET_LE_NUM_XMIT_BUFFERS  	The number of buffer descriptors that are used
 *			   	for a transmitted packet.  Also, number of xmit
 *				ring descriptors. It must be between 1 and 
 *				128 and	be a power of TWO.
 * NET_LE_NUM_XMIT_BUFFERS_LOG2	log base 2 of NET_LE_NUM_XMIT_BUFFERS.
 * NET_LE_RECV_BUFFER_SIZE  	The size of each receive buffer. We make the
 *				buffer big enough to hold the maximum size
 *				ethernet packet + CRC check. This is 1514 +
 *				4 = 1518 bytes. We round it to the nears 1/2 K
 *				boundry to get 1536.
 *			
 * NET_LE_MIN_FIRST_BUFFER_SIZE	The smallest buffer that can be used for the
 *				first element of a chain transmission.
 *				If the first piece of a message is smaller than  *				this then it gets copied to other storage and
 *				made the minimum size.
 * NET_LE_NUM_XMIT_ELEMENTS 	The number of elements to preallocate for the 
 *			   	retransmission queue.
 */

#define	NET_LE_NUM_RECV_BUFFERS_LOG2	4
#define	NET_LE_NUM_RECV_BUFFERS		(1 << NET_LE_NUM_RECV_BUFFERS_LOG2)

#define	NET_LE_NUM_XMIT_BUFFERS_LOG2	5
#define	NET_LE_NUM_XMIT_BUFFERS		(1 << NET_LE_NUM_XMIT_BUFFERS_LOG2)


#define	NET_LE_RECV_BUFFER_SIZE		1536
#define NET_LE_MIN_FIRST_BUFFER_SIZE	100

#define	NET_LE_NUM_XMIT_ELEMENTS	32

/*
 * The LANCE chip has four control and status registers that are selected by a
 * register address port (RAP) register. The top 14 bits of RAP are reserved
 * and read as zeros. The register accessable in the register data port (RDP)
 * is selected by the value of the RAP. (page 15)
 *
 */

typedef NetLEMach_Reg NetLE_Reg;

#define AddrPort	0x0e02
#define DataCSR0	0x0010
#define DataCSR1	0x0010
#define DataCSR2	0x0808
#define DataCSR3	0x0d03

/*
 * Possible RAP values. (page15)
 */ 

#define	NET_LE_CSR0_ADDR	0x0
#define	NET_LE_CSR1_ADDR	0x1
#define	NET_LE_CSR2_ADDR	0x2
#define	NET_LE_CSR3_ADDR	0x3

/*
 * Control and status register defintions.
 *
 * CSR0 - Register 0.
 * Note that CSR0 is updated by ORing the previous and present value.
 * See (page 16-17) for description of these bits.
 * Bits of CSR0 fall into the following groups:
 * (R0) - Read only - writing does nothing.
 * (RC)	- Read and clear by writing a one - Writing zero does nothing.
 * (RW) - Read and Write.
 * (RW1) - Read and write with one only. 
 */

/*
 * Error bits
 * NET_LE_CSR0_ERROR = BABBLE|COLLISION_ERROR|MISSED_PACKET|MEMORY_ERROR
 */

#define	NET_LE_CSR0_ERROR		0x8000	/* Error summary - R0 */
#define	NET_LE_CSR0_BABBLE		0x4000	/* Transmitter babble - RC */
#define	NET_LE_CSR0_COLLISION_ERROR	0x2000	/* Late collision - RC */
#define	NET_LE_CSR0_MISSED_PACKET	0x1000	/* Miss a packet - RC */
#define	NET_LE_CSR0_MEMORY_ERROR	0x0800	/* Memory error - RC */
/*
 * Interrupt bits.
 * NET_LE_CSR_INTR = RECV_INTR|XMIT_INTR|INIT_DONE|ERROR
 */
#define	NET_LE_CSR0_RECV_INTR		0x0400 	/* Receiver interrupt - RC */
#define	NET_LE_CSR0_XMIT_INTR		0x0200	/* Trasmit interrupt - RC */
#define	NET_LE_CSR0_INIT_DONE		0x0100	/* Initialization done - RC */
#define	NET_LE_CSR0_INTR		0x0080	/* Interrupt Summary - R0 */
/*
 * Enable bits.
 */
#define	NET_LE_CSR0_INTR_ENABLE		0x0040	/* Interrupt enable - RW */
#define	NET_LE_CSR0_RECV_ON		0x0020	/* Receiver on - R0 */
#define	NET_LE_CSR0_XMIT_ON		0x0010	/* Transmitter on - R0 */
#define	NET_LE_CSR0_XMIT_DEMAND		0x0008	/* Sent now flag. - RW1 */
#define	NET_LE_CSR0_STOP		0x0004	/* Stop and reset - RW1 */
#define	NET_LE_CSR0_START		0x0002	/* (Re)Start after stop - RW1*/
#define	NET_LE_CSR0_INIT		0x0001	/* Initialize - RW1 */

/*
 * Control and status register 1 (CSR1) (page 18)
 * CSR1 is the low order 16 bits of the address of the Initialization block.
 * Note that the LSB must be zero.
 */

/*
 * Control and status register (CSR2) (page 18)
 * CSR2 is the high order 16 bits of address of the Initialization block.
 * Note that the top 8 bits are reserved and must be zero.
 */

/*
 * Control and status register (CSR3) (page 18)
 * CSR3 defines the Bus Master interface.
 */
#define	NET_LE_CSR3_BYTE_SWAP		0x0004	/* Byte swap for us. - RW */
#define	NET_LE_CSR3_ALE_CONTROL		0x0002	/* Signals active low - RW */
#define	NET_LE_CSR3_BYTE_CONTROL	0x0001	/* Byte control - RW */

/*
 * Define the value for csr3 for the different machine types.
 */
#if defined(sun4c)
#define NET_LE_CSR3_VALUE \
    (NET_LE_CSR3_BYTE_SWAP | NET_LE_CSR3_ALE_CONTROL | NET_LE_CSR3_BYTE_CONTROL)
#else 

#if (defined(ds5000) || defined(sun3))
#define NET_LE_CSR3_VALUE  0
#else
#define NET_LE_CSR3_VALUE NET_LE_CSR3_BYTE_SWAP
#endif

#endif

/*
 * First in the mode register.
 *
 *typedef struct NetLEModeReg {
 *    unsigned int	promiscuous	:1;	Read all incoing packets.  
 *    unsigned int			:8;	Reserved  
 *    unsigned int        internalLoop	:1;	Internal if lookBack.  
 *    unsigned int	disableRetry	:1;	Disable collision retry.  
 *    unsigned int	forceCollision	:1;	Force collision.	 
 *    unsigned int	disableCRC	:1;	Disable transmit CRC.  
 *    unsigned int	loopBack	:1;	Loop back mode.  
 *    unsigned int	disableXmit	:1;	Disable the transmitter.  
 *    unsigned int	disableRecv	:1;	Disable the receiver.  
 *} NetLEModeReg;
 */

typedef unsigned short NetLEModeReg[1];

#define Promiscuous               0x0001
#define InternalLoop              0x0901
#define DisableRetry              0x0a01
#define ForceCollision            0x0b01
#define DisableCRC                0x0c01
#define LoopBack                  0x0d01
#define DisableXmit               0x0e01
#define DisableRecv               0x0f01


/*
 *  Descriptor Ring Pointer (page 21) (Byte swapped. )
 *
 *typedef struct NetLERingPointer {
 *    unsigned short	ringAddrLow	:16;	  Low order ring address.
 *						 * Must be quad word aligned. 
 *						  
 *    unsigned int	logRingLength	:3;	  log2 of ring length.  
 *    unsigned int			:5;	  Reserved  
 *    unsigned int	ringAddrHigh	:8;	  High order ring address.  
 *} NetLERingPointer;
 */

typedef unsigned short NetLERingPointer[2];

#define RingAddrLow               0x0010
#define LogRingLength             0x1003
#define RingAddrHigh              0x1808

/*
 * LE Initialization block. (Page 19)
 */

typedef struct NetLEInitBlock {
    NetLEModeReg	mode;			/* Mode register */
    /*
     * It looks like the ethernet address needs to be byte swapped.
     * Also, lowest-order bit must be zero.
     */
    Net_EtherAddress	etherAddress;		/* The ethernet address. */
    unsigned short	multiCastFilter[4];	/* Logical address filter. */
    NetLERingPointer	recvRing;		/* Receive ring buffers. */
    NetLERingPointer	xmitRing;		/* Transmit ring buffers. */
} NetLEInitBlock;

/*
 * LE Net Recv messages descriptors
 * 
 * typedef struct NetLERecvMsgDesc {
 *     unsigned short	bufAddrLow;	  Low order 16 addr bits of buffer.  
 *     unsigned int	chipOwned	:1;	  Buffer is owned by LANCE.  
 *     unsigned int	error		:1;	  Error summary  
 *     unsigned int	framingError	:1;	  Framing Error occured.   
 *     unsigned int	overflowError	:1;	  Packet overflowed.  
 *     unsigned int	crcError	:1;	  CRC error.  
 *     unsigned int	bufferError	:1;	  Buffer error.  
 *     unsigned int	startOfPacket	:1;	  First buffer of packet.  
 *     unsigned int	endOfPacket	:1;	  Last buffer of packet.  
 *     unsigned char	bufAddrHigh;	  High order 8 addr bits of buffer.  
 *     short	        bufferSize;	  Size of buffer in bytes. This 
 * 					 * has to be the 2's complement of
 * 					 * the buffer size.
 * 					  
 *     short	packetSize;		  Size of the packet (bytes).  
 * } NetLERecvMsgDesc;
 */

typedef struct NetLERecvMsgDesc {
    unsigned short	bufAddrLow;	/* Low order 16 addr bits of buffer. */
#ifdef ds5000
    unsigned char	bufAddrHigh;	/* High order 8 addr bits of buffer. */
    unsigned char	bits[1];	/* Control bits. */
#else
    unsigned char	bits[1];	/* Control bits. */
    unsigned char	bufAddrHigh;	/* High order 8 addr bits of buffer. */
#endif
    short	        bufferSize;	/* Size of buffer in bytes. This 
					 * has to be the 2's complement of
					 * the buffer size.
					 */
    short	packetSize;		/* Size of the packet (bytes). */
} NetLERecvMsgDesc;

#define ChipOwned                 0x0001
#define Error                     0x0101
#define FramingError              0x0201
#define OverflowError             0x0301
#define CrcError                  0x0401
#define RecvBufferError           0x0501
#define StartOfPacket             0x0601
#define EndOfPacket               0x0701

/*
 * LE Net Xmit messages descriptors (page 23-23)
 * 
 * typedef struct NetLEXmitMsgDesc {
 *     unsigned short  bufAddrLow;	  Low order 16 addr bits of buffer. 
 *     unsigned int    chipOwned	    :1;	  Buffer owned by the LANCE  
 *     unsigned int    error	    :1;	  Error summary  
 *     unsigned int		    :1;	  Reserved.   
 *     unsigned int    retries	    :1;	  More than one retry was needed.   
 *     unsigned int    oneRetry	    :1;	  Exactly one retry was needed.  
 *     unsigned int    deferred	    :1;	  Transmission deferred.  
 *     unsigned int    startOfPacket   :1;	  First buffer of packet.  
 *     unsigned int    endOfPacket     :1;	  Last buffer of packet.  
 *     unsigned char   bufAddrHigh;	  High order 8 addr bits of buffer.  
 *     short           bufferSize;	  Signed size of buffer in bytes. This 
 * 					 * has to be the 2's complement of
 * 					 * the buffer size.
 * 					 * Note that the first buffer in a
 * 					 * chain must have at least 100 bytes.
 * 					  
 *     unsigned int    bufferError	    :1;	  Buffering error.  
 *     unsigned int    underflowError  :1;	  Underflow error.  
 *     unsigned int		    :1;	  Reserved.  
 *     unsigned int    lateCollision   :1;	  Late collision error.  
 *     unsigned int    lostCarrier	    :1;	  Loss of carrier error.  
 *     unsigned int    retryError	    :1;	  Too many collision.  
 *     unsigned int    tdrCounter   :10;  Time Domain Reflectometry counter.  
 * } NetLEXmitMsgDesc;
 */
typedef struct NetLEXmitMsgDesc {
    unsigned short  bufAddrLow;	/* Low order 16 addr bits of buffer.*/
#ifdef ds5000
    unsigned char   bufAddrHigh;	/* High order 8 addr bits of buffer. */
    unsigned char   bits1[1];	/* Control bits. See below. */
#else
    unsigned char   bits1[1];	/* Control bits. See below. */
    unsigned char   bufAddrHigh;	/* High order 8 addr bits of buffer. */
#endif
    short           bufferSize;		/* Signed size of buffer in bytes. This 
					 * has to be the 2's complement of
					 * the buffer size.
					 * Note that the first buffer in a
					 * chain must have at least 100 bytes.
					 */
    unsigned short   bits2[1];	/* Control bits. See below. */
} NetLEXmitMsgDesc;

#define ChipOwned                 0x0001
#define Error                     0x0101
#define Retries                   0x0301
#define OneRetry                  0x0401
#define Deferred                  0x0501
#define StartOfPacket             0x0601
#define EndOfPacket               0x0701

#define XmitBufferError           0x0001
#define UnderflowError            0x0101
#define LateCollision             0x0301
#define LostCarrier               0x0401
#define RetryError                0x0501
#define TdrCounter                0x060a



/*
 * Structure to hold all state information associated with one of these
 * chips.
 */

typedef struct {
    volatile NetLE_Reg	    *regPortPtr;    /* Port to chip's registers. */
    volatile NetLEInitBlock *initBlockPtr;  /* Chip initialization block. */
    /*
     * Pointers for ring of receive buffers. 
     */
    volatile NetLERecvMsgDesc	*recvDescFirstPtr;
                                            /* Ring of receive desc start.*/
    volatile NetLERecvMsgDesc	*recvDescNextPtr;
                                            /* Next recv desc to be filled. */
    volatile NetLERecvMsgDesc	*recvDescLastPtr;
                                            /* Ring of recv descriptors end. */
    /*
     * Pointers for ring of transmit buffers. 
     */
    volatile NetLEXmitMsgDesc	*xmitDescFirstPtr;
                                            /* Ring of xmit descriptors start.*/
    volatile NetLEXmitMsgDesc	*xmitDescNextPtr;
                                            /* Next xmit desc to be filled. */
    volatile NetLEXmitMsgDesc	*xmitDescLastPtr;
                                            /* Ring of xmit descriptors end. */

    List_Links		xmitListHdr;	/* List of packets to be transmited. */
    List_Links		*xmitList;	/* Pointer to the front of the list of
					   packets to be transmited. */
    List_Links		xmitFreeListHdr; /* List of unused packets. */
    List_Links      	*xmitFreeList;	/* Pointer to a list of unused 
					   transmission queue elements. */
    Boolean		transmitting;	/* Set if are currently transmitting a
					   packet. */
    Boolean		running;	/* Is the chip currently active. */
    Net_EtherAddress	etherAddressBackward;	/* The ethernet address in 
						 * reverse byte order. */
    Net_EtherAddress	etherAddress;	/* The ethernet address */
    Address		recvDataBuffer[NET_LE_NUM_RECV_BUFFERS]; /* Receive
							* data buffers. */
    Boolean		recvMemInitialized;	/* Flag for initializing
						 * kernel memory. */
    Boolean		recvMemAllocated;	/* Flag for allocating
						 * memory for ring buffers. */
    Net_ScatterGather 	*curScatGathPtr;  /* Pointer to scatter gather element 
					   * for current packet being sent. */
    char            	loopBackBuffer[NET_ETHER_MAX_BYTES]; /* Buffer for the
						  * loopback address. */
    char		*firstDataBuffer; /* Buffer used to ensure that
					   * first element is of a minimum
					   * size. */
    Boolean		xmitMemInitialized; /* Flag to note if xmit memory
					     * has been initialized. */
    Boolean		xmitMemAllocated; /* Flag to note if xmit memory
					   * has been allocated. */
    Net_Interface	*interPtr;	/* Pointer back to network interface. */
    Net_EtherStats	stats;		/* Performance statistics. */
    int			numResets;	/* Number of times the chip has
					 * been reset. */
    Boolean		resetPending;	/* TRUE => chip should be reset when
					 * current transmit is done. */
    int			lastRecvCnt;	/* Number of packets done during
					 * last receive interrupt. */
#ifdef ds5000
    char		*bufAddr;	/* Network buffer. */
    char		*bufAllocPtr;	/* Current allocation address in
					 * network buffer. */
    int			bufSize;	/* Size of the network buffer. */
#endif
} NetLEState;


/*
 * General routines.
 */

extern	ReturnStatus	NetLEInit _ARGS_((Net_Interface *interPtr));
extern	ReturnStatus	NetLEOutput _ARGS_((Net_Interface *interPtr,
			    Address hdrPtr,Net_ScatterGather *scatterGatherPtr,
			    int scatterGatherLength, Boolean rpc,
			    ReturnStatus *statusPtr));
extern	void		NetLEIntr _ARGS_((Net_Interface *interPtr, 
			    Boolean polling));
extern	void		NetLERestart _ARGS_((Net_Interface *interPtr));
extern	void		NetLEReset _ARGS_((Net_Interface *interPtr));
extern	ReturnStatus	NetLEGetStats _ARGS_((Net_Interface *interPtr, 
			    Net_Stats *statPtr));
extern Address		NetLEMemAlloc _ARGS_((NetLEState *statePtr, 
			    int numBytes));

extern ReturnStatus	NetLEMachInit _ARGS_((Net_Interface *interPtr,
			    NetLEState *statePtr));
/*
 * Routines for transmitting.
 */

extern	void		NetLEXmitInit _ARGS_((NetLEState *statePtr));
extern	ReturnStatus	NetLEXmitDone _ARGS_((NetLEState *statePtr));
extern	void		NetLEXmitRestart _ARGS_((NetLEState *statePtr));
extern	void		NetLEXmitDrop _ARGS_((NetLEState *statePtr));

/*
 * Routines for the receive unit.
 */

extern	void		NetLERecvInit _ARGS_((NetLEState *statePtr));
extern	ReturnStatus	NetLERecvProcess _ARGS_((Boolean dropPackets,
			    NetLEState *statePtr));


extern	NetLEState	netLEDebugState;
extern	NetLERecvMsgDesc netLEDebugRecv[];
extern	NetLEXmitMsgDesc netLEDebugXmit[];
extern	int		netLEDebugCount;
extern	unsigned short	netLEDebugCsr0;
extern	char		netLEDebugBuffer[];

#endif /* _NETLEINT */
