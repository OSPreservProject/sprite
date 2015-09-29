/* 
 * tcpOutput.c --
 *
 *	Routines to handle TCP output processing.
 *
 *	Based on 4.3BSD	@(#)tcp_output.c	7.12 (Berkeley) 3/24/88
 *
 *	To do:
 *	1) allow user-supplied TCP options to be sent with the segment.
 *
 *
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
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/tcpOutput.c,v 1.8 90/02/22 14:22:04 ouster Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "stat.h"
#include "ip.h"
#include "tcp.h"
#include "tcpInt.h"
#include "netInet.h"

/*
 * Initial TCP options.
 */
struct {
    char	kind;
    char	length;
    unsigned short value;
} initialOptions = { 
    NET_TCP_OPTION_MAX_SEG_SIZE, 
    sizeof(initialOptions), 
    0
};

/*
 * Values for the TCP header flags when in a certain state.
 */
static int stateToOutputFlags[] = {
   NET_TCP_RST_FLAG|NET_TCP_ACK_FLAG, 	/* CLOSED */
   0, 					/* LISTEN */
   NET_TCP_SYN_FLAG, 			/* SYN_SENT */
   NET_TCP_SYN_FLAG|NET_TCP_ACK_FLAG,	/* SYN_RECEIVED */
   NET_TCP_ACK_FLAG, 			/* ESTABLISHED */

   NET_TCP_ACK_FLAG,			/* CLOSE_WAIT */
   NET_TCP_FIN_FLAG|NET_TCP_ACK_FLAG, 	/* LAST_ACK */

   NET_TCP_FIN_FLAG|NET_TCP_ACK_FLAG, 	/* FIN_WAIT_1 */
   NET_TCP_ACK_FLAG,			/* FIN_WAIT_2 */
   NET_TCP_FIN_FLAG|NET_TCP_ACK_FLAG, 	/* CLOSING */
   NET_TCP_ACK_FLAG,			/* TIME_WAIT */
}; 

static int 	timeToLive = NET_TCP_TTL;
static void	CalcChecksum();


/*
 *----------------------------------------------------------------------
 *
 * TCPOutput --
 *
 *	This routine determines what data needs to be sent to the
 *	remote peer.
 *
 * Results:
 *	SUCCESS		- currently always returned.
 *
 * Side effects:
 *	One or more packets may be sent to the network.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
TCPOutput(sockPtr, tcbPtr)
    Sock_InfoPtr		sockPtr;
    register TCPControlBlock	*tcbPtr;
{
    register int	len;
    register int	window;
    int			amtUnacked;
    int			sendBufAmtUsed;
    int			flags;
    Boolean		idle;
    Boolean		sendAlot;
    unsigned char	*opt;
    int			optlen = 0;
    IPS_Packet		packet;
    Address		ptr;

    /*
     * Determine length of data that should be transmitted, and flags that
     * will be used.  If there is some data or critical controls (SYN, RST)
     * to send, then transmit; otherwise, investigate further.
     *
     * We're idle if all the stuff we've sent has been ACKed.
     */

    idle = (tcbPtr->send.maxSent == tcbPtr->send.unAck);

