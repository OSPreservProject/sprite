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
#endif not lint


#include "sprite.h"

#include "mem.h"
#include "rpc.h"
#include "rpcInt.h"
#include "rpcServer.h"
#include "fsRpcStubs.h"
#include "procMigrate.h"
#include "fs.h"
#include "timer.h"
#include "sync.h"

/*
 * The service procedure switch.
 */

RpcService rpcService[RPC_LAST_COMMAND+1] = {
        RpcNull, "0",                  		/* 0 - nothing */
        RpcNull, "echo intr",			/* 1 - ECHO, interrupt level */
        RpcEcho, "echo",			/* 2 - ECHO2, server process */
        Fs_RpcOpen, "open",			/* 3 - FS_SPRITE_OPEN */
        RpcNull,  "unix name",			/* 4 - NAME unix */
        RpcNull,  "old, locate",		/* 5 - LOCATE unused */
        Fs_RpcRead, "read",			/* 6 - READ */
        Fs_RpcWrite, "write",			/* 7 - WRITE */
        Fs_RpcClose, "close",			/* 8 - CLOSE */
        RpcNull, "unix trunc",			/* 9 - TRUNC unix */
        RpcNull, "unix append",			/* 10 - APPEND unix */
        RpcNull, "unix stat",			/* 11 - STAT unix  */
        Fs_RpcRemove, "remove",			/* 12 - UNLINK */
        Fs_Rpc2Path, "rename",			/* 13 - RENAME */
        Fs_RpcMakeDir, "makeDir",		/* 14 - MKDIR */
        Fs_RpcRemove, "rmDir",			/* 15 - RMDIR */
        RpcNull, "unix chmod",			/* 16 - CHMOD */
        RpcNull, "unix chown",			/* 17 - CHOWN */
        Fs_Rpc2Path, "hard link",		/* 18 - LINK */
        RpcFsUnixPrefix, "unix prefix",		/* 19 - FS_UNIX_PREFIX */
        RpcNull, "old, pull-in",		/* 20 - PULLIN old */
        RpcNull, "unix updat",			/* 21 - UPDAT unix */
        RpcGetTime, "get time",			/* 22 - GETTIME */
        RpcNull, "unix open",			/* 23 - FS_UNIX_OPEN */
        RpcEcho, "send",			/* 24 - SEND */
	Fs_RpcPrefix, "prefix",			/* 25 - FS_SPRITE_PREFIX */
	Fs_RpcGetAttr, "get attr",		/* 26 - FS_GET_ATTR */
	Fs_RpcSetAttr, "set attr",		/* 27 - FS_SET_ATTR */
	RpcProcMigInit, "mig init",		/* 28 - PROC_MIG_INIT */
	RpcProcMigInfo, "mig info",		/* 29 - PROC_MIG_INFO */
	RpcProcRemoteCall, "rmt call",		/* 30 - PROC_REMOTE_CALL */
	Fs_RpcStartMigration, "start mig",	/* 31 - FS_START_MIGRATION */
	RpcNull, "old, wakeup",			/* 32 - FS_REMOTE_WAKEUP old */
	Fs_RpcConsist, "consist",		/* 33 - FS_CONSIST */
	Fs_RpcDevOpen, "dev open",		/* 34 - FS_DEV_OPEN */
	RpcSigMigSend, "sig mig send",		/* 35 - SIG_MIG_SEND */
	Sync_RemoteNotifyStub, "rmt notify",	/* 36 - REMOTE_NOTIFY */
	RpcNull, "old, lock",			/* 37 - FS_LOCK  old */
	Proc_RpcRemoteWait, "remote wait",	/* 38 - PROC_REMOTE_WAIT */
	Fs_RpcSelectStub, "select",		/* 39 - FS_SELECT */
	Fs_RpcFinishMigration, "end mig",	/* 40 - FS_START_MIGRATION */
	Fs_RpcIOControl, "io control",		/* 41 - FS_RPC_IO_CONTROL */
	Fs_RpcConsistReply, "consist done",	/* 42 - FS_RPC_CONSIST_REPLY */
	Fs_RpcBlockCopy, "copy block",		/* 43 - FS_COPY_BLOCK */
	Fs_RpcMakeDev, "make dev",		/* 44 - FS_MKDEV */
	Fs_RpcGetAttrPath, "stat",		/* 45 - FS_GET_ATTR_PATH */
	Sig_RpcSend, "send signal",		/* 46 - SIG_SEND */
	Fs_RpcReopen, "reopen",			/* 47 - FS_REOPEN */
	Fs_RpcDomainInfo, "domain info",	/* 48 - FS_DOMAIN_INFO */
	Fs_RpcDevReopen, "dev reopen",		/* 49 - FS_DEV_REOPEN */
	Fs_RpcRecovery, "recover",		/* 50 - FS_RECOVERY */
	Fs_RpcRequest, "request", 		/* 51 - FS_REQUEST */
	Fs_RpcReply, "reply", 			/* 52 - FS_REPLY */
	Fs_RpcSetAttrPath, "setAttrPath",	/* 53 - FS_GET_ATTR_PATH */
	Fs_RpcGetIOAttr, "getIOAttr",		/* 54 - FS_GET_IO_ATTR */
	Fs_RpcSetIOAttr, "setIOAttr",		/* 55 - FS_SET_IO_ATTR */

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
	Mem_Free(replyMemPtr->paramPtr);
    }
    if (replyMemPtr->dataPtr != (Address) NIL) {
	Mem_Free(replyMemPtr->dataPtr);
    }
    Mem_Free((Address) replyMemPtr);
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
    Rpc_Reply(srvToken, SUCCESS, storagePtr, NIL, NIL);
}

