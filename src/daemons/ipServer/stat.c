/* 
 * stat.c --
 *	
 *	Routines to print statistics about the IP server.
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
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/stat.c,v 1.5 89/04/10 16:36:03 mgbaker Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "ipServer.h"
#include "stat.h"

#include "time.h"
#include "proc.h"
#include "dev/net.h"

Stat_Info	stats;

static Proc_ResUsage	lastUsage;
static void ComputeDiff();
static char statDumpFile[100];


/*
 *----------------------------------------------------------------------
 *
 * Stat_Command --
 *
 *	Interprets IOC_NET_STATS command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Statistics may be updated or printed.
 *
 *----------------------------------------------------------------------
 */

extern char myHostName[];

int
Stat_Command(command)
    unsigned int command;
{

    switch (command & 0xFFFF) { 
	case NET_STATS_RESET:
	    bzero((Address) &stats,sizeof(stats));
#ifndef KERNEL
	    Sys_GetTimeOfDay(&stats.startTime, (int *) NULL, (Boolean *) NULL);
	    Proc_GetResUsage(PROC_MY_PID, &lastUsage);
#endif
	    break;

	case NET_STATS_DUMP:
#ifndef KERNEL
	    (void) sprintf(statDumpFile, 
			"/tmp/ipServer.%s.%d", myHostName, command >> 16);
#endif
	    Stat_PrintInfo(-1, -1);
	    break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Stat_PrintInfo --
 *
 *	Prints a summary of the statistics collected so far.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Statistics are printed on the standard error stream or to a file.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
int
Stat_PrintInfo(sigNum, sigCode)
    int		sigNum;		/* > 0 if called by signal handler. */
    int		sigCode;	/* used as stat file version #. */
{
    Proc_ResUsage	usage;
    Proc_ResUsage	diff;
    Time		curTime;
    int			localOffset;
    char		timeString[TIME_CVT_BUF_SIZE];
#ifndef KERNEL
    extern unsigned int fsNumTimeoutEvents;
    extern unsigned int fsNumStreamEvents;
    FILE		*stream;
    if (sigNum == -1) {
	stream = fopen(statDumpFile, "w");
    } else {
	stream = stderr;
    }
#else
    int stream = 0;
#endif

    Sys_GetTimeOfDay(&curTime, &localOffset, (Boolean *) NULL);
#ifndef sun4
    {
	char	*Version();
	(void) fprintf(stream, "IP Server stats:  %s\n\n", (char *) Version());
    }
#endif sun4

    Time_ToAscii(stats.startTime.seconds + 60 * (localOffset+60), 
		FALSE, timeString);
    (void) fprintf(stream, "Started at:   %s\n", timeString);

    Time_ToAscii(curTime.seconds + 60 * (localOffset+60), FALSE, timeString);
    (void) fprintf(stream, "Current time: %s\n", timeString);

    Time_Subtract(curTime, stats.startTime, &curTime);
    Time_ToAscii(curTime.seconds, TRUE, timeString);
    (void) fprintf(stream, "Difference:   %s\n", timeString);

#ifndef KERNEL
    Proc_GetResUsage(PROC_MY_PID, &usage);
    ComputeDiff(&usage, &lastUsage, &diff);
    (void) fprintf(stream, "\nResource Usage:\n");
	(void) fprintf(stream, " %16s %16s %11s %11s\n",
	    "kernel CPU", "user CPU", "invol.cs", "vol.cs");
	(void) fprintf(stream, " %9d.%06d %9d.%06d %11d %11d\n",
	    diff.kernelCpuUsage.seconds,
	    diff.kernelCpuUsage.microseconds,
	    diff.userCpuUsage.seconds,
	    diff.userCpuUsage.microseconds,
	    diff.numQuantumEnds,
	    diff.numWaitEvents);
#endif
    (void) fprintf(stream, "Socket:\n");
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "open","close", "read", "write", "ioctl", "select");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.sock.open,
	    stats.sock.close,
	    stats.sock.read,
	    stats.sock.write,
	    stats.sock.ioctl,
	    stats.sock.select);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "append", "app.part", "part.bytes", "app.fail", "remove", "fetch");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.sock.buffer.append,
	    stats.sock.buffer.appendPartial,
	    stats.sock.buffer.appPartBytes,
	    stats.sock.buffer.appendFail,
	    stats.sock.buffer.remove,
	    stats.sock.buffer.fetch);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "copies", "bytes copied", "rte $hits %", "dispatches", 
	    "timeouts", "streams");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.sock.buffer.copy,
	    stats.sock.buffer.copyBytes,
	    (stats.misc.routeCalls != 0 ? 
		(100*stats.misc.routeCacheHits)/stats.misc.routeCalls : 0),
	    stats.misc.dispatchLoop,
#ifndef KERNEL
	    fsNumTimeoutEvents,
	    fsNumStreamEvents);
#else
	    0, 0);
