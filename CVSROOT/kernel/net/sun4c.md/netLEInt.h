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

#include "netEther.h"
#include "net.h"
#include "netInt.h"

/*
 * Defined constants:
 *
 * NET_LE_CONTROL_REG_ADDR	The address of the control register for the 
 *				ethernet chip.
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
#ifdef sun4
#define NET_LE_CONTROL_REG_ADDR		0xffd10000
#else
#define NET_LE_CONTROL_REG_ADDR		0xfe10000
#endif

#define	NET_LE_NUM_RECV_BUFFERS_LOG2	4
#define	NET_LE_NUM_RECV_BUFFERS		(1 << NET_LE_NUM_RECV_BUFFERS_LOG2)

#define	NET_LE_NUM_XMIT_BUFFERS_LOG2	5
#define	NET_LE_NUM_XMIT_BUFFERS		(1 << NET_LE_NUM_XMIT_BUFFERS_LOG2)


#define	NET_LE_RECV_BUFFER_SIZE		1536
#define NET_LE_MIN_FIRST_BUFFER_SIZE	100

#define	NET_LE_NUM_XMIT_ELEMENTS	32

/*
 * Macros for converting chip to cpu and cpu to chip address.
 * We always deal with chip addresses in two parts, the lower 16 bits
 * and the upper 8 bits.
 */
#ifdef sun4
#define	NET_LE_SUN_FROM_CHIP_ADDR(high,low)	\
		((Address) (0xff000000 + ((high) << 16) + (low)))
#else
#define	NET_LE_SUN_FROM_CHIP_ADDR(high,low)	\
		((Address) (0xf000000 + ((high) << 16) + (low)))
#endif
#define	NET_LE_SUN_TO_CHIP_ADDR_HIGH(a) ( (((unsigned int) (a)) >> 16) & 0xff)
#define	NET_LE_SUN_TO_CHIP_ADDR_LOW(a) ( ((unsigned int) (a)) & 0xffff)
/*
 * The LANCE chip has four control and status registers that are selected by a
 * register address port (RAP) register. The top 14 bits of RAP are reserved
 * and read as zeros. The register accessable in the register data port (RDP)
 * is selected by the value of the RAP. (page 15)
 *
 * The sun4 compiler generates byte loads and stores for short bit fields but
 * the hardware doesn't support byte access to the the LE registers.  Making
 * addrPort a short will cause the compiler to use sth/lduh instructions.
 */

typedef struct NetLE_Reg {
	unsigned short	dataPort;	/* RDP */
#ifdef sun4
        unsigned short	addrPort;	/* RAP */
#else
        unsigned int        	: 14;	/* Reserved - must be zero */
	unsigned int	addrPort: 2;	/* RAP */
#endif
} NetLE_Reg;

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
 * First in the mode register.
 */

typedef struct NetLEModeReg {
    unsigned int	promiscuous	:1;	/* Read all incoing packets. */
    unsigned int			:8;	/* Reserved */
    unsigned int        internalLoop	:1;	/* Internal if lookBack. */
    unsigned int	disableRetry	:1;	/* Disable collision retry. */
    unsigned int	forceCollision	:1;	/* Force collision.	*/
    unsigned int	disableCRC	:1;	/* Disable transmit CRC. */
    unsigned int	loopBack	:1;	/* Loop back mode. */
    unsigned int	disableXmit	:1;	/* Disable the transmitter. */
    unsigned int	disableRecv	:1;	/* Disable the receiver. */
} NetLEModeReg;

/*
 *  Descriptor Ring Pointer (page 21) (Byte swapped. )
 *  Also, 
 */
typedef struct NetLERingPointer {
    unsigned short	ringAddrLow	:16;	/* Low order ring address.
						 * Must be quad word aligned. 
						 */
    unsigned int	logRingLength	:3;	/* log2 of ring length. */
    unsigned int			:5;	/* Reserved */
    unsigned int	ringAddrHigh	:8;	/* High order ring address. */
} NetLERingPointer;

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
 */

