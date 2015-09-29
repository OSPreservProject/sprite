/*
 * tcpInt.h --
 *
 *	Internal declarations of data structures for handling the TCP protocol.
 *	The data structures in this file are based on the functional
 *	specification of the Transmission Control Protocol in RFC 793
 *	(Sept. 1981).
 *
 *	Based on the following 4.3BSD files:
 *	  "@(#)tcp_fsm.h   7.2 (Berkeley) 12/7/87"
 *	  "@(#)tcp_seq.h   7.2 (Berkeley) 12/7/87"
 *	  "@(#)tcp_var.h   7.7 (Berkeley) 2/27/88"
 *	  "@(#)tcp_debug.h 7.2 (Berkeley) 12/7/87"
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /sprite/src/daemons/ipServer/RCS/tcpInt.h,v 1.5 89/08/15 19:55:49 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_TCPINT
#define _IPS_TCPINT

#include "sprite.h"
#include "tcpTimer.h"
#include "socket.h"
#include "list.h"
#include "netInet.h"


/*
 * Define the default maximum segment size to be the size of a default
 * IP segment, minus the header.  On the local net we can make it bigger.
 */
#define TCP_MAX_SEG_SIZE	(NET_IP_MAX_SEG_SIZE - sizeof(Net_TCPHeader))

extern int		tcpISS;		/* Initial Send sequence #. */
extern int		tcpKeepLen;	/* Size of data for keep-alive
					 * packets. */

/* data structures */

/*
 * The various states of the TCP finite state machine.
 * The names are taken from RFC 793, p. 21-2.
 */
typedef enum {
    /* These states are used before a conection is established. */
	CLOSED,		/* Not connected. */
	LISTEN,		/* Waiting for a connection from a remote TCP. */
	SYN_SENT,	/* Have sent a SYN, waiting for reply.  */
	SYN_RECEIVED,	/* Waiting for ACK of our connect request,
			 * have ACK'ed the remote's request. */

	ESTABLISHED,	/* An open connection -- data can be sent
			 * and received. */

    /* The remote TCP has closed the connection. */
	CLOSE_WAIT,	/* Got a close request from the remote TCP.
			 * Now waiting for the User to close the connection. */
	LAST_ACK,	/* Waiting for the remote TCP to ACK our FIN. */

    /* The user has closed the connection. */
	FIN_WAIT_1,	/* User has closed the connection. Have sent a FIN
			 * to the remote TCP, waiting for the ACK. */
	FIN_WAIT_2,	/* Got the ACK of our FIN, waiting for the remote
			 * TCP's FIN. */
	CLOSING,	/* Got a FIN from the remote TCP, but still waiting
			 * for ACK of our FIN. */

	TIME_WAIT	/* Waiting for enough time to pass to be sure that
			 * the remote TCP received our ACK of its FIN. */
} TCPState;

#define TCP_HAVE_RECVD_SYN(state) ((int)state >= (int)SYN_RECEIVED)
#define TCP_UNSYNCHRONIZED(state) ((int)state < (int)ESTABLISHED)
#define TCP_HAVE_RECVD_FIN(state) (state == TIME_WAIT)
#define TCP_HAVE_SENT_FIN(state) ((int)state > (int)CLOSE_WAIT)


/*
 * From RFC 793, p. 24:
 * "[...] every byte of data sent over a TCP connection has a sequence number."
 * "The sequence number space ranges from 0 to 2**32 -1. Since
 * this space if finite, all arithmetic dealing with sequence numbers
 * must be performed modulo 2**32 [... to preserve] the relationship of
 * sequence numbers as they cycle from 2**32-1 to 0 again."
 */

typedef unsigned int TCPSeqNum;

#define TCP_SEQ_LT(a, b)	((int) ((a) - (b)) < 0)
#define TCP_SEQ_LE(a, b)	((int) ((a) - (b)) <= 0)
#define TCP_SEQ_GT(a, b)	((int) ((a) - (b)) > 0)
#define TCP_SEQ_GE(a, b)	((int) ((a) - (b)) >= 0)

/*
 * Macros to initialize TCP sequence numbers for send and receive from
 * initial send and receive sequence numbers.
 *
 * TCP_INIT_SEND_SEQ_INCR is the increment added to the initial send
 * sequence number every second.
 */

#define TCP_RECV_SEQ_INIT(tcpPtr) \
    (tcpPtr)->recv.advtWindow = (tcpPtr)->recv.next = (tcpPtr)->recv.initial+1

#define TCP_SEND_SEQ_INIT(tcpPtr) \
    (tcpPtr)->send.unAck = (tcpPtr)->send.next = (tcpPtr)->send.maxSent = \
	(tcpPtr)->send.urgentPtr = (tcpPtr)->send.initial

#define TCP_INIT_SEND_SEQ_INCR	(125 * 1024)


/*
 * Information about TCP connection attempts from remote hosts. This info
 * is used accept or reject such connections. The queue length is increased
 * by 1 because circular queues waste 1 slot in order to tell if the
 * queue is full or not.
 */

#define TCP_MAX_NUM_CONNECTS	5
typedef struct {
    int		maxQueueSize;
    int		head;
    int		tail;
    Sock_InfoPtr	queue[1 + TCP_MAX_NUM_CONNECTS];
} TCPConnectInfo;


