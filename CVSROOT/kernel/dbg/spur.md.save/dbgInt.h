/*
 * dbgInt.h --
 *
 *	Declarations of the network protocol for the SPUR kernel debugger and
 *	downloader.
 *
 * The interface to the debugger and the stub consists of two routines:
 *
 * void getpkt(buffer)
 *	char *buffer;
 * and
 *
 * void putpkt(buffer)
 *	char *buffer;
 *
 * where buffer is a null (zero) byte terminate string.  putpkt sends a buffer 
 * to the other side and returns when it is received.  getpkt receives a data 
 * buffers sent by the other side.  The protocol used assumes that the
 * two sides agree on sending and receiving. If one side executes a putpkt the
 * other side should execute a getpkt().
 *
 * The ethernet debugging stub runs it's protocol on top of UDP datagrams. 
 * To recover from loss packets, the side sending data (putpkt) is respondsible
 * for retransmitting the data until an acknowledgement is received. This
 * acknowledgement maybe an explicit ACK or the next command. All data and
 * acks contain a sequence number in the header. To send data the putpkt 
 * command sends a packet with header.type == DBG_DATA_PACKET and the
 * sequence number greater than any sequence number received or sent so 
 * far.  After sending the packet, the putpkt routine must wait for a
 * packet with a sequence number greater than to equal to the one sent.
 *
 * The getpkt() routine receives data packets and if it has not already
 * seen it before it sends an ACK and returns.
 * 
 *
 * The kernel downloader uses the same packet headers but has header type 
 * of DBG_DOWNLOAD_PACKET.
 *
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

#ifndef _DBGINT
#define _DBGINT

/*
 * This header starts caller DBG packets. The contents should be all 32bit
 * integers and sent in network byte order. An data bytes should follow this
 * packet.
 */

typedef struct Dbg_PacketHeader {
    unsigned int	magic;		/* Magic number for packet. */
    int			type;		/* Pack type - see below. */
    unsigned int	sequenceNumber; /* Sequence number of packet. */
    int			initial;	/* 1 if this is the initial seq # */
    unsigned int	startAddress;	/* Start of address for download. */
    int			command;	/* Download command. */
    int			dataLength;	/* Length of data portion. */
} Dbg_PacketHeader;

/*
 * Magic number for header. This is for two reasons: throwing away garbage sent
 * to the DBG port and watching for bad byte swap routines.
 */

#define	DBG_HEADER_MAGIC	0x01020304

/*
 * Packet types.
 */
#define	DBG_ACK_PACKET	1
#define	DBG_DATA_PACKET 2
#define	DBG_DOWNLOAD_PACKET	4

/*
 * DBG_DOWNLOAD_PACKET commands.
 */
#define	DBG_DOWNLOAD_PING	0	/* Packet echo. */
#define	DBG_DOWNLOAD_DATA_XFER	1	/* Data transfer. */
#define	DBG_DOWNLOAD_ZERO_MEM	2	/* Zero memory. */
#define	DBG_DOWNLOAD_JUMP	3	/* Jump to address. */

/*
 * The UDP port number used by the debugger.
 */
#define	DBG_UDP_PORT	3033

/*
 * Maximum size of the data portion.
 */

#define	DBG_MAX_BUFFER_SIZE	1420


extern void Dbg_IPputpkt();
extern void Dbg_IPgetpkt();
extern void Dbg_Rs232putpkt();
extern void Dbg_Rs232getpkt();

#endif _DBGINT
