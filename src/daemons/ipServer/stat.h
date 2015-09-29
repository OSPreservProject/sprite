/*
 * stat.h --
 *
 *	Data structure declartions for recording statistics from the
 *	Internet protocol modules.
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
 * $Header: /sprite/src/daemons/ipServer/RCS/stat.h,v 1.1 89/09/02 14:40:04 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_STAT
#define _IPS_STAT

#include "spriteTime.h"
#include "netInet.h"

/* constants */

/* data structures */

typedef struct {

    Time	startTime;		/* Time when server started. */

    struct {
	unsigned int	totalRcv;	/* Total # of packets received. */
	unsigned int	shortPacket;	/* Packet len. was smaller than the
					*  min. IP header size. */
	unsigned int	shortHeader;	/* The headerLen value in the header
					 * wass smaller that the min. header 
					 * size. */
	unsigned int	shortLen;	/* The headerLen was bigger than the
					 * totalLen. */
	unsigned int	badChecksum;	/* Checksum didn't match recomputed 
					 * value.*/
	unsigned int	fragsRcv;	/* # of packets received that were 
					 * fragmented when they arrived. */
	unsigned int	fragsDropped;	/* # of fragments received that were
					 * dropped because they were replaced
					 * by other fragments. */
	unsigned int	fragsTimedOut;	/* # of fragments received that were
					 * dropped because the other fragments
					 * didn't arrive before the timeout.*/
	unsigned int	fragTimeouts;	/* # of fragments reassembly timeouts.*/
	unsigned int	fragsReass;	/* # of fragments that were 
					 * reasssembled. */
	unsigned int	forwards;	/* # of received packets that weren't
					 * for us and had to be forwarded. */
	unsigned int	cannotForward;	/* # of packets to be forwarded but
					 * couldn't determine who to send to. */

	unsigned int	wholeSent;	/* # of unfragmented packets sent. */
	unsigned int	fragOnSend;	/* # of packets that were fragmented 
					 * before sent because they were too
					 * big to send whole. */
	unsigned int	fragsSent;	/* # of fragments sent. */
	unsigned int	dontFragment;	/* # of packets to be sent that were
					 * too big but couldn't be fragmented 
					 * (DONT_FRAGMENT was set). */
    } ip;

    struct {
	unsigned int	total;
	unsigned int	shortLen;
	unsigned int	badChecksum;
	unsigned int	badType;	/* # of packets that had a bad ICMP
					 * request type. */
	unsigned int	badCode;	/* # of packets with a bad ICMP code. */
	unsigned int	redirectSent;	/* # of "redirect" packets sent out. */

	unsigned int	inHistogram[NET_ICMP_MAX_TYPE];	
					/* histogram of request types in 
					 * incoming ICMP packets. */
	unsigned int	outHistogram[NET_ICMP_MAX_TYPE];
					/* histogram of request types in 
					 * outgoing ICMP packets. */
    } icmp;

    struct {
	struct {
	    unsigned int	total;
	    unsigned int	dataLen;	/* # of bytes sent. */
	} send;
	struct {
	    unsigned int	total;		/* total # received. */
	    unsigned int	dataLen;	/* total # of valid bytes 
						 * received. */ 
	    unsigned int	shortLen;	/* bad length in header. */
	    unsigned int	badChecksum;	/* bad header checksum. */
	    unsigned int	daemon;		/* destined for rwhod, sunrpc
						 * and route daemons. */
	    unsigned int	accepted;	/* a client wants the packet.*/
	    unsigned int	acceptLen;	/* # of bytes wanted. */
	} recv;
    } udp;

    struct {
	unsigned int	connAttempts;	/* connections initiated */
	unsigned int	accepts;	/* connections accepted */
	unsigned int	connects;	/* connections established */
	unsigned int	drops;		/* connections dropped */
	unsigned int	connDrops;	/* embryonic connections dropped */
	unsigned int	closed;		/* conn. closed (includes drops) */
	unsigned int	segsTimed;	/* segs where we tried to get rtt */
	unsigned int	rttUpdated;	/* times we succeeded */
	unsigned int	delayAck;	/* delayed acks sent */
	unsigned int	timeoutDrop;	/* conn. dropped in rxmt timeout */
	unsigned int	rexmtTimeout;	/* retransmit timeouts */
	unsigned int	persistTimeout;	/* persist timeouts */
	unsigned int	keepTimeout;	/* keepalive timeouts */
	unsigned int	keepProbe;	/* keepalive probes sent */
	unsigned int	keepDrops;	/* connections dropped in keepalive */
	unsigned int	mslTimeout;	/* 2*MSL timeouts. */
	unsigned int	timerCalls;	/* # times the timeout handler is 
					 * called.*/

	struct {
	    unsigned int	total;	/* total packets sent */
	    unsigned int	pack;	/* data packets sent */
	    unsigned int	byte;	/* data bytes sent */
	    unsigned int	rexmitPack;	/* data packets retransmitted */
	    unsigned int	rexmitByte;	/* data bytes retransmitted */
	    unsigned int	acks;	/* ack-only packets sent */
	    unsigned int	probe;	/* window probes sent */
	    unsigned int	urg;	/* packets sent with URG only */
	    unsigned int	winUpdate;/* window update-only packets sent */
	    unsigned int	ctrl;	/* control (SYN|FIN|RST) packets sent */
	} send;

	struct {
	    unsigned int	total;	/* total packets received */
	    unsigned int	pack;	/* packets received in sequence */
	    unsigned int	byte;	/* bytes received in sequence */
	    unsigned int	badChecksum;	/* packets rcvd with checksum 
						 * errors */
	    unsigned int	badOffset;	/* packets rcvd with bad 
						 * offset */
	    unsigned int	shortLen;	/* packets received too short */
	    unsigned int	dupPack;	/* duplicate-only packets rcvd*/
	    unsigned int	dupByte;	/* duplicate-only bytes recvd */
	    unsigned int	partDupPack;	/* packets with some duplicate 
						 * data */
	    unsigned int	partDupByte;	/* dup. bytes in part-dup. 
						 * packets */
	    unsigned int	ooPack;		/* out-of-order packets recvd */
	    unsigned int	ooByte;		/* out-of-order bytes recvd */
	    unsigned int	packAfterWin;	/* packets with data after 
						 * window */
	    unsigned int	byteAfterWin;	/* bytes rcvd after window */
	    unsigned int	afterClose;	/* packets rcvd after "close" */
	    unsigned int	winProbe;	/* rcvd window probe packets */
	    unsigned int	dupAck;		/* rcvd duplicate acks */
	    unsigned int	ackTooMuch;	/* rcvd acks for unsent data */
	    unsigned int	ackPack;	/* rcvd ack packets */
	    unsigned int	ackByte;	/* bytes acked by rcvd acks */
	    unsigned int	winUpd;		/* rcvd window update packets */
	    unsigned int	urgent;		/* #packets with urgent data.*/
	    unsigned int	urgentOnly;	/* #packets with only urgent 
						 * data.*/
	} recv;

    } tcp;

    struct {
	struct {
	    unsigned int	total;		/* # packets recv'd with an 
				 	 	 *  unknown protocol. */
	    unsigned int	accepted;	/* a client wants the packet.*/
	} recv;
	struct {
	    unsigned int	total;		/* # raw packets sent. */
	    unsigned int	dataLen;	/* #bytes sent in raw packets.*/
	} send;
    } raw;

    /*
     * Socket-related statitics.
     */
    struct {
	unsigned int	open;		/* # of socket opens. */
	unsigned int	close;		/* # of socket closes. */
	unsigned int	read;		/* # of socket reads. */
	unsigned int	write;		/* # of socket writes. */
	unsigned int	ioctl;		/* # of socket ioctls. */
	unsigned int	select;		/* # of socket select. */
	struct {
	    unsigned int append;	/* # of socket buffer appends. */
	    unsigned int appendPartial;	/* # of socket buffer appends that
					 * didn't append the full amount. */
	    unsigned int appPartBytes;	/* # of bytes partially appended. */
	    unsigned int appendFail;	/* # of socket buffer appends that
					 * failed because the buffer was full.*/
	    unsigned int remove;	/* # of socket buffer removals. */
	    unsigned int fetch;		/* # of socket buffer fetches. */
	    unsigned int copy;		/* # of socket buffer copies. */
	    unsigned int copyBytes;	/* # of bytes copied. */
	} buffer;
    } sock;

    struct {
	unsigned int dispatchLoop;	/* # of times Fs_Dispatch is called. */
	unsigned int routeCalls;	/* # of times Rte_FindOutNet is 
					 * called. */
	unsigned int routeCacheHits;	/* # of times the cache in 
					 * Rte_FindOutNet is used. */
    } misc;

} Stat_Info;

extern Stat_Info	stats;
extern int		Stat_PrintInfo();

#endif _IPS_STAT
