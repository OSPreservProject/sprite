/* 
 * fddicmd.c --
 *
 *	Program for dealing with the DEC FDDI adapter.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/fddicmd/RCS/fddicmd.c,v 1.4 92/05/29 11:13:57 voelker Exp Locker: jhh $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "option.h"
#include <stdio.h>
#include <sys/file.h>
#include <fs.h>
#include <kernel/netTypes.h>
#include <net.h>
#include <netFDDI.h>
#include <dev/fddi.h>
#include <status.h>
#include <fmt.h>
#include <time.h>

char    *deviceName = "/dev/fddi";
char    *infileName = NULL;
Boolean reset = FALSE;
Boolean halt = FALSE;
Boolean debug = FALSE;
Boolean readDevice = FALSE;
Boolean printReg = FALSE;
Boolean printErrLog = FALSE;
Boolean flush = FALSE;
Boolean linkAddr = FALSE;
Boolean syslog = FALSE;
Boolean source = FALSE;
Boolean getStats = FALSE;
Boolean multipleWrite = FALSE;
Boolean getStatus = FALSE;

int     echo = NIL;
int     packetSize = 1024;
int     repeat = 100;

Option optionArray[] = {
    {OPT_DOC, NULL, NULL, "Usage: fddicmd [options]"},
    {OPT_TRUE, "reset", (Address) &reset,
	"Reset the adapter."},
    {OPT_TRUE, "halt", (Address) &halt,
	"Halt the adapter."},
    {OPT_TRUE, "flush", (Address) &flush,
	"Flush the transmit queue."},
    {OPT_TRUE, "addr", (Address) &linkAddr,
	"Link address of the adapter."},
    {OPT_TRUE, "debug", (Address) &debug,
	"Toggle the adapter debugging information."},
    {OPT_TRUE, "stats", (Address) &getStats,
	"Print adapter statistics."},
    {OPT_INT, "echo", (Address) &echo,
	"Echo the designated RPC server (by SpriteID)"},
    {OPT_INT, "size", (Address) &packetSize,
	"Size of packet to use."},
    {OPT_INT, "repeat", (Address) &repeat,
	"Number of times to echo."},
    {OPT_TRUE, "syslog", (Address) &syslog,
	 "Print output onto the syslog intead of stdout."},
    {OPT_TRUE, "reg", (Address) &printReg,
	"Print the values in the adapter registers."},
    {OPT_TRUE, "status", (Address) &getStatus,
	"Print the status of the FDDI interface."},
    {OPT_TRUE, "errlog", (Address) &printErrLog,
	"Print the useful information in the adapter error log."},
    {OPT_DOC, "\0", (Address)NULL, "These are the internal and external error codes that the adapter writes"},
    {OPT_DOC, "\0", (Address)NULL, "as it halts.  They should be zero for normal operation."},
};

int numOptions = sizeof(optionArray) / sizeof(Option);

#define CheckStatus(status) \
    if (status != SUCCESS) { \
	printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); \
    }


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Parse the arguments and execute the command given.  The
 *      commands are fairly simple...most of the work is done at
 *      the driver level.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
main(argc, argv)
    int argc;
    char *argv[];
{
    int                argsLeft;
    int                fd;
    ReturnStatus       status;
    int                size;

    argsLeft = Opt_Parse(argc, argv, optionArray, numOptions, 0);
    if (argsLeft > 2) {
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    fd = open(deviceName, O_RDONLY, 0);
    if (fd < 0) {
	printf("Can't open device \"%s\"\n", deviceName);
	perror("");
	exit(1);
    }
    /*
     * Turns on debugging output to the syslog.  This really slows
     * down the adapter because it has to print all of these
     * messages.
     */
    if (debug) {
	status = Fs_IOControl(fd, IOC_FDDI_DEBUG, 0, NULL, 0, NULL);
	CheckStatus(status);
	exit(0);
    }
    /*
     * Print out the contents of the adapter registers.  It doesn't
     * help much, unless you have the manual there to look at.
     */
    if (printReg) {
	Dev_FDDIRegContents contents;
	status = Fs_IOControl(fd, IOC_FDDI_REG_CONTENTS, 0, NULL, 
			      sizeof(Dev_FDDIRegContents), &contents);
	if (status != SUCCESS) { 
	    printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); 
	} else {
	    printf("Reset: 0x%x\tCtrlA: 0x%x\tCtrlB: 0x%x\n", 
		   contents.regReset, contents.regCtrlA, contents.regCtrlB);
	    printf("Status: 0x%x\tEvent: 0x%x\tMask: 0x%x\n", 
		   contents.regStatus, contents.regEvent, contents.regMask);
	}
	exit(0);
    }
    /*
     * Print out the status of the interface.
     */
    if (getStatus) {
	Dev_FDDIRegContents contents;
	static char *msgs[] = {"Resetting", "Unitialized", "Initialized",
	    "Running", "Maintenance", "Halted", "Undefined 1", "Undefined 2"};
	status = Fs_IOControl(fd, IOC_FDDI_REG_CONTENTS, 0, NULL, 
			      sizeof(Dev_FDDIRegContents), &contents);
	if (status != SUCCESS) { 
	    printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); 
	} else {
	    printf("Status: %-16s Link: %-16s\n", 
		msgs[(contents.regStatus & 0x0700) >> 8], 
		(contents.regStatus & 0x800) ? "Available" : "Unavailable");
	}
	exit(0);
    }
    /*
     * Prints out the external and internal error values that
     * the adapter write's into its error log as it halts.
     */
    if (printErrLog) {
	Dev_FDDIErrLog errLog;

	status = Fs_IOControl(fd, IOC_FDDI_ERR_LOG, 0, NULL, 
			      sizeof(Dev_FDDIErrLog), &errLog);
	if (status != SUCCESS) { 
	    printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); 
	} else {
	    printf("External: 0x%x\tInternal: 0x%x\n",
		   errLog.external, errLog.internal);
	}
	exit(0);
    }
    /*
     * Flushes the driver's queue of pending writes.
     */
    if (flush) {
	status = Fs_IOControl(fd, IOC_FDDI_FLUSH_XMT_Q, 0, NULL, 0, NULL);
	CheckStatus(status);
	exit(0);
    }
    /*
     * Print out the network address of the adapter.
     */
    if (linkAddr) {
	Dev_FDDILinkAddr fddiInfo;
	char             buffer[32];

	status = Fs_IOControl(fd, IOC_FDDI_ADDRESS, 0, NULL, 
			      sizeof(Dev_FDDILinkAddr), &fddiInfo);
	if (status != SUCCESS) {
	    printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); 
	} else {
	    (void)Net_EtherAddrToString(&fddiInfo.source, buffer);
	    printf("Adapter FDDI address: %s\n", buffer);
	}
	exit(0);
    }
    /*
     * Print out adapter statistics.
     */
    if (getStats) {
	Dev_FDDIStats stats;
	int           i;

	status = Fs_IOControl(fd, IOC_FDDI_STATS, 0, NULL,
			      sizeof(Dev_FDDIStats), &stats);
	if (status != SUCCESS) {
	    printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); 
	} else {
	    printf("Packets sent:\t%9d\n", stats.packetsSent);
	    printf("Packets queued:\t%9d\n", stats.packetsQueued);
	    printf("Packets dropped:\t%9d\n", stats.xmtPacketsDropped);
	    printf("Bytes sent  :\t%9d\n", stats.bytesSent);
	    printf("Packets received:\t%9d\n", 
		   stats.packetsReceived);
	    printf("Bytes received  :\t%9d\n", 
		   stats.bytesReceived);
	    for (i = 0; i < NET_FDDI_STATS_RCV_REAPED; i++) {
		printf("RCV reaped = [%d]:\t%9d\n", i + 1, 
		       stats.receiveReaped[i]);
	    }
	    for (i = 0; i < NET_FDDI_STATS_HISTO_NUM; i++) {
		printf("RCV packet size [%d-%d]:\t%9d\nXMT packet size [%d-%d]:\t%9d\n",
		       i * NET_FDDI_STATS_HISTO_SIZE,
		       ((i + 1) * NET_FDDI_STATS_HISTO_SIZE) - 1,
		       stats.receiveHistogram[i],
		       i * NET_FDDI_STATS_HISTO_SIZE,
		       ((i + 1) * NET_FDDI_STATS_HISTO_SIZE) - 1,
		       stats.transmitHistogram[i]);
	    }
		   
	}
	exit(0);
    }
    /*
     * Do an RPC Echo.
     */
    if (echo != NIL) {
	Dev_FDDIRpcEcho       rpcEcho;
	Dev_FDDIRpcEchoReturn rpcReturn;

	rpcEcho.serverID = echo;
	rpcEcho.packetSize = packetSize;
	rpcEcho.numEchoes = repeat;
	rpcEcho.printSyslog = syslog;
	status = Fs_IOControl(fd, IOC_FDDI_RPC_ECHO, sizeof(Dev_FDDIRpcEcho),
			      &rpcEcho, sizeof(Dev_FDDIRpcEchoReturn),
			      &rpcReturn);
	if (status != SUCCESS) {
	    printf(stderr, "fddicmd: Fs_IOControl return 0x%x\n", status); 
	    exit(1);
	}
	printf("Time per RPC (size %d) %d.%06d\n", packetSize,
	       rpcReturn.rpcTime.seconds, rpcReturn.rpcTime.microseconds);
	exit(0);
    }
    /*
     * Halt the adapter.
     */
    if (halt) {
	status = Fs_IOControl(fd, IOC_FDDI_HALT, 0, NULL, 0, NULL);
	CheckStatus(status);
	exit(0);
    }
    /*
     * Reset the adapter.
     */
    if (reset) {
	status = Fs_IOControl(fd, IOC_FDDI_RESET, 0, NULL, 0, NULL);
	CheckStatus(status);
	exit(0);
    }
}

