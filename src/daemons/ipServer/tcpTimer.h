/*
 * tcpTimer.h --
 *
 *	Definitions of timers for the TCP protocol.
 *
 *	Based on 4.3BSD @(#)tcp_timer.h	7.5 (Berkeley) 3/16/88
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
 * $Header: /sprite/src/daemons/ipServer/RCS/tcpTimer.h,v 1.5 89/08/15 19:55:52 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_TCP_TIMER
#define _IPS_TCP_TIMER
/*
 * Definitions of the 4 TCP timers:
 *
 * The TCP_TIMER_REXMT timer is used to force retransmissions.  The TCB
 * has the TCP_TIMER_REXMT timer set whenever segments have been sent for
 * which ACKs are expected but not yet received.  If an ACK is received
 * that advances tcbPtr->send.unAck, then the retransmit timer is cleared (if
 * there are no more outstanding segments) or reset to the base value (if
 * more ACKs are expected).  Whenever the retransmit timer goes off,
 * we retransmit one unacknowledged segment, and do a backoff on the
 * retransmit timer.
 *
 * The TCP_TIMER_PERSIST timer is used to keep window size information
 * flowing even if the window goes shut.  If all previous transmissions
 * have been acknowledged (so that there are no retransmissions in
 * progress), and the window is too small to bother sending anything, then
 * we start the TCP_TIMER_PERSIST timer.  When it expires, if the window
 * is nonzero, we go into the transmit state.  Otherwise, at intervals, send a
 * single byte into the peer's window to force him to update our window
 * information.  We do this at most as TCP_MIN_PERSIST_TIME time
 * intervals, but no more frequently than the current estimate of
 * round-trip packet time.  The TCP_TIMER_PERSIST timer is cleared
 * whenever we receive a window update from the peer.
 *
 * The TCP_TIMER_KEEP_ALIVE timer is used to keep connections alive.  If a
 * connection is idle (no segments received) for TCP_KEEP_TIME_INIT amount
 * of time, but not yet established, then we drop the connection.  Once
 * the connection is established, if the connection is idle for
 * TCP_KEEP_TIMER_IDLE time (and keepalives have been enabled on the
 * socket), we begin to probe the connection.  We force the peer to send
 * us a segment by sending:
 *      <SEQ=SEND.UNACK-1><ACK=RECV.NEXT><CTL=ACK>
 * This segment is (deliberately) outside the window, and should elicit an
 * ack segment in response from the peer.  If, despite the
 * TCP_TIMER_KEEP_ALIVE initiated segments we cannot elicit a response
 * from a peer after TCP_KEEP_COUNT times, then we drop the connection.
 */

#define	TCP_NUM_TIMERS		4

#define	TCP_TIMER_REXMT		0		/* retransmit */
#define	TCP_TIMER_PERSIST	1		/* retransmit persistence */
#define	TCP_TIMER_KEEP_ALIVE	2		/* keep alive */
#define	TCP_TIMER_2MSL		3		/* 2*msl quiet time timer */

#ifdef	TCP_TIMER_NAMES
char *tcpTimers[] =
    { "REXMT", "PERSIST", "KEEP ALIVE", "2MSL" };
#endif

/*
 * Number of times per second to update timers.
 */

#define	TIMER_UPDATE_RATE	2
/*
 * Time constants: (In timer ticks)
 *
 * TCP_MSL_TIME		- maximum segment lifetime.
 * TCP_SRTT_BASE_TIME	- base roundtrip time; if 0, no idea yet.
 * TCP_SRTT_DEFLT_TIME	- assumed RTT if no info.
 * TCP_KEEP_TIME_INIT	- initial connect keep-alive time.
 * TCP_KEEP_TIME_IDLE	- default time before probing.
 * TCP_KEEP_TIME_INTVL	- default proble interval.
 * TCP_MIN_PERSIST_TIME	- retransmit persistence.
 * TCP_MAX_PERSIST_TIME	- maximum retransmit persistence.
 * TCP_MIN_REXMT_TIME	- minimum allowable value for retransmit timer.
 * TCP_MAX_REXMT_TIME	- maximum allowable value for retrans. and persist
 *			  timers.
 *
 * Other constants:
 * TCP_MAX_RXT_SHIFT	- maximum number of retransmissions.
 * TCP_KEEP_COUNT	- max. # of probes before we drop the connection.
 */

#define	TCP_MSL_TIME		(30 * TIMER_UPDATE_RATE)
#define	TCP_SRTT_BASE_TIME	 (0 * TIMER_UPDATE_RATE)
#define	TCP_SRTT_DEFLT_TIME	 (3 * TIMER_UPDATE_RATE)

#define	TCP_KEEP_TIME_INIT	(75 * TIMER_UPDATE_RATE)
#define	TCP_KEEP_TIME_IDLE	((120*60) * TIMER_UPDATE_RATE)
#define	TCP_KEEP_TIME_INTVL	(75 * TIMER_UPDATE_RATE)

#define	TCP_MIN_PERSIST_TIME	 (5* TIMER_UPDATE_RATE)
#define	TCP_MAX_PERSIST_TIME	(60 * TIMER_UPDATE_RATE)

#define	TCP_MIN_REXMT_TIME	( 1 * TIMER_UPDATE_RATE)
#define	TCP_MAX_REXMT_TIME	(64 * TIMER_UPDATE_RATE)

#define	TCP_KEEP_COUNT		 8
#define	TCP_MAX_RXT_SHIFT	12

extern int tcpKeepIdle;
extern int tcpKeepIntvl;
extern int tcpMaxIdle;

/*
 * A macro to force a time value to be in a certain range.
 */
#define	TCP_TIMER_RANGESET(tv, value, tvmin, tvmax) { \
	(tv) = (value); \
	if ((tv) < (tvmin)) { \
	    (tv) = (tvmin); \
	} else if ((tv) > (tvmax)) { \
	    (tv) = (tvmax); \
	} \
}
#endif /* _IPS_TCP_TIMER */