again:

    sendAlot = FALSE;
    amtUnacked = tcbPtr->send.next - tcbPtr->send.unAck;
    window = MIN(tcbPtr->send.window, tcbPtr->send.congWindow);

    /*
     * If called from the persist timeout handler with a window of 0, 
     * send 1 byte.  Otherwise, if the window is small but nonzero and 
     * the timer has expired, we will send what we can and go to the 
     * transmit state.
     */

    if (tcbPtr->force) {
	if (window == 0) {
	    window = 1;
	} else {
	    tcbPtr->timer[TCP_TIMER_PERSIST] = 0;
	    tcbPtr->rxtshift = 0;
	}
    }

    sendBufAmtUsed = Sock_BufSize(sockPtr, SOCK_SEND_BUF, SOCK_BUF_USED);

    len = MIN(sendBufAmtUsed, window) - amtUnacked;

    flags = stateToOutputFlags[(int)tcbPtr->state];
    if (len < 0) {
	/*
	 * If a FIN has been sent but not acked, and we haven't been
	 * called to retransmit, then len will be -1 so transmit if
	 * acking, otherwise return.  If a FIN has not been sent, the
	 * window shrank after we sent into it.  If the window shrank to
	 * 0, cancel the pending retransmit and pull send.next back to
	 * (closed) window.  We will enter the persist state below.  If
	 * the window didn't close completely, just wait for an ACK.
	 */

	len = 0;
	if (window == 0) {
	    tcbPtr->timer[TCP_TIMER_REXMT] = 0;
	    tcbPtr->send.next = tcbPtr->send.unAck;
	}
    } 
    if (len > tcbPtr->maxSegSize) {
	len = tcbPtr->maxSegSize;
	sendAlot = TRUE;
    }

    /*
     * If there's data to be sent, make sure the FIN flag is off.
     */
    if (TCP_SEQ_LT(tcbPtr->send.next+ len, tcbPtr->send.unAck+ sendBufAmtUsed)){
	flags &= ~NET_TCP_FIN_FLAG;
    }
    window = Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_FREE); 


    /*
     * If our state indicates that FIN should be sent and we have not 
     * yet done so, or we're retransmitting the FIN, then we need to send.
     */
    if ((flags & NET_TCP_FIN_FLAG) &&
	  ( !(tcbPtr->flags & TCP_SENT_FIN) || 
	  (tcbPtr->send.next == tcbPtr->send.unAck))) {
	goto send;
    }

    /*
     * We want to send a packet if any of the following conditions hold:
     *  1) we owe our peer an ACK.
     *  2) we need to send a SYN or RESET flag
     *  3) there's urgent data to send.
     */
    if ((tcbPtr->flags & TCP_ACK_NOW) ||
	(flags & (NET_TCP_SYN_FLAG|NET_TCP_RST_FLAG)) ||
	(TCP_SEQ_GT(tcbPtr->send.urgentPtr, tcbPtr->send.unAck))) {
	goto send;
    }

    /*
     * Sender silly window avoidance.  If the connection is idle and we
     * can send all data, either a maximum segment or at least a maximum
     * default-size segment, then do it. Also send if the persist force flag 
     * is set. If peer's buffer is tiny, then send when window is at least 
     * half open.  If retransmitting (possibly after persist timer forced us 
     * to send into a small window), then we must resend.
     */
    if (len != 0) {
	if (len == tcbPtr->maxSegSize) {
	    goto send;
	}
	if ((idle || (tcbPtr->flags & TCP_NO_DELAY)) &&
	    (len + amtUnacked) >= sendBufAmtUsed) {
	    goto send;
	}
	if (tcbPtr->force ||
	    (len >= (tcbPtr->send.maxWindow / 2)) ||
	    (TCP_SEQ_LT(tcbPtr->send.next, tcbPtr->send.maxSent))) {
	    goto send;
	}
    }

    /*
     * Compare the available window to amount of window known to the peer
     * (i.e., advertised window less next expected input.)  If the difference
     * is 35% or more of the maximum possible window, then we want to send
     * a window update to the peer.
     */
    if (window > 0) {
	int advertised = window - (tcbPtr->recv.advtWindow - tcbPtr->recv.next);
	if (Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_USED) == 0 &&
	    advertised >= (2 * tcbPtr->maxSegSize)) {
	    goto send;
	}
	if (((100 * advertised) / 
	    Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_MAX_SIZE)) >= 35){
	    goto send;
	}
    }

    /*
     * TCP window updates are not reliable, so a polling protocol
     * using "persist" packets is used to insure receipt of window
     * updates.  The three "states" for the output side are:
     *	  idle			not doing retransmits or persists,
     *	  persisting		to move a small or zero window,
     *	  (re)transmitting	and thereby not persisting.
     *
     * tcbPtr->timer[TCP_TIMER_PERSIST] is set when we are in persist state.
     * tcbPtr->force  is set when we are called to send a persist packet.
     * tcbPtr->timer[TCP_TIMER_REXMT]  is set when we are retransmitting
     * The output side is idle when both timers are zero.
     *
     * If the send window is too small, and there are data to transmit, and no
     * retransmit or persist timer is pending, then go to the persist state.
     * If nothing happens soon, send when the timer expires:  if the window is
     * nonzero, transmit what we can, otherwise force out a byte.
     */

    if (sendBufAmtUsed != 0 &&
	(tcbPtr->timer[TCP_TIMER_REXMT] == 0) &&
	(tcbPtr->timer[TCP_TIMER_PERSIST] == 0)) {

	tcbPtr->rxtshift = 0;
	TCPSetPersist(tcbPtr);
    }

    /*
     * No reason to send a segment, just return.
     */
    return(SUCCESS);