/*
 * Information used to manage TCP data flow to and from remote hosts.
 */

typedef struct {
    List_Links	reassList;		/* Segment reassembly list. */
    Net_TCPHeader *templatePtr;		/* Output header template. */
    Net_IPHeader *IPTemplatePtr;	/* IP output header template. */
    TCPConnectInfo *connectPtr;		/* Waiting connection attempts. */
    TCPState	state;			/* State of this connection. */
    int		flags;			/* Defined below. */
    int		timer[TCP_NUM_TIMERS];
    int		rxtshift;		/* Log(2) of retransmit exp. backoff. */
    int		rxtcur;			/* Current retransmit value. */
    int		dupAcks;		/* Consecutive dupl. acks received. */
    int		maxSegSize;		/* Max. segment size. */

    /* Transmit timing: */
    int		idle;			/* Inactivity timer. */
    int		rtt;			/* Round-trip time. */
    int		srtt;			/* Smoothed round-trip time. It is
					 * fixed-point with the low-order 3 bits
					 * containing the fraction. */
    int		rttvar;			/* Round-trip time "variance" (actually
					 * a smoothed difference). It is
					 * fixed-point w/ low-order 2 bits
					 * containing the fraction. */
    TCPSeqNum	rtseq;			/* Seq. # used to measure RTT */

    char	urgentData;		/* The urgent data octest */
    int		urgentBufPos;		/* Logical location of urgent data in
				 	* the receive buffer. */
    Boolean	force;			/* TRUE if forcing out a byte. */

    struct {			/* send sequence variables. */
	TCPSeqNum	unAck;		/* oldest acknowledged seq # (snd_una)*/
	TCPSeqNum	next;		/* next seq. # to be sent. (snd_nxt) */
	TCPSeqNum	window;		/* send window size. (snd_wnd) */
	TCPSeqNum	urgentPtr;	/* urgent data offset. (snd_up) */
	TCPSeqNum	updateSeqNum;	/* seq. # use for last window update.
					 * (snd_wl1) */
	TCPSeqNum	updateAckNum;	/* ack # used for last window update.
					 * (snd_wl2) */
	TCPSeqNum	initial;	/* initial seq # sent on SYN packets.
					 * (iss) */
	TCPSeqNum	maxSent;	/* highest seq. # sent, used to
					 * recognize retransmits. (snd_max) */
	unsigned int	congWindow;	/* congestion-controlled window.
					 * (snd_cwnd) */
	unsigned int	cwSizeThresh;	/* congWindow size threshold for
					 * slow start exponential to linear
					 * switch. (snd_ssthresh) */
	int		maxWindow;	/* largest window size the peer
					 * has offered. (max_sndwnd) */
    } send;

    struct {			/* receive sequence variables. */
	TCPSeqNum	next;		/* next seq. # expected on incoming
					 * packets. (rcv_nxt) */
	TCPSeqNum	window;		/* recv. window size. (rcv_wnd) */
	TCPSeqNum	urgentPtr;	/* urgent data offset. (rcv_up) */
	TCPSeqNum	initial;	/* initial seq # from peer's SYN
					 * packet. (irs) */
	TCPSeqNum	advtWindow;	/* advertised window size. (rcv_adv) */
	int		maxWindow;	/* largest amount the peer has sent
					 *  into a window. (max_rcvd) */
    } recv;
} TCPControlBlock;

/*
 * Definitions of "flags" in the TCP_ControlInfo struct.
 *
 *	TCP_ACK_NOW		- ACK the peer immediately.
 *	TCP_DELAY_ACK		- do an ACK, but try to delay it.
 *	TCP_NO_DELAY		- don't delay packets in order to coalesce.
 *	TCP_IGNORE_OPTS		- don't use the options.
 *	TCP_SENT_FIN		- a FIN has been sent.
 *	TCP_HAVE_URGENT_DATA	- urgent data is ready to be read.
 *	TCP_HAD_URGENT_DATA	- urgent data has been read.
 */

#define TCP_ACK_NOW		0x01
#define TCP_DELAY_ACK		0x02
#define TCP_NO_DELAY		0x04
#define TCP_IGNORE_OPTS		0x08
#define TCP_SENT_FIN		0x10
#define TCP_HAVE_URGENT_DATA	0x20
#define TCP_HAD_URGENT_DATA	0x40


/*
 * Commands for TCPTrace().
 */

typedef enum {
    TCP_TRACE_INPUT,
    TCP_TRACE_OUTPUT,
    TCP_TRACE_RESPOND,
    TCP_TRACE_DROP,
} TCPTraceCmd;


/* procedures */

extern void		TCPTrace();
extern void		TCPCloseConnection();
extern void		TCPDropConnection();
extern void		TCPCancelTimers();
extern void		TCPRespond();
extern void		TCPMakeTemplateHdr();
extern ReturnStatus	TCPOutput();
extern TCPControlBlock	*TCPSocketToTCB();
extern TCPControlBlock	*TCPReassemble();
extern Sock_InfoPtr	TCPCloneConnection();
extern TCPControlBlock	*TCPSockToTCB();
extern int		TCPCalcMaxSegSize();
extern void		TCPCleanReassList();
extern void		TCPPrintHdrFlags();
extern void		TCPTimerInit();
extern void		TCPSetPersist();

#endif /* _IPS_TCPINT */