#endif


    (void) fprintf(stream, "\n");

    (void) fprintf(stream, "TCP:\n");
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "send: total", "data bytes", "data pack","ack only", "win probe", 
	    "win update");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.tcp.send.total,
	    stats.tcp.send.byte,
	    stats.tcp.send.pack,
	    stats.tcp.send.acks,
	    stats.tcp.send.probe,
	    stats.tcp.send.winUpdate);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "recv: total","bad", "data: ok","dupl.", "part dupl.", "bad order");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.tcp.recv.total,
	    stats.tcp.recv.badChecksum +
		stats.tcp.recv.badOffset +
		stats.tcp.recv.shortLen,
	    stats.tcp.recv.pack,
	    stats.tcp.recv.dupPack,
	    stats.tcp.recv.partDupPack,
	    stats.tcp.recv.ooPack);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "data bytes", "data > win", "after close", "win probe", 
	    "win update","");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.tcp.recv.byte,
	    stats.tcp.recv.packAfterWin,
	    stats.tcp.recv.afterClose,
	    stats.tcp.recv.winProbe,
	    stats.tcp.recv.winUpd,
	    0);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "acks", "dupl. ack", "ack > win", "urgent", "urg only","");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.tcp.recv.ackPack,
	    stats.tcp.recv.dupAck,
	    stats.tcp.recv.ackTooMuch,
	    stats.tcp.recv.urgent,
	    stats.tcp.recv.urgentOnly,
	    0);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "timer: tot.", "delay ack","retrans", "retr.drop", "persist", 
	    "2*msl");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.tcp.timerCalls,
	    stats.tcp.delayAck,
	    stats.tcp.rexmtTimeout,
	    stats.tcp.timeoutDrop,
	    stats.tcp.persistTimeout,
	    stats.tcp.mslTimeout);
	(void) fprintf(stream, " %11s %11s %11s\n",
	    "total keeps", "dropped", "probes");
	(void) fprintf(stream, " %11d %11d %11d\n",
	    stats.tcp.keepTimeout,
	    stats.tcp.keepDrops,
	    stats.tcp.keepProbe);

    (void) fprintf(stream, "\n");
    (void) fprintf(stream, "UDP:\n");
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "recv: total", "accepts", "deamons", "bad", "#bytes", "acc.bytes");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.udp.recv.total,
	    stats.udp.recv.accepted,
	    stats.udp.recv.daemon,
	    stats.udp.recv.shortLen+
		stats.udp.recv.badChecksum,
	    stats.udp.recv.dataLen,
	    stats.udp.recv.acceptLen);
	(void) fprintf(stream, " %11s %11s\n",
	    "send: total", "#bytes");
	(void) fprintf(stream, " %11d %11d\n",
	    stats.udp.send.total,
	    stats.udp.send.dataLen);

    (void) fprintf(stream, "\n");
    (void) fprintf(stream, "ICMP:\n");
	(void) fprintf(stream, " %11s %11s %11s %11s %11s\n",
	    "recv: total", "short", "bad sum", "bad type", "bad code");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d\n",
	    stats.icmp.total, 
	    stats.icmp.shortLen, 
	    stats.icmp.badChecksum, 
	    stats.icmp.badType, 
	    stats.icmp.badCode);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s\n",
	  "unreachable", "redirect", "src quench", "time exceed", "param prob");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d\n",
	    stats.icmp.inHistogram[NET_ICMP_UNREACHABLE],
	    stats.icmp.inHistogram[NET_ICMP_REDIRECT],
	    stats.icmp.inHistogram[NET_ICMP_SOURCE_QUENCH],
	    stats.icmp.inHistogram[NET_ICMP_TIME_EXCEED],
	    stats.icmp.inHistogram[NET_ICMP_PARAM_PROB]);
	(void) fprintf(stream, " %11s %11s %11s %11s\n",
	    "echo", "timestamp", "info", "mask");
	(void) fprintf(stream, " %11d %11d %11d %11d\n",
	    stats.icmp.inHistogram[NET_ICMP_ECHO],
	    stats.icmp.inHistogram[NET_ICMP_TIMESTAMP],
	    stats.icmp.inHistogram[NET_ICMP_INFO_REQ],
	    stats.icmp.inHistogram[NET_ICMP_MASK_REQ]);

    (void) fprintf(stream, "\n");
    (void) fprintf(stream, "IP:\n");
	(void) fprintf(stream, " %11s %11s %11s %11s %11s\n",
	    "recv: total", "frags","short", "bad sum", "not for us");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d\n",
	    stats.ip.totalRcv,
	    stats.ip.fragsRcv,
	    stats.ip.shortPacket + stats.ip.shortHeader + 
		stats.ip.shortLen,
	    stats.ip.badChecksum,
	    stats.ip.forwards);
	(void) fprintf(stream, " %11s %11s %11s %11s %11s %11s\n",
	    "send: total", "whole","fragmented", "#frags", "dont frag",
	    "TO calls");
	(void) fprintf(stream, " %11d %11d %11d %11d %11d %11d\n",
	    stats.ip.wholeSent+ stats.ip.fragOnSend,
	    stats.ip.wholeSent,
	    stats.ip.fragOnSend,
	    stats.ip.fragsSent,
	    stats.ip.dontFragment,
	    stats.ip.fragTimeouts);

    (void) fprintf(stream, "\n");
    (void) fprintf(stream, "Raw:\n");
	(void) fprintf(stream, " %11s %11s %11s %11s\n",
	    "recv: total", "accepted","send: total", "#bytes");
	(void) fprintf(stream, " %11d %11d %11d %11d\n",
	    stats.raw.recv.total,
	    stats.raw.recv.accepted,
	    stats.raw.send.total,
	    stats.raw.send.dataLen);
    (void) fflush(stream);
}

static void
ComputeDiff(newPtr, oldPtr, diffPtr)
    register Proc_ResUsage	*newPtr;
    register Proc_ResUsage	*oldPtr;
    register Proc_ResUsage	*diffPtr;
{
    Time_Subtract(newPtr->kernelCpuUsage, oldPtr->kernelCpuUsage, 
			&diffPtr->kernelCpuUsage);
    Time_Subtract(newPtr->userCpuUsage, oldPtr->userCpuUsage, 
			&diffPtr->userCpuUsage);
    diffPtr->numQuantumEnds = newPtr->numQuantumEnds - oldPtr->numQuantumEnds;
    diffPtr->numWaitEvents  = newPtr->numWaitEvents  - oldPtr->numWaitEvents;
}
