/* 
 * tcpTimer.c --
 *
 *	The file contains routines to manage the 4 TCP connection timers.
 *	The timers are described in detail in tcpTimer.h.
 *
 *	Based on 4.3BSD	@(#)tcp_timer.c	7.12 (Berkeley) 3/16/88
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/tcpTimer.c,v 1.6 89/07/23 17:35:18 nelson Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "stat.h"
#include "socket.h"
#include "tcp.h"
#include "tcpInt.h"

#include "time.h"

/*
 * Number of times per second that TimeoutHandler is called. 
 */
#ifdef SPUR
#define TIMEOUTS_PER_SEC	1
#else
#define TIMEOUTS_PER_SEC	4
#endif

int tcpKeepLen = 1;	/* must be nonzero for 4.2 compat- XXX */
int tcpKeepIdle = TCP_KEEP_TIME_IDLE;
int tcpKeepIntvl = TCP_KEEP_TIME_INTVL;
int tcpMaxIdle;

static int backoff[TCP_MAX_RXT_SHIFT+1] = {
    1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64
};

/*
 * Forward declarations.
 */
static void	TimeoutHandler();
static Boolean	ProcessTimers();



/*
 *----------------------------------------------------------------------
 *
 * TCPTimerInit --
 *
 *	Sets up the TCP timeout handler so it is called at regular
 *	intervals.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A handler will be called at regular intervals.
 *
 *----------------------------------------------------------------------
 */

void
TCPTimerInit()
{
    Time interval;
    Time_Divide(time_OneSecond, TIMEOUTS_PER_SEC, &interval);
    (void)Fs_TimeoutHandlerCreate(interval,TRUE, TimeoutHandler, (ClientData)0);
}

