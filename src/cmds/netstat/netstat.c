/*
 * sysStat.c --
 *
 *	Print collected by the kernel's net module.
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
 */

#ifndef lint
static char rcsid[] = "$Header: netStat.c,v 1.3 88/02/21 15:57:08 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sysStats.h"
#include <stdio.h>
#include "option.h"
#include "kernel/net.h"

Net_EtherStats	etherStats;

void PrintEtherStats();


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

    status = Sys_Stats(SYS_NET_ETHER_STATS, 0, (Address) &etherStats);
    if (status != SUCCESS) {
	printf(stderr, "Error %x returned from Sys_Stats.\n", status);
	    Stat_PrintMsg(status, "");
	    exit(1);
    }

    PrintEtherStats();
    exit(status);
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
