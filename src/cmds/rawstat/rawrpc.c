/* 
 * rawrpc.c --
 *
 *	Print raw format RPC statistics.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/rawstat/RCS/rawrpc.c,v 1.6 90/11/29 23:09:19 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <stdio.h>
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

static ReturnStatus status;
extern int zero;

/*
 *----------------------------------------------------------------------
 *
 * PrintRawRpcCltStat --
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

PrintRawRpcCltStat()
{
    Rpc_CltStat rpcCltStat;
    Rpc_CltStat *X = &rpcCltStat;

    status = Sys_Stats(SYS_RPC_CLT_STATS, TRUE, &rpcCltStat);
    if (status != SUCCESS) {
	return;
    }
    printf("Rpc_CltStat\n");
    ZeroPrint("toClient       %8u\n", X->toClient);
    ZeroPrint("badChannel     %8u\n", X->badChannel);
    ZeroPrint("chanBusy       %8u\n", X->chanBusy);
    ZeroPrint("badId          %8u\n", X->badId);
    ZeroPrint("requests       %8u\n", X->requests);
    ZeroPrint("replies        %8u\n", X->replies);
    ZeroPrint("acks           %8u\n", X->acks);
    ZeroPrint("recvPartial    %8u\n", X->recvPartial);
    ZeroPrint("timeouts       %8u\n", X->timeouts);
    ZeroPrint("aborts         %8u\n", X->aborts);
    ZeroPrint("resends        %8u\n", X->resends);
    ZeroPrint("sentPartial    %8u\n", X->sentPartial);
    ZeroPrint("errors         %8u\n", X->errors);
    ZeroPrint("nullErrors     %8u\n", X->nullErrors);
    ZeroPrint("dupFrag        %8u\n", X->dupFrag);
    ZeroPrint("close          %8u\n", X->close);
    ZeroPrint("oldInputs      %8u\n", X->oldInputs);
    ZeroPrint("badInput       %8u\n", X->badInput);
    ZeroPrint("tooManyAcks    %8u\n", X->tooManyAcks);
    ZeroPrint("chanWaits      %8u\n", X->chanWaits);
    ZeroPrint("chanBroads     %8u\n", X->chanBroads);
    ZeroPrint("chanHits       %8u\n", X->chanHits);
    ZeroPrint("chanNew        %8u\n", X->chanNew);
    ZeroPrint("chanReuse      %8u\n", X->chanReuse);
    ZeroPrint("paramOverrun   %8u\n", X->paramOverrun);
    ZeroPrint("dataOverrun    %8u\n", X->dataOverrun);
    ZeroPrint("shorts         %8u\n", X->shorts);
    ZeroPrint("longs          %8u\n", X->longs);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintRawRpcSrvStat --
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

PrintRawRpcSrvStat()
{
    Rpc_SrvStat rpcSrvStat;
    Rpc_SrvStat *X = &rpcSrvStat;

    status = Sys_Stats(SYS_RPC_SRV_STATS, TRUE, &rpcSrvStat);
    if (status != SUCCESS) {
	return;
    }
    printf("Rpc_SrvStat\n");
    ZeroPrint("toServer       %8u\n", X->toServer);
    ZeroPrint("noAlloc        %8u\n", X->noAlloc);
    ZeroPrint("nacks          %8u\n", X->nacks);
    ZeroPrint("invClient      %8u\n", X->invClient);
    ZeroPrint("serverBusy     %8u\n", X->serverBusy);
    ZeroPrint("requests       %8u\n", X->requests);
    ZeroPrint("impAcks        %8u\n", X->impAcks);
    ZeroPrint("handoffs       %8u\n", X->handoffs);
    ZeroPrint("fragMsgs       %8u\n", X->fragMsgs);
    ZeroPrint("handoffAcks    %8u\n", X->handoffAcks);
    ZeroPrint("fragAcks       %8u\n", X->fragAcks);
    ZeroPrint("sentPartial    %8u\n", X->sentPartial);
    ZeroPrint("busyAcks       %8u\n", X->busyAcks);
    ZeroPrint("resends        %8u\n", X->resends);
    ZeroPrint("badState       %8u\n", X->badState);
    ZeroPrint("extra          %8u\n", X->extra);
    ZeroPrint("reclaims       %8u\n", X->reclaims);
    ZeroPrint("reassembly     %8u\n", X->reassembly);
    ZeroPrint("dupFrag        %8u\n", X->dupFrag);
    ZeroPrint("nonFrag        %8u\n", X->nonFrag);
    ZeroPrint("fragAborts     %8u\n", X->fragAborts);
    ZeroPrint("recvPartial    %8u\n", X->recvPartial);
    ZeroPrint("closeAcks      %8u\n", X->closeAcks);
    ZeroPrint("discards       %8u\n", X->discards);
    ZeroPrint("unknownAcks    %8u\n", X->unknownAcks);
    ZeroPrint("mostNackBufs   %8u\n", X->mostNackBuffers);
    ZeroPrint("selfNacks      %8u\n", X->selfNacks);
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
	return;
    }

    printf("Rpc Service Calls\n");
    for (call=0 ; call<=RPC_LAST_COMMAND ; call++) {
	if (zero || rpcServiceCount[call] > 0) {
	    PrintCommand(stdout, call, "%-15s");
	    printf("%8u\n", rpcServiceCount[call]);
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
	return;
    }

    printf("Rpc Client Calls\n");
    for (call=0 ; call<=RPC_LAST_COMMAND ; call++) {
	if (zero || rpcClientCalls[call] > 0) {
	    PrintCommand(stdout, call, "%-15s");
	    printf("%8u\n", rpcClientCalls[call]);
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

    entryPtr = Host_ByID(spriteID);
    if (entryPtr == (Host_Entry *)NULL) {
	sprintf(string, "%d", spriteID);
	ZeroPrint(format, string);
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
	ZeroPrint(format, entryPtr->name);
    }
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
	    string = "get_time";
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
	    string = "make_dev";
	    break;
	case RPC_FS_LINK:
	    string = "link";
	    break;
	case RPC_FS_SYM_LINK:
	    string = "link";
	    break;
	case RPC_FS_GET_ATTR:
	    string = "get_attrID";
	    break;
	case RPC_FS_SET_ATTR:
	    string = "set_attrID";
	    break;
	case RPC_FS_GET_ATTR_PATH:
	    string = "get_attr";
	    break;
	case RPC_FS_SET_ATTR_PATH:
	    string = "set_attr";
	    break;
	case RPC_FS_GET_IO_ATTR:
	    string = "getI/Oattr";
	    break;
	case RPC_FS_SET_IO_ATTR:
	    string = "setI/Oattr";
	    break;
	case RPC_FS_DEV_OPEN:
	    string = "dev_open";
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
	    string = "cnsst_rply";
	    break;
	case RPC_FS_COPY_BLOCK:
	    string = "block_copy";
	    break;
	case RPC_FS_MIGRATE:
	    string = "mig_file";
	    break;
	case RPC_FS_RELEASE:
	case RPC_FS_RELEASE_NEW:
	    string = "release";
	    break;
	case RPC_FS_REOPEN:
	    string = "reopen";
	    break;
	case RPC_FS_RECOVERY:
	    string = "recov";
	    break;
	case RPC_FS_DOMAIN_INFO:
	    string = "domain_info";
	    break;
	case RPC_PROC_MIG_COMMAND:
	    string = "mig_cmd";
	    break;
	case RPC_PROC_REMOTE_CALL:
	    string = "mig_call";
	    break;
	case RPC_PROC_REMOTE_WAIT:
	    string = "wait";
	    break;
	case RPC_PROC_GETPCB:
	    string = "getpcb";
	    break;
	case RPC_REMOTE_WAKEUP:
	    string = "wakeup";
	    break;
	case RPC_SIG_SEND:
	    string = "signal";
	    break;
	default: {
	    sprintf(buffer,"%d",command);
	    string = buffer;
	    break;
	}
    }
    fprintf(stream, format, string);
}
