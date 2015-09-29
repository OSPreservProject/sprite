/*
 * sysStat.c --
 *
 *	Print collected by the kernel's net module.
 *
 * Copyright 1988, 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/cmds/netstat/RCS/netstat.c,v 1.3 92/07/06 15:05:10 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <sysStats.h>
#include <stdio.h>
#include <option.h>
#include <kernel/net.h>

Boolean doEtherStats = FALSE;
Boolean doGenStats = TRUE;

Net_EtherStats	etherStats;
Net_GenStats	genStats;

Option optionArray[] = {
    {OPT_TRUE, "ether", (Address)&doEtherStats, "Display ethernet stats"},
    {OPT_TRUE, "generic", (Address)&doGenStats,
	 "Display generic stats (default)"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

void PrintEtherStats();
void PrintGenStats();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Driver.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */


main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status = SUCCESS;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    if (doEtherStats) {
	status = Sys_Stats(SYS_NET_ETHER_STATS, 0, (Address) &etherStats);
	if (status != SUCCESS) {
	    fprintf(stderr, "Can't get ethernet stats: %s\n",
		    Stat_GetMsg(status));
	    exit(1);
	}
	PrintEtherStats();
    }
    if (doGenStats) {
	status = Sys_Stats(SYS_NET_GEN_STATS, 0, (Address) &genStats);
	if (status != SUCCESS) {
	    fprintf(stderr, "Can't get generic stats: %s\n",
		    Stat_GetMsg(status));
	    exit(1);
	}
	PrintGenStats();
    }

    exit(0);
}



/*
 *----------------------------------------------------------------------
 *
 * PrintEtherStats --
 *
 *	Print the kernel EtherStats structure. 
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Output is generator on stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintEtherStats()
{
    register Net_EtherStats	*es = &etherStats;

    printf( "Number packets received:             %d\n",es->packetsRecvd);
    printf( "Number packets send:                 %d\n",es->packetsSent);
    printf( "Number packets Output:               %d\n",es->packetsOutput);
    printf( "Number broadcast packets received:   %d\n",es->broadRecvd);
    printf( "Number broadcast packets sent:       %d\n",es->broadSent);
    printf( "Number other packets:                %d\n",es->others);
    printf( "Number overrunError packets:         %d\n",es->overrunErrors);
    printf( "Number crcError packets:             %d\n",es->crcErrors);
    printf( "Number fcsErrors packets:             %d\n",es->fcsErrors);
    printf( "Number frameError packets:           %d\n",es->frameErrors);
    printf( "Number rangeError packets:           %d\n",es->rangeErrors);
    printf( "Number collisions:                   %d\n",es->collisions);
    printf( "Number dropped due to collisions:    %d\n",es->xmitCollisionDrop);
    printf( "Number transmit dropped:             %d\n",es->xmitPacketsDropped);
    printf( "Number received dropped:             %d\n",es->recvPacketsDropped);
    printf( "Number address matches:              %d\n",es->matches);
    printf( "Number bytes sent:		  	  %d\n",es->bytesSent);
    printf( "Number bytes received:		  %d\n",es->bytesReceived);
    printf( "Average recv packet size:		  %d\n",es->recvAvgPacketSize);
    printf( "Average recv large packet size:      %d\n",es->recvAvgLargeSize);
    printf( "Average recv small packet size:	  %d\n",es->recvAvgSmallSize);
    printf( "Average sent packet size:		  %d\n",es->sentAvgPacketSize);
    printf( "Average sent large packet size:      %d\n",es->sentAvgLargeSize);
    printf( "Average sent small packet size:	  %d\n",es->sentAvgSmallSize);

}


/*
 *----------------------------------------------------------------------
 *
 * PrintGenStats --
 *
 *	Print generic (Mach-oriented) network stats.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
PrintGenStats()
{
    printf("Number packets sent:	%d\n", genStats.packetsOut);
    printf("Number packets sent inline:	%d\n", genStats.inlinePacketsOut);
    printf("Number bytes sent:		%d\n", genStats.bytesOut);
    printf("Number packets read:	%d\n", genStats.packetsIn);
    printf("Number ignored broadcast packets: %d\n",
	   genStats.selfBroadcastIn);
    printf("Number bytes read:		%d\n", genStats.bytesIn);
}