send:

    /*
     * Initialize a packet and copy the data to be transmitted into the
     * packet. Also, initialize the header from the template for sends 
     * on this connection.
     */

    IPS_InitPacket(len, &packet);

    if (len != 0) {
	if (tcbPtr->force && len == 1) {
	    stats.tcp.send.probe++;
	} else if (TCP_SEQ_LT(tcbPtr->send.next, tcbPtr->send.maxSent)) {
	    stats.tcp.send.rexmitPack++;
	    stats.tcp.send.rexmitByte += len;
	} else {
	    stats.tcp.send.pack++;
	    stats.tcp.send.byte += len;
	}
	Sock_BufCopy(sockPtr, SOCK_SEND_BUF, amtUnacked, len, packet.data);
	if (ips_Debug) {
	    (void) fprintf(stderr, "TCP Output: sending '%.*s'\n",
		MIN(len, 20), packet.data);
	}
    } else if (tcbPtr->flags & TCP_ACK_NOW) {
	stats.tcp.send.acks++;
    } else if (flags & (NET_TCP_SYN_FLAG|NET_TCP_FIN_FLAG|NET_TCP_RST_FLAG)) {
	stats.tcp.send.ctrl++;
    } else if (TCP_SEQ_GT(tcbPtr->send.urgentPtr, tcbPtr->send.unAck)) {
	stats.tcp.send.urg++;
    } else {
	stats.tcp.send.winUpdate++;
    }


    /*
     * Before the ESTABLISHED state, we must send the initial options
     * unless an option is set to not do any options.
     */
    opt = (unsigned char *) NULL;
    if (TCP_UNSYNCHRONIZED(tcbPtr->state) &&
	!(tcbPtr->flags & TCP_IGNORE_OPTS)) {
	unsigned short mss;

	mss = MIN(Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_MAX_SIZE) / 2,
			 TCPCalcMaxSegSize(tcbPtr));
	if (mss > NET_IP_MAX_SEG_SIZE - sizeof(Net_TCPHeader)) {
	    opt = (unsigned char *) &initialOptions;
	    optlen = sizeof(initialOptions);
	    initialOptions.value = Net_HostToNetShort(mss);
	}
#ifdef not_def
    } else if (tcbPtr->tcpOptions != (Address) NULL) {
	opt = (unsigned char *) tcbPtr->tcpOptions;
	optlen = tcbPtr->tcpOptLen;
#endif not_def
    }

    /*
     * Copy the options to the packet. Make sure they are padded to
     * an 4-byte multiple.
     */
    ptr = packet.data;
    if (opt != (unsigned char *) NULL) {
	int padLen = 0;
	while ((optlen & 0x3) != 0) {
	    ptr--;
	    *ptr = NET_TCP_OPTION_EOL;
	    padLen++;
	}
	ptr -= optlen;
	bcopy((Address)opt, ptr, (int)optlen);
	optlen += padLen;
    }

