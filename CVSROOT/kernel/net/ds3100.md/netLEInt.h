/*
 * netLEInt.h --
 *
 *	External definitions for the Am7990 (LANCE) ethernet controller.
 * The description of the definitions here come from AMD Am7990 LANCE
 * data sheet (Publication 05698 Rev B) and the Am7990 Techincal Manual.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
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

#define NET_LE_CONTROL_REG_ADDR		0xB8000000

#define	NET_LE_NUM_RECV_BUFFERS_LOG2	4
#define	NET_LE_NUM_RECV_BUFFERS		(1 << NET_LE_NUM_RECV_BUFFERS_LOG2)

#define	NET_LE_NUM_XMIT_BUFFERS_LOG2	0
#define	NET_LE_NUM_XMIT_BUFFERS		(1 << NET_LE_NUM_XMIT_BUFFERS_LOG2)


#define	NET_LE_RECV_BUFFER_SIZE		1536
#define NET_LE_MIN_FIRST_BUFFER_SIZE	100

#define	NET_LE_NUM_XMIT_ELEMENTS	32

/*
 * Buffer size from the chips point of view.
 */
#define NET_LE_DMA_BUF_SIZE		0x20000
#define NET_LE_DMA_CHIP_ADDR_MASK	(NET_LE_DMA_BUF_SIZE - 1)
#define NET_LE_DMA_BUFFER_ADDR		0xB9000000

/*
 * Macros for converting chip to cpu and cpu to chip addresses.
 */
unsigned BUF_TO_CHIP_ADDR();
#define BUF_TO_ADDR(base, offset) ((volatile short *)(((unsigned)(base) & 0x1)?\
			    (unsigned)(base) + (offset) * 2 + (offset) % 2 : \
			    (unsigned)(base) + (offset) * 2 - (offset) % 2))

#define	CHIP_TO_BUF_ADDR(addr) ((volatile unsigned short *) ((((unsigned)addr & NET_LE_DMA_CHIP_ADDR_MASK) << 1) | 0xB9000000))

/*
 * The LANCE chip has four control and status registers that are selected by a
 * register address port (RAP) register. The top 14 bits of RAP are reserved
 * and read as zeros. The register accessable in the register data port (RDP)
 * is selected by the value of the RAP. (page 15)
 */

typedef struct NetLE_Reg {
	unsigned short	dataPort;	/* RDP */
	unsigned short		: 14;	/* Reserved - must be zero */
	unsigned short	addrPort: 2;	/* RAP */
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
 * LE Initialization block.
 */
#define	NET_LE_INIT_MODE	0x0
#define NET_LE_INIT_ETHER_ADDR	0x2
#define NET_LE_INIT_MULTI_MASK	0x8
#define NET_LE_INIT_RECV_LOW	0x10
#define NET_LE_INIT_RECV_HIGH	0x12
#define NET_LE_INIT_RECV_LEN	0x13
#define NET_LE_INIT_XMIT_LOW	0x14
#define NET_LE_INIT_XMIT_HIGH	0x16
#define NET_LE_INIT_XMIT_LEN	0x17

#define NET_LE_INIT_SIZE		0x18

/*
 * LE Net Recv messages descriptors
 */
#define NET_LE_RECV_BUF_ADDR_LOW	0x0
#define NET_LE_RECV_STATUS		0x2
#define NET_LE_RECV_BUF_SIZE		0x4
#define NET_LE_RECV_PACKET_SIZE		0x6
#define NET_LE_RECV_DESC_SIZE		8

/*
 * Bits in the status.
 */
#define NET_LE_RECV_CHIP_OWNED		0x8000
#define NET_LE_RECV_ERROR		0x4000
#define NET_LE_RECV_FRAMING_ERROR	0x2000
#define NET_LE_RECV_OVER_FLOW_ERROR	0x1000
#define NET_LE_RECV_CRC_ERROR		0x0800
#define NET_LE_RECV_BUFFER_ERROR	0x0400
#define NET_LE_RECV_START_OF_PACKET	0x0200
#define NET_LE_RECV_END_OF_PACKET	0x0100
#define NET_LE_RECV_BUF_ADDR_HIGH	0x00FF

/*
 * LE Net Xmit messages descriptors (page 23-23)
 */
#define NET_LE_XMIT_BUF_ADDR_LOW	0x0
#define NET_LE_XMIT_STATUS1		0x2
#define NET_LE_XMIT_BUF_SIZE		0x4
#define NET_LE_XMIT_STATUS2		0x6
#define NET_LE_XMIT_DESC_SIZE		8

/*
 * The status bits in the first status short.
 */
#define	NET_LE_XMIT_CHIP_OWNED		0x8000
#define NET_LE_XMIT_ERROR		0x4000
#define NET_LE_XMIT_RETRIES		0x1000
#define NET_LE_XMIT_ONE_RETRY		0x0800
#define NET_LE_XMIT_DEFERRED		0x0400
#define NET_LE_XMIT_START_OF_PACKET	0x0200
#define NET_LE_XMIT_END_OF_PACKET	0x0100
#define NET_LE_XMIT_BUF_ADDR_HIGH	0x00FF

/*
 * Bits in the second status short.
 */
#define NET_LE_XMIT_BUFFER_ERROR	0x8000
#define NET_LE_XMIT_UNDER_FLOW_ERROR	0x4000
#define NET_LE_XMIT_LATE_COLLISION	0x1000
#define NET_LE_XMIT_LOST_CARRIER	0x0800
#define NET_LE_XMIT_RETRY_ERROR		0x0400
#define NET_LE_XMIT_TDR_COUNTER		0x03FF

/*
 * Structure to hold all state information associated with one of these
 * chips.
 */
typedef struct {
    volatile unsigned short *regAddrPortPtr; /* Which register to read or 
					      *	write. */
    volatile unsigned short *regDataPortPtr; /* The value to write or read 
					      * from. */
    Address		initBlockPtr;	/* Chip initialization block. */
    /*
     * Pointers for ring of receive buffers. 
     */
    Address		recvDescFirstPtr;/* Ring of receive desc start.*/
    Address		recvDescNextPtr; /* Next receive desc to be filled. */
    Address		recvDescLastPtr; /* Ring of receive descriptors end. */
    /*
     * Pointers for ring of transmit buffers. 
     */
    Address		xmitDescFirstPtr;/* Ring of xmit descriptors start.*/
    Address		xmitDescNextPtr; /* Next xmit desc to be filled. */
    Address		xmitDescLastPtr; /* Ring of xmit descriptors end. */

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
extern	Address	NetLEMemAlloc();

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