/*
 *---------------------------------------------------------------------------
 *
 * RpcFsUnixPrefix --
 *
 *	This is the broadcast made by the Unix Server and to which
 *	we don't reply.
 *
 * Results:
 *	RPC_NO_REPLY
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

ReturnStatus
RpcFsUnixPrefix(srvToken, clientID, command, storagePtr)
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
    return(RPC_NO_REPLY);
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
	(struct timeReturn *)Mem_Alloc(sizeof(struct timeReturn));
    Timer_GetTimeOfDay(&timeReturnPtr->time, &timeReturnPtr->offset,
					 &timeReturnPtr->DST);
    storagePtr->replyParamPtr = (Address)timeReturnPtr;
    storagePtr->replyParamSize = sizeof(struct timeReturn);

    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;

    Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem, replyMemPtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcProcMigInit --
 *
 *	Handle a request to start process migration.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
RpcProcMigInit(srvToken, clientID, command, storagePtr)
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
    ReturnStatus status;

    status = Proc_AcceptMigration(clientID); 
    Rpc_Reply(srvToken, status, storagePtr, NIL, NIL);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcProcMigInfo --
 *
 *	Handle a request to transfer information for process migration.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Process state (process control block, virtual memory, or file state)
 * 	is copied onto the remote workstation (the RPC server).
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
RpcProcMigInfo(srvToken, clientID, command, storagePtr)
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
    ReturnStatus status;
    Rpc_ReplyMem	*replyMemPtr;
    Proc_MigrateReply *returnInfoPtr;

    returnInfoPtr = (Proc_MigrateReply *) Mem_Alloc(sizeof(Proc_MigrateReply));
    status = Proc_MigReceiveInfo(clientID,
            (Proc_MigrateCommand *) storagePtr->requestParamPtr,
	    storagePtr->requestDataSize,
 	    storagePtr->requestDataPtr,
	    returnInfoPtr);		 
    if (status == SUCCESS) {
	storagePtr->replyParamPtr = (Address) returnInfoPtr;
	storagePtr->replyParamSize = sizeof(Proc_MigrateReply);
	
	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) returnInfoPtr;
	replyMemPtr->dataPtr = (Address) NIL;
	Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem, replyMemPtr);
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcProcRemoteCall --
 *
 *	Handle a system call for a migrated process.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
RpcProcRemoteCall(srvToken, clientID, command, storagePtr)
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
    Address returnData = (Address) NIL;
    int returnDataLength = 0;
    ReturnStatus status;

    status = Proc_RpcRemoteCall((Proc_RemoteCall *)storagePtr->requestParamPtr,
 	    storagePtr->requestDataPtr, storagePtr->requestDataSize,
 	    &returnData, &returnDataLength);
    
    storagePtr->replyDataPtr = returnData;
    storagePtr->replyDataSize = returnDataLength;

    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = (Address) NIL;
    replyMemPtr->dataPtr = returnData;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem, replyMemPtr);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcSigMigSend --
 *
 *	Signal a migrated process.
 *
 * Results:
 *	A ReturnStatus.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
RpcSigMigSend(srvToken, clientID, command, storagePtr)
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
    ReturnStatus status;

    status = Sig_RpcMigSend(storagePtr->requestParamPtr, clientID);

    Rpc_Reply(srvToken, status, storagePtr, NIL, NIL);

    return(status);
}