#define tcpHdrPtr	packet.hdr.tcpPtr
#define ipHdrPtr	packet.ipPtr

    tcpHdrPtr = (Net_TCPHeader *) (ptr - sizeof(Net_TCPHeader));
    packet.hdrLen = sizeof(Net_TCPHeader) + optlen;
    ipHdrPtr = (Net_IPHeader *)(((Address) tcpHdrPtr) - sizeof(Net_IPHeader));

    if (tcbPtr->templatePtr == (Net_TCPHeader *)NULL) {
	panic("TCPOutput: no template");
    }
    *tcpHdrPtr = *tcbPtr->templatePtr;


    /*
     * Fill in the header fields, remembering the maximum advertised
     * window for use in delaying messages about window sizes.
     * If resending a FIN, be sure not to use a new sequence number.
     */
    if ((flags & NET_TCP_FIN_FLAG) && 
	(tcbPtr->flags & TCP_SENT_FIN) && 
	len == 0) {

	tcbPtr->send.next--;
    }
    tcpHdrPtr->seqNum = Net_HostToNetInt(tcbPtr->send.next);
    tcpHdrPtr->ackNum = Net_HostToNetInt(tcbPtr->recv.next);
    tcpHdrPtr->dataOffset = (sizeof(Net_TCPHeader) + optlen) >> 2;
    tcpHdrPtr->flags = flags;

    /*
     * Calculate the receive window size.  Don't shrink the window, but 
     * avoid silly window syndrome.
     */

    if (window < Sock_BufSize(sockPtr,SOCK_RECV_BUF, SOCK_BUF_MAX_SIZE) / 4  &&
	window < tcbPtr->maxSegSize) {
	window = 0;
    }
    if (window > NET_IP_MAX_PACKET_SIZE) {
	window = NET_IP_MAX_PACKET_SIZE;
    }
    if (window < (tcbPtr->recv.advtWindow - tcbPtr->recv.next)) {
	window = tcbPtr->recv.advtWindow - tcbPtr->recv.next;
    }
    tcpHdrPtr->window = Net_HostToNetShort((unsigned short)window);

    if (TCP_SEQ_GT(tcbPtr->send.urgentPtr, tcbPtr->send.next)) {
	tcpHdrPtr->urgentOffset = 
		Net_HostToNetShort((unsigned short)(tcbPtr->send.urgentPtr - 
		    tcbPtr->send.next));
	tcpHdrPtr->flags |= NET_TCP_URG_FLAG;
    } else {
	/*
	 * If there's no urgent pointer to send, then we pull
	 * the urgent pointer to the left edge of the send window
	 * so that it doesn't drift into the send window on sequence
	 * number wraparound.
	 */
	tcbPtr->send.urgentPtr = tcbPtr->send.unAck; /* drag it along */
    }

    /*
     * If there's anything to send and we can send it all, set the PUSH flag.
     * (This will keep happy those implementations which only give data
     * to the user when a buffer fills or a PUSH comes in.)
     */

    if ((len != 0) && 
	((amtUnacked+len) == Sock_BufSize(sockPtr, SOCK_SEND_BUF, 
				SOCK_BUF_USED) )) {
	tcpHdrPtr->flags |= NET_TCP_PSH_FLAG;
    }

    IP_FormatPacket(NET_IP_PROTOCOL_TCP,  timeToLive,
		    tcbPtr->IPTemplatePtr->source,
		    tcbPtr->IPTemplatePtr->dest, &packet);
    CalcChecksum(&packet, len + optlen);

    /*
     * When in the transmit state, time the transmission and arrange for
     * the retransmit.  In the persist state, just set send.maxSent.
     */
    if ((!tcbPtr->force) || (tcbPtr->timer[TCP_TIMER_PERSIST] == 0)) {
	TCPSeqNum	startSeq;

	startSeq = tcbPtr->send.next;
	/*
	 * Advance send.next over sequence space of this segment.
	 */
	if (flags & NET_TCP_SYN_FLAG) {
	    tcbPtr->send.next++;
	}
	if (flags & NET_TCP_FIN_FLAG) {
	    tcbPtr->send.next++;
	    tcbPtr->flags |= TCP_SENT_FIN;
	}

	tcbPtr->send.next += len;
	if (TCP_SEQ_GT(tcbPtr->send.next, tcbPtr->send.maxSent)) {
	    tcbPtr->send.maxSent = tcbPtr->send.next;

	    /*
	     * Time this transmission if it's not a retransmission and
	     * we are not currently timing anything.
	     */
	    if (tcbPtr->rtt == 0) {
		tcbPtr->rtt = 1;
		tcbPtr->rtseq = startSeq;
		stats.tcp.segsTimed++;
	    }
	}

	/*
	 * Set the retransmit timer if it's not currently set
	 * and we're not doing an ACK or a keep-alive probe.
	 * The initial value for the retransmit timer is smoothed
	 * round-trip time + 2 * round-trip time variance.
	 * Also initialize the shift counter, which is used for backoff
	 * of retransmit time.
	 */
	if (tcbPtr->timer[TCP_TIMER_REXMT] == 0 &&
	    tcbPtr->send.next != tcbPtr->send.unAck) {

	    tcbPtr->timer[TCP_TIMER_REXMT] = tcbPtr->rxtcur;
	    if (tcbPtr->timer[TCP_TIMER_PERSIST] != 0) {
		tcbPtr->timer[TCP_TIMER_PERSIST] = 0;
		tcbPtr->rxtshift = 0;
	    }
	}
    } else {
	if (TCP_SEQ_GT(tcbPtr->send.next + len, tcbPtr->send.maxSent)) {
	    tcbPtr->send.maxSent = tcbPtr->send.next + len;
	}
    }

    /*
     * Trace this operation if debugging is set.
     */

    if (Sock_IsOptionSet(sockPtr, NET_OPT_DEBUG)) {
	TCPTrace(TCP_TRACE_OUTPUT, tcbPtr->state, tcbPtr, tcpHdrPtr, len);
    }
    if (ips_Debug) {
	(void) fprintf(stderr, 
			"TCP Output: %d bytes <%x,%d,#%d> -> <%x,%d>\n\t", 
			len, 
			Net_NetToHostInt(ipHdrPtr->source), 
			Net_NetToHostShort(tcpHdrPtr->srcPort), 
			Net_NetToHostShort(ipHdrPtr->ident),
			Net_NetToHostInt(ipHdrPtr->dest), 
			Net_NetToHostShort(tcpHdrPtr->destPort));
	TCPPrintHdrFlags(stderr, tcpHdrPtr->flags);
	(void) fprintf(stderr, "  seq=%d, ack=%d, wind=%d\n", 
		Net_NetToHostInt(tcpHdrPtr->seqNum), 
		Net_NetToHostInt(tcpHdrPtr->ackNum),
		Net_NetToHostShort(tcpHdrPtr->window));
    }


    IP_QueueOutput(&packet);
