/* 
 * rpcStat.c --
 *
 *	Statistics generation, especially for the rpc module.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rpcstat/RCS/rpcstat.c,v 1.23 92/07/10 15:16:17 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysStats.h>
#include <option.h>
#include <kernel/sched.h>
#include <vmStat.h>
#include <host.h>
#include <kernel/sync.h>
#include <kernel/timer.h>
#include <kernel/rpcClient.h>
#include <kernel/rpcServer.h>
#include <kernel/rpcCltStat.h>
#include <kernel/rpcSrvStat.h>
#include <kernel/rpcTrace.h>
#include <kernel/rpcCall.h>
#include <kernel/rpcHistogram.h>

Boolean doCltStats = FALSE;
Boolean doSrvStats = FALSE;
Boolean doCltState = FALSE;
Boolean doSrvState = FALSE;
Boolean doRpcTrace = FALSE;
Boolean doCallCount = FALSE;
Boolean doSrvCount = FALSE;
Boolean doCltHist = FALSE;
Boolean doSrvHist = FALSE;
Boolean zero	 = FALSE;
int extraSpace = 1000;
Boolean	nohostdb = FALSE;

Option optionArray[] = {
    {OPT_TRUE, "trace", (Address)&doRpcTrace, "Print trace of RPCs"},
    {OPT_TRUE, "cinfo", (Address)&doCltStats, "Print client RPC statistics"},
    {OPT_TRUE, "sinfo", (Address)&doSrvStats, "Print server RPC statistics"},
    {OPT_TRUE, "chan", (Address)&doCltState, "Print client channel state"},
    {OPT_TRUE, "srvr", (Address)&doSrvState, "Print server process state"},
    {OPT_TRUE, "calls", (Address)&doCallCount, "Print number of client calls"},
    {OPT_TRUE, "rpcs", (Address)&doSrvCount, "Print number of service calls"},
    {OPT_TRUE, "chist", (Address)&doCltHist,
	 "Print histogram of (client) call times"},
    {OPT_TRUE, "shist", (Address)&doSrvHist,
	 "Print histogram of service times"},
    {OPT_TRUE, "zero", (Address)&zero, "Print zero valued stats"},
    {OPT_INT, "x", (Address)&extraSpace, "Extra malloc space to avoid crashes"},
    {OPT_TRUE, "nohostdb", (Address)&nohostdb, "Do not search the host database"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

ReturnStatus status;

/* forward references: */

