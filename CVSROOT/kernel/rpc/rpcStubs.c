/*
 * rpcStubs.c --
 *
 *	The stub procedures for the Rpc service procedures.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"

#include "mem.h"
#include "rpc.h"
#include "rpcInt.h"
#include "rpcServer.h"
#include "fsRpcStubs.h"
#include "fs.h"
#include "timer.h"
#include "sync.h"

/*
 * The service procedure switch.
 */

RpcService rpcService[RPC_LAST_COMMAND+1] = {
        RpcNull, "0",                  		/* 0 - nothing */
        RpcNull, "echo intr",			/* 1 - ECHO1, interrupt level */
        RpcEcho, "echo",			/* 2 - ECHO2, server process */
	RpcEcho, "send",			/* 3 - SEND, server process */
        RpcNull,  "receive",			/* 4 - RECEIVE, unimplemented */
	Fs_RpcOpen, "open",			/* 5 - OPEN */
        Fs_RpcRead, "read",			/* 6 - READ */
        Fs_RpcWrite, "write",			/* 7 - WRITE */
        Fs_RpcClose, "close",			/* 8 - CLOSE */
        Fs_RpcRemove, "remove",			/* 9 - UNLINK */
        Fs_Rpc2Path, "rename",			/* 10 - RENAME */
        Fs_RpcMakeDir, "makeDir",		/* 11 - MKDIR */
        Fs_RpcRemove, "rmDir",			/* 12 - RMDIR */
        Fs_Rpc2Path, "hard link",		/* 13 - LINK */
        RpcGetTime, "get time",			/* 14 - GETTIME */
	Fs_RpcPrefix, "prefix",			/* 15 - FS_PREFIX */
	Fs_RpcGetAttr, "get attr",		/* 16 - FS_GET_ATTR */
	Fs_RpcSetAttr, "set attr",		/* 17 - FS_SET_ATTR */
	Fs_RpcGetAttrPath, "stat",		/* 18 - FS_GET_ATTR_PATH */
	Fs_RpcSetAttrPath, "setAttrPath",	/* 19 - FS_SET_ATTR_PATH */
	Fs_RpcGetIOAttr, "getIOAttr",		/* 20 - FS_GET_IO_ATTR */
	Fs_RpcSetIOAttr, "setIOAttr",		/* 21 - FS_SET_IO_ATTR */
	Proc_RpcMigInit, "mig init",		/* 22 - PROC_MIG_INIT */
	Proc_RpcMigInfo, "mig info",		/* 23 - PROC_MIG_INFO */
	Proc_RpcRemoteCall, "rmt call",		/* 24 - PROC_REMOTE_CALL */
	Fs_RpcStartMigration, "migrate",	/* 25 - FS_MIGRATE */
	Fs_RpcConsist, "consist",		/* 26 - FS_CONSIST */
	Fs_RpcDevOpen, "dev open",		/* 27 - FS_DEV_OPEN */
	Sync_RemoteNotifyStub, "rmt notify",	/* 28 - REMOTE_WAKEUP */
	Proc_RpcRemoteWait, "remote wait",	/* 29 - PROC_REMOTE_WAIT */
	Fs_RpcSelectStub, "select",		/* 30 - FS_SELECT */
	Fs_RpcIOControl, "io control",		/* 31 - FS_RPC_IO_CONTROL */
	Fs_RpcConsistReply, "consist done",	/* 32 - FS_RPC_CONSIST_REPLY */
	Fs_RpcBlockCopy, "copy block",		/* 33 - FS_COPY_BLOCK */
	Fs_RpcMakeDev, "make dev",		/* 34 - FS_MKDEV */
	Sig_RpcSend, "send signal",		/* 35 - SIG_SEND */
	Fs_RpcReopen, "reopen",			/* 36 - FS_REOPEN */
	Fs_RpcDomainInfo, "domain info",	/* 37 - FS_DOMAIN_INFO */
	Fs_RpcDevReopen, "dev reopen",		/* 38 - FS_DEV_REOPEN */
	Fs_RpcRecovery, "recover",		/* 39 - FS_RECOVERY */
	Proc_RpcGetPCB, "get PCB",		/* 40 - PROC_GETPCB */
};


/*
 *----------------------------------------------------------------------
 *
 * Rpc_FreeMem --
 *
 *	Free the memory that was allocated for a reply.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *----------------------------------------------------------------------
 */
int
Rpc_FreeMem(replyMemPtr)
    Rpc_ReplyMem	*replyMemPtr;
{
    if (replyMemPtr->paramPtr != (Address) NIL) {
	free(replyMemPtr->paramPtr);
    }
    if (replyMemPtr->dataPtr != (Address) NIL) {
	free(replyMemPtr->dataPtr);
    }
    free((Address) replyMemPtr);
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcNull --
 *
 *	The stub for the null procedure call.
 *
 * Results:
 *	Always return the error code RPC_INVALID_RPC.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
RpcNull(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;	/* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    return(RPC_INVALID_RPC);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcEcho --
 *
 *	Service an echo request.  The input data is simply turned around
 *	to the client.  This type of RPC is used for benchmarks, and
 *	by hosts to query the status of other hosts (pinging).
 *
 * Results:
 *	SUCCESS usually, except if rpcServiceEnabled is off, when
 *	RPC_SERVICE_DISABLED is returned.
 *
 * Side effects:
 *	The echo.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
RpcEcho(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;	/* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    if (command == RPC_ECHO_2) {
	/*
	 * The data is stored in buffers specified by the buffer set for
	 * the request message.  The correct length of the two parts is
	 * computed by the dispatcher and saved in the "actual" size fields.
	 */
	storagePtr->replyParamPtr  = storagePtr->requestParamPtr;
	storagePtr->replyParamSize = storagePtr->requestParamSize;
	storagePtr->replyDataPtr   = storagePtr->requestDataPtr;
	storagePtr->replyDataSize  = storagePtr->requestDataSize;
    } else {
	/*
	 * RPC_SEND has a null reply already set up by Rpc_Server.
	 */
    }
    Rpc_Reply(srvToken, SUCCESS, storagePtr, (int(*)()) NIL, (ClientData) NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcGetTime --
 *
 *	Return the time of day.  The RPC_GET_TIME is done at boot time
 *	by machines and used to initialize their rpcBootID, as well as
 *	set their clock.  The point of the rpcBootID is to have a different
 *	one each time a host boots so others can detect a reboot.  The
 *	time obtained with this is usually overridden, however, with
 *	an 'rdate' done by the bootcmds (much) later in the boot sequence.
 *
 * Results:
 *	If SUCCESS is returned then reply has been sent.  Otherwise caller
 *	will send the error reply.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
RpcGetTime(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;	/* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    Rpc_ReplyMem	*replyMemPtr;
    struct timeReturn {
	Time time;
	int offset;
	Boolean DST;
    } *timeReturnPtr;
    
    timeReturnPtr =
	(struct timeReturn *)malloc(sizeof(struct timeReturn));
    Timer_GetTimeOfDay(&timeReturnPtr->time, &timeReturnPtr->offset,
					 &timeReturnPtr->DST);
    storagePtr->replyParamPtr = (Address)timeReturnPtr;
    storagePtr->replyParamSize = sizeof(struct timeReturn);

    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;

    Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem,
	    (ClientData) replyMemPtr);
    return(SUCCESS);
}