#ifdef old_way
    {
	ReturnStatus	status;
	status = IP_Output(&packet, TRUE);
	free(packet.base);
	if (status != SUCCESS) {
	    return(status);
	}
    }
#endif old_way

    stats.tcp.send.total++;

    /*
     * The data have been sent (as far as we can tell). If this advertises a 
     * larger window than any other segment, then remember the size of 
     * the advertised window. Any pending ACK has now been sent.
     */

    if ((window > 0) && 
	TCP_SEQ_GT(tcbPtr->recv.next + window, tcbPtr->recv.advtWindow)) {
	tcbPtr->recv.advtWindow = tcbPtr->recv.next + window;
    }

    tcbPtr->flags &= ~(TCP_ACK_NOW | TCP_DELAY_ACK);

    if (sendAlot) {
	goto again;
    }
    return(SUCCESS);
#undef tcpHdrPtr
#undef ipHdrPtr
}


/*
 *----------------------------------------------------------------------
 *
 * TCPRespond --
 *
 *	Send a single message to the TCP at address specified by
 *	the given TCP/IP header.  If flags==0, then we make a copy
 *	of the tcpiphdr at ti and send directly to the addressed host.
 *	This is used to force keep alive messages out using the TCP
 *	template for a connection *tcbPtr->templatePtr.  If flags are given
 *	then we send a message back to the TCP which originated the
 *	segment ti, and discard the mbuf containing it and any other
 *	attached mbufs.
 *
 *	In any case the ack and sequence number of the transmitted
 *	segment are as specified by the parameters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A packet is sent to the remote peer.
 *
 *----------------------------------------------------------------------
 */