/*
 *----------------------------------------------------------------------
 *
 * TimeoutHandler --
 *
 *	This routine handles 2 types of timeouts:
 *	 1) the fast timeout is needed for sending delayed ACKs, and
 *	 2) the slow timeout updates the timers in all active TCB's and
 *	    causes finite state machine actions if the timers expire.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	ACKs may be sent, timers are updated.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
TimeoutHandler(data, time)
    ClientData	data;		/* Ignored. */
    Time	time;		/* Ignored. */
{
    register TCPControlBlock *tcbPtr;
    Sock_InfoPtr	sockPtr;
    register int 	i;
    Boolean		processTimers;
    static int 		count = 0;

    stats.tcp.timerCalls++;

    /*
     * The timers are processed once every second.
     */
    count++;
    if (count == TIMEOUTS_PER_SEC/TIMER_UPDATE_RATE ||
		(TIMEOUTS_PER_SEC/TIMER_UPDATE_RATE == 0)) {
	count = 0;
	processTimers = TRUE;
    } else {
	processTimers = FALSE;
    }

    /*
     * Go through all active TCP sockets and see if ACKs need to be
     * sent and if the timers should be updated.
     */
    sockPtr = (Sock_InfoPtr) NULL;
    while (TRUE) {
	sockPtr = Sock_ScanList(TCP_PROTO_INDEX, sockPtr);
	if (sockPtr == (Sock_InfoPtr) NULL) {
	    break;
	}
	tcbPtr = TCPSockToTCB(sockPtr);

	if (tcbPtr == (TCPControlBlock *) NULL) {
	    continue;
	}

	/*
	 * At every timeout, process send an ACK if it was delayed.
	 */
	if (tcbPtr->flags & TCP_DELAY_ACK) {

	    tcbPtr->flags &= ~TCP_DELAY_ACK;
	    tcbPtr->flags |= TCP_ACK_NOW;
	    stats.tcp.delayAck++;
	    if (ips_Debug) {
		fprintf(stderr, "TCP Timer: delay ack output\n");
	    }

	    (void) TCPOutput(sockPtr, tcbPtr);
	}

	if (processTimers) {

	    /*
	     *  Update active timers.
	     */

	    for (i = 0; i < TCP_NUM_TIMERS; i++) {
		if (tcbPtr->timer[i] != 0) {
		    tcbPtr->timer[i]--;
		    if (tcbPtr->timer[i] == 0) {
			if (!ProcessTimers(sockPtr, tcbPtr, i)) {
			    goto tcbGone;
			}
		    }
		}
	    }
	    tcbPtr->idle++;
	    if (tcbPtr->rtt) {
		tcbPtr->rtt++;
	    }
tcbGone:
	    tcpISS += TCP_INIT_SEND_SEQ_INCR;
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * ProcessTimers --
 *
 *	This routine is called once a second to update the 4 TCP
 *	timers. The type of action taken depends on the timer. 
 *
 * Results:
 *	TRUE	- the connection is still open and the tcb is valid.
 *	FALSE	- the connection has been closed and the tcb is no longer
 *		  valid.
 *
 * Side effects:
 *	The timer is updated. A connection may be dropped.
 *
 *----------------------------------------------------------------------
 */

static Boolean
ProcessTimers(sockPtr, tcbPtr, timer)
    Sock_InfoPtr		sockPtr;	/* TCP socket. */
    register TCPControlBlock	*tcbPtr;	/* TCB for the socket. */
    int 			timer;		/* Which timer. */
{
    register int rexmt;

    switch (timer) {

	/*
	 * 2 MSL timeout in shutdown went off.  If we're closed but
	 * still waiting for peer to close and connection has been idle
	 * too long, or if 2MSL time is up from TIME_WAIT, delete connection
	 * control block.  Otherwise, check again in a bit.
	 */
	case TCP_TIMER_2MSL:
	    stats.tcp.mslTimeout++;
	    if (tcbPtr->state != TIME_WAIT &&
		tcbPtr->idle <= tcpMaxIdle) {
		tcbPtr->timer[TCP_TIMER_2MSL] = TCP_KEEP_TIME_INTVL;
	    } else {
		TCPCloseConnection(sockPtr, tcbPtr);
		return(FALSE);
	    }
	    break;


	/*
	 * The retransmission timer went off.  Our message has not
	 * been ACKed within the retransmit interval.  Back off
	 * to a longer retransmit interval and retransmit one segment.
	 */
	case TCP_TIMER_REXMT:
	    tcbPtr->rxtshift++;
	    if (tcbPtr->rxtshift > TCP_MAX_RXT_SHIFT) {
	        tcbPtr->rxtshift = TCP_MAX_RXT_SHIFT;
		stats.tcp.timeoutDrop++;
		TCPDropConnection(sockPtr, tcbPtr, GEN_TIMEOUT);
		return(FALSE);
	    }
	    stats.tcp.rexmtTimeout++;
	    rexmt = ((tcbPtr->srtt >> 2) + tcbPtr->rttvar) >> 1;
	    rexmt *= backoff[tcbPtr->rxtshift];
	    TCP_TIMER_RANGESET(tcbPtr->rxtcur,
			rexmt, TCP_MIN_REXMT_TIME, TCP_MAX_REXMT_TIME);
	    tcbPtr->timer[TCP_TIMER_REXMT] = tcbPtr->rxtcur;
	    /*
	     * If losing, let the lower level know and try for
	     * a better route.  Also, if we backed off this far,
	     * our smooth rtt estimate is probably bogus.  Clobber it
	     * so we'll take the next rtt measurement as our smooth rtt;
	     * move the current smooth rtt into rttvar to keep the current
	     * retransmit times until then.
	     */
	    if (tcbPtr->rxtshift > TCP_MAX_RXT_SHIFT / 4) {
		Sock_BadRoute(sockPtr);
		tcbPtr->rttvar += (tcbPtr->srtt >> 2);
		tcbPtr->srtt = 0;
	    }
	    tcbPtr->send.next = tcbPtr->send.unAck;

	    /*
	     * If timing a segment in this window, and we have already 
	     * gotten some timing estimate, stop the timer.
	     */
	    tcbPtr->rtt = 0;

	    /*
	     * Close the congestion window down to one segment
	     * (we'll open it by one segment for each ack we get).
	     * Since we probably have a window's worth of unacked
	     * data accumulated, this "slow start" keeps us from
	     * dumping all that data as back-to-back packets (which
	     * might overwhelm an intermediate gateway).
	     *
	     * There are two phases to the opening: Initially we
	     * open by one mss on each ack.  This makes the window
	     * size increase exponentially with time.  If the
	     * window is larger than the path can handle, this
	     * exponential growth results in dropped packet(s)
	     * almost immediately.  To get more time between 
	     * drops but still "push" the network to take advantage
	     * of improving conditions, we switch from exponential
	     * to linear window opening at some threshhold size.
	     * For a threshhold, we use half the current window
	     * size, truncated to a multiple of the mss.
	     *
	     * (the minimum cwnd that will give us exponential
	     * growth is 2 mss.  We don't allow the threshhold
	     * to go below this.)
	     */
	    {
		unsigned int win; 
		win = MIN(tcbPtr->send.window, tcbPtr->send.congWindow) / 2 / 
				tcbPtr->maxSegSize;
		if (win < 2) {
		    win = 2;
		}
		tcbPtr->send.congWindow = tcbPtr->maxSegSize;
		tcbPtr->send.cwSizeThresh = win * tcbPtr->maxSegSize;
	    }
	    if (ips_Debug) {
		fprintf(stderr, "TCP Timer: rexmt output\n");
	    }
	    (void) TCPOutput(sockPtr, tcbPtr);
	    break;


	/*
	 * The persist timer has expired: see of the window is still closed.
	 * Force a byte to be output, if possible.
	 */
	case TCP_TIMER_PERSIST:
	    stats.tcp.persistTimeout++;
	    TCPSetPersist(tcbPtr);
	    tcbPtr->force = 1;
	    if (ips_Debug) {
		fprintf(stderr, "TCP Timer: persist output\n");
	    }
	    (void) TCPOutput(sockPtr, tcbPtr);
	    tcbPtr->force = 0;
	    break;

	/*
	 * Keep-alive timer went off: send something or drop the 
	 * connection if it has been idle for too long.
	 */
	case TCP_TIMER_KEEP_ALIVE:
	    stats.tcp.keepTimeout++;
	    if (TCP_UNSYNCHRONIZED(tcbPtr->state)) {
		stats.tcp.keepDrops++;
		TCPDropConnection(sockPtr, tcbPtr, GEN_TIMEOUT);
		return(FALSE);
	    }
	    if (Sock_IsOptionSet(sockPtr, NET_OPT_KEEP_ALIVE) &&
		(tcbPtr->state == ESTABLISHED || tcbPtr->state == CLOSE_WAIT)) {

		if (tcbPtr->idle >= tcpKeepIdle + tcpMaxIdle) {
    		    stats.tcp.keepDrops++;
		    TCPDropConnection(sockPtr, tcbPtr, GEN_TIMEOUT);
		    return(FALSE);
		}
		/*
		 * Send a packet designed to force a response if the peer
		 * is up and reachable:  either an ACK if the connection
		 * is still alive, or an RESET if the peer has closed the
		 * connection due to timeout or reboot.  Using sequence
		 * number tcbPtr->send.unAck-1 causes the transmitted
		 * zero-length segment to lie outside the receive window;
		 * by the protocol spec, this requires the correspondent
		 * TCP to respond.
		 */
		stats.tcp.keepProbe++;
		TCPRespond(sockPtr, tcbPtr->templatePtr, 
			tcbPtr->IPTemplatePtr, tcbPtr->recv.next - tcpKeepLen, 
			tcbPtr->send.unAck - 1, 0);
		tcbPtr->timer[TCP_TIMER_KEEP_ALIVE] = tcpKeepIntvl;
	    } else {
		tcbPtr->timer[TCP_TIMER_KEEP_ALIVE] = tcpKeepIdle;
	    }
	    break;

    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * TCPSetPersist --
 *
 *	Starts or restarts the persistance timer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer is updated.
 *
 *----------------------------------------------------------------------
 */

void
TCPSetPersist(tcbPtr)
    register TCPControlBlock *tcbPtr;
{
    register int t = ((tcbPtr->srtt >> 2) + tcbPtr->rttvar) >> 1;

    if (tcbPtr->timer[TCP_TIMER_REXMT] != 0) {
	 panic("TCPSetPersist REXMT timer != 0 (%d)\n",
		tcbPtr->timer[TCP_TIMER_REXMT]);
    }

    TCP_TIMER_RANGESET(tcbPtr->timer[TCP_TIMER_PERSIST],
	t * backoff[tcbPtr->rxtshift],
	TCP_MIN_PERSIST_TIME, TCP_MAX_PERSIST_TIME);

    if (tcbPtr->rxtshift < TCP_MAX_RXT_SHIFT) {
	tcbPtr->rxtshift++;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TCPCancelTimers --
 *
 *	Cancel all timers for a TCP control block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timers are cancelled.
 *
 *----------------------------------------------------------------------
 */

void
TCPCancelTimers(tcbPtr)
    register TCPControlBlock *tcbPtr;
{
    register int i;

    for (i = 0; i < TCP_NUM_TIMERS; i++) {
	tcbPtr->timer[i] = 0;
    }
}