typedef struct NetLERecvMsgDesc {
    unsigned short	bufAddrLow;	/* Low order 16 addr bits of buffer. */
    unsigned int	chipOwned	:1;	/* Buffer is owned by LANCE. */
    unsigned int	error		:1;	/* Error summary */
    unsigned int	framingError	:1;	/* Framing Error occured.  */
    unsigned int	overflowError	:1;	/* Packet overflowed. */
    unsigned int	crcError	:1;	/* CRC error. */
    unsigned int	bufferError	:1;	/* Buffer error. */
    unsigned int	startOfPacket	:1;	/* First buffer of packet. */
    unsigned int	endOfPacket	:1;	/* Last buffer of packet. */
    unsigned char	bufAddrHigh;	/* High order 8 addr bits of buffer. */
    short	        bufferSize;	/* Size of buffer in bytes. This 
					 * has to be the 2's complement of
					 * the buffer size.
					 */
    short	packetSize;		/* Size of the packet (bytes). */
} NetLERecvMsgDesc;


/*
 * LE Net Xmit messages descriptors (page 23-23)
 */

typedef struct NetLEXmitMsgDesc {
    unsigned short  bufAddrLow;	/* Low order 16 addr bits of buffer.*/
    unsigned int    chipOwned	    :1;	/* Buffer owned by the LANCE */
    unsigned int    error	    :1;	/* Error summary */
    unsigned int		    :1;	/* Reserved.  */
    unsigned int    retries	    :1;	/* More than one retry was needed.  */
    unsigned int    oneRetry	    :1;	/* Exactly one retry was needed. */
    unsigned int    deferred	    :1;	/* Transmission deferred. */
    unsigned int    startOfPacket   :1;	/* First buffer of packet. */
    unsigned int    endOfPacket	    :1;	/* Last buffer of packet. */
    unsigned char   bufAddrHigh;	/* High order 8 addr bits of buffer. */
    short           bufferSize;		/* Signed size of buffer in bytes. This 
					 * has to be the 2's complement of
					 * the buffer size.
					 * Note that the first buffer in a
					 * chain must have at least 100 bytes.
					 */
    unsigned int    bufferError	    :1;	/* Buffering error. */
    unsigned int    underflowError  :1;	/* Underflow error. */
    unsigned int		    :1;	/* Reserved. */
    unsigned int    lateCollision   :1;	/* Late collision error. */
    unsigned int    lostCarrier	    :1;	/* Loss of carrier error. */
    unsigned int    retryError	    :1;	/* Too many collision. */
    unsigned int    tdrCounter	    :10;/* Time Domain Reflectometry counter. */
} NetLEXmitMsgDesc;


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

    List_Links		*xmitList;	/* Pointer to the front of the list of
					   packets to be transmited. */
    List_Links      	*xmitFreeList;	/* Pointer to a list of unused 
					   transmission queue elements. */
    Boolean		transmitting;	/* Set if are currently transmitting a
					   packet. */
    Boolean		running;	/* Is the chip currently active. */
    Net_EtherAddress	etherAddressBackward;	/* The ethernet address in 
						 * reverse byte order. */
    Net_EtherAddress	etherAddress;	/* The ethernet address */
} NetLEState;

/*
 * The state of all of the interfaces. 
 */

extern	NetLEState	netLEState;


/*
 * General routines.
 */

extern	Boolean	NetLEInit();
extern	void	NetLEOutput();
extern	void	NetLEIntr();
extern	void	NetLERestart();

extern	void	NetLEReset();

/*
 * Routines for transmitting.
 */

extern	void	NetLEXmitInit();
extern	ReturnStatus	NetLEXmitDone();
extern	void	NetLEXmitRestart();

/*
 * Routines for the receive unit.
 */

extern	void	NetLERecvInit();
extern	ReturnStatus	NetLERecvProcess();

#endif /* _NETLEINT */
