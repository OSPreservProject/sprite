/*
 * rpcStubs.c --
 *
 *	The stub procedures for the Rpc service procedures.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"

#include "stdlib.h"
#include "rpc.h"
#include "rpcInt.h"
#include "rpcServer.h"
#include "fsrmtRpcStubs.h"
#include "fsconsist.h"
#include "fsutil.h"
#include "procMigrate.h"
#include "timer.h"
#include "sync.h"
#include "sig.h"
#include "fsio.h"

/*
 * The RPC service procedure switch.  This table and the arguments
 * to the RPC's themselves define the network interface to Sprite.
 * Change things carefully!  You can either add new RPCs to the end,
 * or you can change the RPC_VERSION number defined in rpcPacket.h
 * and create a new in-compatible network interface.
 */

RpcService rpcService[RPC_LAST_COMMAND+1] = {
        RpcNull, "0",                  		/* 0 - nothing */
        RpcNull, "echo intr",			/* 1 - ECHO1, interrupt level */
        RpcEcho, "echo",			/* 2 - ECHO2, server process */
	RpcEcho, "send",			/* 3 - SEND, server process */
        RpcNull,  "receive",			/* 4 - RECEIVE, unimplemented */
        RpcGetTime, "get time",			/* 5 - GETTIME */
	Fsrmt_RpcPrefix, "prefix",		/* 6 - FS_PREFIX */
	Fsrmt_RpcOpen, "open",			/* 7 - FS_OPEN */
        Fsrmt_RpcRead, "read",			/* 8 - FS_READ */
        Fsrmt_RpcWrite, "write",		/* 9 - FS_WRITE */
        Fsrmt_RpcClose, "close",		/* 10 - FS_CLOSE */
        Fsrmt_RpcRemove, "remove",		/* 11 - FS_UNLINK */
        Fsrmt_Rpc2Path, "rename",		/* 12 - FS_RENAME */
        Fsrmt_RpcMakeDir, "makeDir",		/* 13 - FS_MKDIR */
        Fsrmt_RpcRemove, "rmDir",		/* 14 - FS_RMDIR */
	Fsrmt_RpcMakeDev, "make dev",		/* 15 - FS_MKDEV */
        Fsrmt_Rpc2Path, "hard link",		/* 16 - FS_LINK */
        RpcNull, "sym link",			/* 17 - FS_SYM_LINK */
	Fsrmt_RpcGetAttr, "get attr",		/* 18 - FS_GET_ATTR */
	Fsrmt_RpcSetAttr, "set attr",		/* 19 - FS_SET_ATTR */
	Fsrmt_RpcGetAttrPath, "stat",		/* 20 - FS_GET_ATTR_PATH */
	Fsrmt_RpcSetAttrPath, "setAttrPath",	/* 21 - FS_SET_ATTR_PATH */
	Fsrmt_RpcGetIOAttr, "getIOAttr",	/* 22 - FS_GET_IO_ATTR */
	Fsrmt_RpcSetIOAttr, "setIOAttr",	/* 23 - FS_SET_IO_ATTR */
	Fsrmt_RpcDevOpen, "dev open",		/* 24 - FS_DEV_OPEN */
	Fsrmt_RpcSelectStub, "select",		/* 25 - FS_SELECT */
	Fsrmt_RpcIOControl, "io control",	/* 26 - FS_IO_CONTROL */
	Fsconsist_RpcConsist, "consist",	/* 27 - FS_CONSIST */
	Fsconsist_RpcConsistReply, "consist done",/* 28 - FS_CONSIST_REPLY */
	Fsrmt_RpcBlockCopy, "copy block",	/* 29 - FS_COPY_BLOCK */
	Fsrmt_RpcMigrateStream, "migrate",	/* 30 - FS_MIGRATE */
	Fsio_RpcStreamMigClose, "release",	/* 31 - FS_RELEASE */
	Fsrmt_RpcReopen, "reopen",		/* 32 - FS_REOPEN */
	Fsutil_RpcRecovery, "recover",		/* 33 - FS_RECOVERY */
	Fsrmt_RpcDomainInfo, "domain info",	/* 34 - FS_DOMAIN_INFO */
	Proc_RpcMigCommand, "mig command",	/* 35 - PROC_MIG_COMMAND */
	Proc_RpcRemoteCall, "rmt call",		/* 36 - PROC_REMOTE_CALL */
	Proc_RpcRemoteWait, "remote wait",	/* 37 - PROC_REMOTE_WAIT */
	Proc_RpcGetPCB, "get PCB",		/* 38 - PROC_GETPCB */
	Sync_RemoteNotifyStub, "rmt notify",	/* 39 - REMOTE_WAKEUP */
	Sig_RpcSend, "send signal",		/* 40 - SIG_SEND */
	Fsio_RpcStreamMigCloseNew, "new release",/* 41 - FS_RELEASE_NEW */
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
 *	SUCCESS.
 
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
    return (SUCCESS);
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