void
TCPRespond(sockPtr, tcpHdrPtr, ipHdrPtr, ack, seq, flags)
    Sock_InfoPtr	sockPtr;	/* Socket that is sending a response. */
    register Net_TCPHeader *tcpHdrPtr;	/* TCP header from the packet we are
					 * responding to. */
    register Net_IPHeader *ipHdrPtr;	/* IP header from the the packet. */
    TCPSeqNum		ack;		/* Ack and sequence numbers, flags 
					 * to be sent. */
    TCPSeqNum		seq;
    int 		flags;
{
    int 		window = 0;
    IPS_Packet		packet;

    if (sockPtr != (Sock_InfoPtr) NULL) {
	window = Sock_BufSize(sockPtr, SOCK_RECV_BUF, SOCK_BUF_FREE);
    }

    IPS_InitPacket(tcpKeepLen, &packet);

    if (flags == 0) {
	/*
	 * We're sending a response because of a time-out.
	 */
	packet.dataLen = tcpKeepLen;
	flags = NET_TCP_ACK_FLAG;
    } else {
	/*
	 * We're sending a response back to the origin. Swap the source
	 * and destination addresses and ports from the packet.
	 */
	packet.dataLen = 0;
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
	xchg(ipHdrPtr->dest, ipHdrPtr->source, Net_InetAddress);
	xchg(tcpHdrPtr->destPort, tcpHdrPtr->srcPort, unsigned short);
#undef xchg
    }
    packet.hdr.tcpPtr = (Net_TCPHeader *) (packet.data - sizeof(Net_TCPHeader));
    packet.hdrLen = sizeof(Net_TCPHeader);
    packet.ipPtr = (Net_IPHeader *) ((Address)packet.hdr.tcpPtr - 
			sizeof(Net_IPHeader));

    *packet.hdr.tcpPtr = *tcpHdrPtr;
    packet.hdr.tcpPtr->seqNum		= Net_HostToNetInt(seq);
    packet.hdr.tcpPtr->ackNum		= Net_HostToNetInt(ack);
    packet.hdr.tcpPtr->dataOffset	= sizeof(Net_TCPHeader) >> 2;
    packet.hdr.tcpPtr->urgentOffset	= 0;
    packet.hdr.tcpPtr->flags		= flags;
    packet.hdr.tcpPtr->window	   = Net_HostToNetShort((unsigned short)window);


    IP_FormatPacket(NET_IP_PROTOCOL_TCP,  timeToLive,
		    ipHdrPtr->source, ipHdrPtr->dest, &packet);

    if (ips_Debug) {
	(void) fprintf(stderr, 
			"TCP Respond:  <%x,%d,#%d> -> <%x,%d>\n\t", 
			Net_NetToHostInt(ipHdrPtr->source), 
			Net_NetToHostShort(packet.hdr.tcpPtr->srcPort), 
			Net_NetToHostShort(ipHdrPtr->ident),
			Net_NetToHostInt(ipHdrPtr->dest), 
			Net_NetToHostShort(packet.hdr.tcpPtr->destPort));
	TCPPrintHdrFlags(stderr, (unsigned short)flags);
	(void) fprintf(stderr, "  seq=%d, ack=%d, wind=%d\n", 
		Net_NetToHostInt(packet.hdr.tcpPtr->seqNum), 
		Net_NetToHostInt(packet.hdr.tcpPtr->ackNum),
		Net_NetToHostShort(packet.hdr.tcpPtr->window));
    }
    CalcChecksum(&packet, packet.dataLen);


    (void) IP_Output(&packet, TRUE);
    free(packet.base);
}


/*
 *----------------------------------------------------------------------
 *
 * CalcChecksum --
 *
 *	Calculates the checksum for the TCP header, pseudo-header and data.
 *	It is assumed that the data immediately follows the TCP header.
 *	The data may include TCP header options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CalcChecksum(packetPtr, len)
    IPS_Packet *packetPtr;	/* Packet descriptor. */
    int		len;		/* Length of data following the TCP header. */ 
{
    Net_IPPseudoHdr	pseudoHdr;
    Net_TCPHeader	*tcpPtr = packetPtr->hdr.tcpPtr;
    Net_IPHeader	*ipPtr  = packetPtr->ipPtr;

    len += sizeof(Net_TCPHeader);
    pseudoHdr.source	= (ipPtr->source);
    pseudoHdr.dest	= (ipPtr->dest);
    pseudoHdr.zero	= 0;
    pseudoHdr.protocol	= NET_IP_PROTOCOL_TCP;
    pseudoHdr.len	= Net_HostToNetShort(len);
    tcpPtr->checksum = 0;
    tcpPtr->checksum = Net_InetChecksum2(len, (Address) tcpPtr, &pseudoHdr);
}