static void PrintCltHist();
static void PrintSrvHist();
static void PrintHist();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Gets options and calls printing routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{

    argc = Opt_Parse(argc, argv,  optionArray, numOptions, 0);

    if (doCltStats) {
	PrintClientStats();
    }
    if (doSrvStats) {
	PrintServerStats();
    }
    if (doCltState) {
	PrintClientState();
    }
    if (doSrvState) {
	PrintServerState();
    }
    if (doCallCount) {
	PrintCallCount();
    }
    if (doSrvCount) {
	PrintSrvCount();
    }
    if (doCltHist) {
	PrintCltHist();
    }
    if (doSrvHist) {
	PrintSrvHist();
    }
    if (doRpcTrace) {
	PrintRpcTrace(argc, argv);
    }
    exit(0);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintClientStats --
 *
 *	Prints out the low-level statistics for the client side
 *	of the RPC system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintClientStats()
{
    Rpc_CltStat rpcCltStat;

    status = Sys_Stats(SYS_RPC_CLT_STATS, TRUE, &rpcCltStat);
    if (status != SUCCESS) {
	return;
    }
    printf("Rpc Client Statistics\n");
    printf("toClient   = %5d ", rpcCltStat.toClient);
    printf("badChannel  = %4d ", rpcCltStat.badChannel);
    printf("chanBusy    = %4d ", rpcCltStat.chanBusy);
    printf("badId       = %4d ", rpcCltStat.badId);
    printf("\n");
    printf("requests   = %5d ", rpcCltStat.requests);
    printf("replies    = %5d ", rpcCltStat.replies);
    printf("acks        = %4d ", rpcCltStat.acks);
    printf("recvPartial = %4d ", rpcCltStat.recvPartial);
    printf("\n");
    printf("nacks       = %4d ", rpcCltStat.nacks);
    printf("reNacks     = %4d ", rpcCltStat.reNacks);
    printf("maxNacks    = %4d ", rpcCltStat.maxNacks);
    printf("timeouts    = %4d ", rpcCltStat.timeouts);
    printf("\n");
    printf("aborts      = %4d ", rpcCltStat.aborts);
    printf("resends     = %4d ", rpcCltStat.resends);
    printf("sentPartial = %4d ", rpcCltStat.sentPartial);
    printf("errors      = %d(%d)", rpcCltStat.errors,
				       rpcCltStat.nullErrors);
    printf("\n");
    printf("dupFrag     = %4d ", rpcCltStat.dupFrag);
    printf("close       = %4d ", rpcCltStat.close);
    printf("oldInputs   = %4d ", rpcCltStat.oldInputs);
    printf("badInputs   = %4d ", rpcCltStat.oldInputs);
    printf("\n");
    printf("tooManyAcks = %4d ", rpcCltStat.tooManyAcks);
    printf("chanHits   = %5d ", rpcCltStat.chanHits);
    printf("chanNew     = %4d ", rpcCltStat.chanNew);
    printf("chanReuse   = %4d ", rpcCltStat.chanReuse);
    printf("\n");
    printf("newTrouble  = %4d ", rpcCltStat.newTrouble);
    printf("moreTrouble = %4d ", rpcCltStat.moreTrouble);
    printf("endTrouble  = %4d ", rpcCltStat.endTrouble);
    printf("noMark      = %4d ", rpcCltStat.noMark);
    printf("\n");
    printf("nackChanWait= %4d ", rpcCltStat.nackChanWait);
    printf("chanWaits   = %4d ", rpcCltStat.chanWaits);
    printf("chanBroads  = %4d ", rpcCltStat.chanBroads);
    printf("paramOverrun = %3d ", rpcCltStat.paramOverrun);
    printf("\n");
    printf("dataOverrun = %4d ", rpcCltStat.dataOverrun);
    printf("shorts      = %4d ", rpcCltStat.shorts);
    printf("longs       = %4d ", rpcCltStat.longs);
    printf("\n");
}

/*
 *----------------------------------------------------------------------
 *
 * PrintServerStats --
 *
 *	Prints out the low-level statistics for the service side
 *	of the RPC system.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintServerStats()
{
    Rpc_SrvStat rpcSrvStat;

    status = Sys_Stats(SYS_RPC_SRV_STATS, TRUE, &rpcSrvStat);
    if (status != SUCCESS) {
	return;
    }
    printf("Rpc Server Statistics\n");
    printf("toServer   = %5d ", rpcSrvStat.toServer);
    printf("noAlloc     = %4d ", rpcSrvStat.noAlloc);
    printf("invClient   = %4d ", rpcSrvStat.invClient);
    printf("\n");
    printf("nacks       = %4d ", rpcSrvStat.nacks);
    printf("mostNackBufs= %4d ", rpcSrvStat.mostNackBuffers);
    printf("selfNacks   = %4d ", rpcSrvStat.selfNacks);
    printf("\n");
    printf("serverBusy  = %4d ", rpcSrvStat.serverBusy);
    printf("requests   = %5d ", rpcSrvStat.requests);
    printf("impAcks    = %5d ", rpcSrvStat.impAcks);
    printf("handoffs   = %5d ", rpcSrvStat.handoffs);
    printf("\n");
    printf("fragMsgs   = %5d ", rpcSrvStat.fragMsgs);
    printf("handoffAcks = %4d ", rpcSrvStat.handoffAcks);
    printf("fragAcks    = %4d ", rpcSrvStat.fragAcks);
    printf("sentPartial = %4d ", rpcSrvStat.recvPartial);
    printf("\n");
    printf("busyAcks    = %4d ", rpcSrvStat.busyAcks);
    printf("resends     = %4d ", rpcSrvStat.resends);
    printf("badState    = %4d ", rpcSrvStat.badState);
    printf("extra       = %4d ", rpcSrvStat.extra);
    printf("\n");
    printf("reclaims    = %4d ", rpcSrvStat.reclaims);
    printf("reassembly = %5d ", rpcSrvStat.reassembly);
    printf("dupFrag     = %4d ", rpcSrvStat.dupFrag);
    printf("nonFrag     = %4d ", rpcSrvStat.nonFrag);
    printf("\n");
    printf("fragAborts  = %4d ", rpcSrvStat.fragAborts);
    printf("recvPartial = %4d ", rpcSrvStat.recvPartial);
    printf("closeAcks   = %4d ", rpcSrvStat.closeAcks);
    printf("discards    = %4d ", rpcSrvStat.discards);
    printf("\n");
    printf("unknownAcks = %4d ", rpcSrvStat.unknownAcks);
    printf("\n");
}

/*
 *----------------------------------------------------------------------
 *
 * PrintClientState --
 *
 *	Prints out the state of each client-side channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintClientState()
{
    register int index;
    ReturnStatus status = SUCCESS;
    RpcClientChannel *chanPtr =
	(RpcClientChannel *)malloc(sizeof(RpcClientChannel) + extraSpace);

    printf("%2s  %-10s  %-15s %-8s %s\n",
	    "I", "RPC", "Server", "Channel", "State");
    for (index=0 ; status == SUCCESS ; index++) {
	status = Sys_Stats(SYS_RPC_CLT_STATE, index, (Address)chanPtr);
	if (status != SUCCESS) {
	    break;
	}
	printf("%2d ", index);
	PrintCommand(stdout, chanPtr->requestRpcHdr.command, " %-10s ");
	PrintHostName(chanPtr->serverID, " %-15s ");
	printf("%-8d ", chanPtr->requestRpcHdr.channel);
	if (chanPtr->state == CHAN_FREE) {
	    printf("free ");
	}
	if (chanPtr->state & CHAN_BUSY) {
	    printf("busy ");
	}
	if (chanPtr->state & CHAN_WAITING) {
	    printf("wait ");
	}
	if (chanPtr->state & CHAN_INPUT) {
	    printf("input ");
	}
	if (chanPtr->state & CHAN_TIMEOUT) {
	    printf("timeout ");
	}
	printf("\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PrintServerState --
 *
 *	Prints out state of each RPC server process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintServerState()
{
    register int index;
    ReturnStatus status = SUCCESS;
    RpcServerState *srvPtr =
	(RpcServerState *)malloc(sizeof(RpcServerState) + extraSpace);

    printf("%2s  %-10s  %-15s %-8s %s\n",
	    "I", "RPC", "Client", "Channel", "State");
    for (index=0 ; status == SUCCESS ; index++) {
	status = Sys_Stats(SYS_RPC_SRV_STATE, index, (Address)srvPtr);
	if (status != SUCCESS) {
	    break;
	}
	printf("%2d ", index);
	PrintCommand(stdout, srvPtr->requestRpcHdr.command, " %-10s ");
	PrintHostName(srvPtr->clientID, " %-15s ");
	printf("%-8d ", srvPtr->channel);
	if (srvPtr->state == SRV_NOTREADY) {
	    printf("not ready");
	}
	if (srvPtr->state & SRV_FREE) {
	    printf("free ");
	}
	if (srvPtr->state & SRV_STUCK) {
	    printf("stuck ");
	}
	if (srvPtr->state & SRV_BUSY) {
	    printf("busy ");
	}
	if (srvPtr->state & SRV_WAITING) {
	    printf("wait ");
	}
	if (srvPtr->state & SRV_AGING) {
	    printf("aging (%d) ", srvPtr->age);
	}
	if (srvPtr->state & SRV_NO_REPLY) {
	    printf("no reply ");
	}
	if (srvPtr->state & SRV_FRAGMENT) {
	    printf("frag ");
	}
	printf("\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PrintSrvCount --
 *
 *	Prints out the number of RPC calls made to this (server) host
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintSrvCount()
{
    ReturnStatus status = SUCCESS;
    register int call;
    int rpcServiceCount[RPC_LAST_COMMAND+1];

    status = Sys_Stats(SYS_RPC_SRV_COUNTS, sizeof(rpcServiceCount),
					(Address)rpcServiceCount);
    if (status != SUCCESS) {
	fprintf(stderr, "Sys_Stats(SYS_RPC_SRV_COUNTS) failed <%x>\n", status);
	return;
    }

    printf("Rpc Service Calls\n");
    for (call=0 ; call<=RPC_LAST_COMMAND ; call++) {
	if (zero || rpcServiceCount[call] > 0) {
	    PrintCommand(stdout, call, "%-15s");
	    printf("%8d\n", rpcServiceCount[call]);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrintCallCount --
 *
 *	Prints out the number of RPC calls made by this (client) host
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintCallCount()
{
    ReturnStatus status = SUCCESS;
    register int call;
    int rpcClientCalls[RPC_LAST_COMMAND+1];

    status = Sys_Stats(SYS_RPC_CALL_COUNTS, sizeof(rpcClientCalls),
					(Address)rpcClientCalls);
    if (status != SUCCESS) {
	fprintf(stderr, "Sys_Stats(SYS_RPC_CALL_COUNTS) failed <%x>\n", status);
	return;
    }

    printf("Rpc Client Calls\n");
    for (call=0 ; call<=RPC_LAST_COMMAND ; call++) {
	if (zero || rpcClientCalls[call] > 0) {
	    PrintCommand(stdout, call, "%-15s");
	    printf("%8d\n", rpcClientCalls[call]);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PrintHostName --
 *
 *	Prints out the host name and trims of the internet domain suffix.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

PrintHostName(spriteID, format)
    int spriteID;
    char *format;
{
    Host_Entry *entryPtr;
    char string[64];
    char *cPtr;

    if (nohostdb) {
	/*
	 * Don't search the /etc/spritehosts host database.  This option
	 * is useful for finding consist RPCs that are hung with 
	 * /etc/spritehosts locked.  
	 */
	entryPtr = (Host_Entry *)NULL;
    } else {
	entryPtr = Host_ByID(spriteID);
    }
    if (entryPtr == (Host_Entry *)NULL) {
	sprintf(string, "%d", spriteID);
	printf(format, string);
    } else {
	for (cPtr = entryPtr->name ; *cPtr ; cPtr++) {
	    /*
	     * Strip off the domain suffix.
	     */
	    if (*cPtr == '.') {
		*cPtr = '\0';
		break;
	    }
	}
	printf(format, entryPtr->name);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRpcTrace --
 *
 *	Dump out the RPC trace.  Its a circular buffer and we'll print
 *	out time delta's for each record.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
PrintRpcTrace(argc, argv)
    int argc;
    char *argv[];
{
    register int index;		/* index of current table entry */
    Time baseTime, deltaTime;	/* Times for print out */
    FILE *output;		/* Output stream */
    register RpcHdr *rpcHdrPtr;	/* Rpc header stored in trace record */
    Address buffer;		/* Storage for trace records */
    int  bufSize;
    Trace_Record *recordPtr;
    int numRecords;

    if (argc > 1) {
	if (strcmp("on", argv[1]) == 0) {
	    status = Sys_Stats(SYS_RPC_TRACE_STATS, SYS_RPC_TRACING_ON, 0);
	    return;
	} else if (strcmp("off", argv[1]) == 0) {
	    status = Sys_Stats(SYS_RPC_TRACE_STATS, SYS_RPC_TRACING_OFF, 0);
	    return;
	} else {
	    /*
	     * argv[1] is a filename - as an argument causes
	     * the trace dump to be written to that file.
	     */
	    output = fopen(argv[1], "w");
	    if (output == (FILE *)NULL) {
		status = FAILURE;
		return;
	    }
	}
    } else {
	output = stdout;
    }
    /*
     * Get a copy of the trace table.
     */
    bufSize = sizeof(int) +
		RPC_TRACE_LEN * (sizeof(Trace_Record) + sizeof(RpcHdr));
    buffer = (Address)malloc(bufSize);
    status = Sys_Stats(SYS_RPC_TRACE_STATS, bufSize, buffer);
    if (status != SUCCESS) {
	return;
    }

    fprintf(output, "\n");
#define PRINT_HEADER() \
    fprintf(output, \
        "%8s %5s %8s %4s %6s %6s %6s %5s %5s %5s %8s\n", \
	"ID", "code", "time", "flag", "commnd", "client", \
	"server", "psize", "dsize", "doff", "fragment")
    PRINT_HEADER();

    numRecords = *(int *)buffer;
    buffer += sizeof(int);
    recordPtr = (Trace_Record *)buffer;
    rpcHdrPtr = (RpcHdr *)((int)buffer + numRecords * sizeof(Trace_Record));

    baseTime.seconds = 0;
    baseTime.microseconds = 0;
    for (index=0 ; index<numRecords ; index++, rpcHdrPtr++, recordPtr++) {
	fprintf(output, "%8x", rpcHdrPtr->ID);
	PrintType(output, recordPtr->event, " %-5s");

	Time_Subtract(recordPtr->time, baseTime, &deltaTime);
	fprintf(output, " %3d.%04d",
			   deltaTime.seconds,
			   deltaTime.microseconds / 100);
	baseTime = recordPtr->time;
	PrintFlags(output, rpcHdrPtr->flags, " %-2s");
	if (rpcHdrPtr->flags & RPC_ERROR) {
	    fprintf(output, " %8x", rpcHdrPtr->command);
	} else {
	    PrintCommand(output, rpcHdrPtr->command, " %-10s");
	}
	fprintf(output, " %4d %d %4d %d",
			   rpcHdrPtr->clientID,
			   rpcHdrPtr->channel,
			   rpcHdrPtr->serverID,
			   rpcHdrPtr->serverHint);
	fprintf(output, " %5d %5d %5d %2d %5x",
			   rpcHdrPtr->paramSize,
			   rpcHdrPtr->dataSize,
			   rpcHdrPtr->dataOffset,
			   rpcHdrPtr->numFrags,
			   rpcHdrPtr->fragMask);
	fprintf(output, "\n");
    }
    PRINT_HEADER();
}

/*
 * PrintFlags --
 *
 *	Convert from bit flags to a character string and output it.
 */
PrintFlags(stream, flags, format)
    FILE *stream;
    int flags;
    char *format;
{
    char c;
    char string[10];
    int index = 0;

    switch (flags & RPC_TYPE) {
	case RPC_REQUEST: {
	    c = 'Q';
	    break;
	}
	case RPC_NACK: {
	    c = 'N';
	    break;
	}
	case RPC_ACK: {
	    c = 'A';
	    break;
	}
	case RPC_REPLY: {
	    c = 'R';
	    break;
	}
	case RPC_ECHO: {
	    c = 'E';
	    break;
	}
	default: {
	    c = '-';
	    break;
	}
    }
    string[index] = c;
    index++;
    if (flags & RPC_PLSACK) {
	string[index] = 'p';
	index++;
    }
    if (flags & RPC_LASTFRAG) {
	string[index] = 'f';
	index++;
    }
    if (flags & RPC_CLOSE) {
	string[index] = 'c';
	index++;
    }
    if (flags & RPC_ERROR) {
	string[index] = 'e';
	index++;
    }
    
    string[index] = '\0';
    
    fprintf(stream, format, string);
}

/*
 * PrintCommand --
 *
 *	Convert from procedure ID to procedure name and output it.
 */
PrintCommand(stream, command, format)
    FILE *stream;
    int command;
    char *format;
{
    char buffer[128];
    char *string;

    switch (command) {
	case RPC_ECHO_1:
	    string = "echoIntr";
	    break;
	case RPC_ECHO_2:
	    string = "echo";
	    break;
	case RPC_SEND:
	    string = "send";
	    break;
	case RPC_RECEIVE:
	    string = "recv";
	    break;
	case RPC_GETTIME:
	    string = "get time";
	    break;
	case RPC_FS_PREFIX:
	    string = "prefix";
	    break;
	case RPC_FS_OPEN:
	    string = "open";
	    break;
	case RPC_FS_READ:
	    string = "read";
	    break;
	case RPC_FS_WRITE:
	    string = "write";
	    break;
	case RPC_FS_CLOSE:
	    string = "close";
	    break;
	case RPC_FS_UNLINK:
	    string = "remove";
	    break;
	case RPC_FS_RENAME:
	    string = "rename";
	    break;
	case RPC_FS_MKDIR:
	    string = "mkdir";
	    break;
	case RPC_FS_RMDIR:
	    string = "rmdir";
	    break;
	case RPC_FS_MKDEV:
	    string = "make dev";
	    break;
	case RPC_FS_LINK:
	    string = "link";
	    break;
	case RPC_FS_SYM_LINK:
	    string = "link";
	    break;
	case RPC_FS_GET_ATTR:
	    string = "get attrID";
	    break;
	case RPC_FS_SET_ATTR:
	    string = "set attrID";
	    break;
	case RPC_FS_GET_ATTR_PATH:
	    string = "get attr";
	    break;
	case RPC_FS_SET_ATTR_PATH:
	    string = "set attr";
	    break;
	case RPC_FS_GET_IO_ATTR:
	    string = "getI/Oattr";
	    break;
	case RPC_FS_SET_IO_ATTR:
	    string = "setI/Oattr";
	    break;
	case RPC_FS_DEV_OPEN:
	    string = "dev open";
	    break;
	case RPC_FS_SELECT:
	    string = "select";
	    break;
	case RPC_FS_IO_CONTROL:
	    string = "ioctl";
	    break;
	case RPC_FS_CONSIST:
	    string = "consist";
	    break;
	case RPC_FS_CONSIST_REPLY:
	    string = "cnsst rply";
	    break;
	case RPC_FS_COPY_BLOCK:
	    string = "block copy";
	    break;
	case RPC_FS_MIGRATE:
	    string = "mig file";
	    break;
	case RPC_FS_RELEASE:
	    string = "release";
	    break;
	case RPC_FS_REOPEN:
	    string = "reopen";
	    break;
	case RPC_FS_RECOVERY:
	    string = "recov";
	    break;
	case RPC_FS_DOMAIN_INFO:
	    string = "domain info";
	    break;
	case RPC_PROC_MIG_COMMAND:
	    string = "mig cmd";
	    break;
	case RPC_PROC_REMOTE_CALL:
	    string = "mig call";
	    break;
	case RPC_PROC_REMOTE_WAIT:
	    string = "wait";
	    break;
	case RPC_PROC_GETPCB:
	    string = "wait";
	    break;
	case RPC_REMOTE_WAKEUP:
	    string = "wakeup";
	    break;
	case RPC_SIG_SEND:
	    string = "signal";
	    break;
	case RPC_FS_RELEASE_NEW:
	    string = "release new";
	    break;
	default: {
	    sprintf(buffer,"%d",command);
	    string = buffer;
	    break;
	}
    }
    fprintf(stream, format, string);
}

/*
 * PrintType --
 *
 *	Format and print the type field of the trace record.
 */
PrintType(stream, type, format)
    FILE *stream;
    int type;
    char *format;
{
    char *string;
    char buffer[20];

    switch(type) {
	case RPC_INPUT:
	    string = "in";
	    break;
	case RPC_OUTPUT:
	    string = "out";
	    break;
	case RPC_CLIENT_a:		/* Client interrupt time stamps */
	case RPC_CLIENT_b:
	case RPC_CLIENT_c:
	case RPC_CLIENT_d:
	case RPC_CLIENT_e:
	case RPC_CLIENT_f:
	    sprintf(buffer, "Ci %c ", type - RPC_CLIENT_a + 'a');
	    string = buffer;
	    break;
	case RPC_CLIENT_A:		/* Client process level time stamps */
	case RPC_CLIENT_B:
	case RPC_CLIENT_C:
	case RPC_CLIENT_D:
	case RPC_CLIENT_E:
	case RPC_CLIENT_F:
	    sprintf(buffer, "Cp %c ", type - RPC_CLIENT_A + 'A');
	    string = buffer;
	    break;
	case RPC_SERVER_a:		/* Server interrupt time stamps */
	case RPC_SERVER_b:
	case RPC_SERVER_c:
	case RPC_SERVER_d:
	case RPC_SERVER_e:
	case RPC_SERVER_f:
	    sprintf(buffer, "Ci %c ", type - RPC_SERVER_a + 'a');
	    string = buffer;
	    break;
	case RPC_SERVER_A:		/* Server process level time stamps */
	case RPC_SERVER_B:
	case RPC_SERVER_C:
	case RPC_SERVER_D:
	case RPC_SERVER_E:
	case RPC_SERVER_F:
	    sprintf(buffer, "Cp %c ", type - RPC_SERVER_A + 'A');
	    string = buffer;
	    break;
	case RPC_CLIENT_OUT:
	    string = "Cexit";
	    break;
	case RPC_SERVER_OUT:
	    string = "Sexit";
	    break;
	default:
	    (void)sprintf(buffer,"%d",type);
	    string = buffer;
    }
    fprintf(stream, format, string);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintCltHist --
 *
 *	Print the client-side histogram numbers for each RPC command.
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
PrintCltHist()
{
    ReturnStatus status;
    int cmdNum;			/* RPC command number */
    Address buffer;		/* holds histogram struct & buckets */
    char rpcName[RPC_MAX_NAME_LENGTH];

    buffer = malloc(sizeof(Rpc_Histogram) +
		    RPC_NUM_HIST_BUCKETS * sizeof(int));

    printf("RPC CLIENT HISTOGRAMS:\n");
    for (cmdNum = 1; cmdNum <= RPC_LAST_COMMAND; ++cmdNum) {
	Rpc_GetName(cmdNum, sizeof(rpcName), rpcName);
	status = Sys_Stats(SYS_RPC_CLIENT_HIST, cmdNum, buffer);
	if (status != SUCCESS) {
	    fprintf(stderr,
		    "Couldn't get clt histogram info for %s call (%d): %s\n",
		    rpcName, cmdNum, Stat_GetMsg(status));
	    return;
	}
	printf("  %s:\t", rpcName);
	PrintHist(buffer);
    }

    free(buffer);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintSrvHist --
 *
 *	Print the server-side histogram numbers for each RPC command.
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
PrintSrvHist()
{
    ReturnStatus status;
    int cmdNum;			/* RPC command number */
    Address buffer;		/* holds histogram struct & buckets */
    char rpcName[RPC_MAX_NAME_LENGTH];

    buffer = malloc(sizeof(Rpc_Histogram) +
		    RPC_NUM_HIST_BUCKETS * sizeof(int));

    printf("RPC SERVER HISTOGRAMS:\n");
    for (cmdNum = 1; cmdNum <= RPC_LAST_COMMAND; ++cmdNum) {
	Rpc_GetName(cmdNum, sizeof(rpcName), rpcName);
	status = Sys_Stats(SYS_RPC_SERVER_HIST, cmdNum, buffer);
	if (status != SUCCESS) {
	    fprintf(stderr,
		    "Couldn't get srv histogram info for %s call (%d): %s\n",
		    rpcName, cmdNum, Stat_GetMsg(status));
	    return;
	}
	printf("  %s:\t", rpcName);
	PrintHist(buffer);
    }

    free(buffer);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintHist --
 *
 *	Print out numbers from a single histogram.
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
PrintHist(buffer)
    Address buffer;		/* histogram struct, followed by counters */
{
    Rpc_Histogram *histPtr;
    int *countPtr;
    int firstBucket;		/* number of first bucket in row */
    int numCols = 8;		/* number of columns in the table of counts */
    int column;			/* which column in the table */

    histPtr = (Rpc_Histogram *)buffer;
    countPtr = (int *)(buffer + sizeof(Rpc_Histogram));

    Time_Divide(histPtr->totalTime, histPtr->numCalls,
		&histPtr->aveTimePerCall);
    printf("%d Calls,  ave %d.%06d secs each (%d.%06d sec overhead)\n",
	   histPtr->numCalls, histPtr->aveTimePerCall.seconds,
	   histPtr->aveTimePerCall.microseconds,
	   histPtr->overheadTime.seconds,
	   histPtr->overheadTime.microseconds);

    firstBucket = 0;
    /* 
     * There are numBuckets + 1 buckets to print (the +1 is for the 
     * overflow bucket).
     */
    while (firstBucket < histPtr->numBuckets + 1) {
	for (column = 0; column < numCols; ++column) {
	    if (firstBucket + column >= histPtr->numBuckets) {
		break;
	    }
	    printf("%8d ", (firstBucket + column) * histPtr->usecPerBucket);
	}
	if (column < numCols && firstBucket + column == histPtr->numBuckets) {
	    printf("Overflow");
	    ++column;
	}
	printf("\n");
	for (column = 0; column < numCols; ++column) {
	    if (firstBucket + column >= histPtr->numBuckets) {
		break;
	    }
	    printf("%7d  ", countPtr[firstBucket+column]);
	}
	if (column < numCols && firstBucket + column == histPtr->numBuckets) {
	    printf("%7d\n", histPtr->numHighValues);
	    ++column;
	}
	printf("\n\n");
	firstBucket += column;
    }
}
