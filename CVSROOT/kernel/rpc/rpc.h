/*
 * rpc.h --
 *
 *	External definitions needed by users of the RPC system.  The
 *	remote procedure call numbers are defined in rpcCall.h which
 *	is also included by this file.  The other main thing needed
 *	by users of the RPC system is the Rpc_Storage type.  This is
 *	a record of buffer references manipulated by stub procedures
 *	and passed into Rpc_Call.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPC
#define _RPC

#include <status.h>
#ifdef KERNEL
#include <net.h>
#include <sys.h>
#else
#include <kernel/net.h>
#include <kernel/sys.h>
#endif /* KERNEL */


/*
 * A client stub procedure has to set up 2 sets of 2 storage areas for an
 * RPC.  The first pair of storage areas is for the request, or input,
 * parameters of the service procedure.  The second pair is for the reply,
 * or return, parameters of the service procedure.  Both the request and
 * the reply have a "data" area and a "parameter" area.  The general
 * convention is that the parameter area is meant for flags, tokens, and
 * other small control information.  The data area is for larger chunks of
 * data like pathnames or fileblocks. Either, or both, of the areas can
 * be empty by setting the address to NIL and the size to zero.
 *
 * The function of a client stub is to arrange its input parameters into
 * two buffers for the two parts of the request.  Also, it must set up the
 * buffer space for the two parts of the reply.  The RPC system will copy
 * the reply data and parameters into these areas, and the stub can access
 * them after Rpc_Call returns.
 */

typedef struct Rpc_Storage {
    /*
     * Two areas for data sent to the server.
     */
    Address	requestParamPtr;
    int		requestParamSize;
    Address	requestDataPtr;
    int		requestDataSize;
    /*
     * Two areas for data returned from the server.
     */
    Address	replyParamPtr;
    int		replyParamSize;
    Address	replyDataPtr;
    int		replyDataSize;
} Rpc_Storage;

#ifdef KERNEL
#include <rpcCall.h>
#else
#include <kernel/rpcCall.h>
#endif


/*
 * Structure to use for the simple call back to free up memory.
 * The reply a service stub generates is held onto until the
 * next request from the client arrives.  These pointers are to
 * memory that should be free'ed - ie. the previous reply.
 * If either pointer is NIL then it isn't freed.  See Rpc_Reply.
 */

typedef struct {
    Address	paramPtr;
    Address	dataPtr;
} Rpc_ReplyMem;

/*
 * This is set up to be the Sprite Host ID used for broadcasting.
 */
#define RPC_BROADCAST_SERVER_ID	NET_BROADCAST_HOSTID

/*
 * The local host's Sprite ID is exported for convenience to the filesystem
 * which needs to know who it is relative to file servers.
 */
extern int rpc_SpriteID;

/*
 * Hooks exported so they can be set via Fs_Command...
 */
extern Boolean rpc_Tracing;
extern Boolean rpc_NoTimeouts;

/*
 * Forward declarations
 */
extern ReturnStatus Rpc_Call _ARGS_((int serverID, int command, Rpc_Storage *storagePtr));
extern void Rpc_Reply _ARGS_((ClientData srvToken, int error, register Rpc_Storage *storagePtr, int (*freeReplyProc)(ClientData freeReplyData), ClientData freeReplyData));
extern void Rpc_ErrorReply _ARGS_((ClientData srvToken, int error));
extern int Rpc_FreeMem _ARGS_((ClientData freeReplyData));
extern ReturnStatus Rpc_CreateServer _ARGS_((int *pidPtr));
extern ReturnStatus Rpc_Echo _ARGS_((int serverId, Address inputPtr, Address returnPtr, int size));
extern ReturnStatus Rpc_Ping _ARGS_((int serverId));
extern ReturnStatus Rpc_EchoTest _ARGS_((int serverId, int numEchoes, int size, Address inputPtr, Address returnPtr, Time *deltaTimePtr));
extern ReturnStatus Rpc_GetTime _ARGS_((int serverId, Time *timePtr, int *timeZoneMinutesPtr, int *timeZoneDSTPtr));
extern ReturnStatus Test_RpcStub _ARGS_((int command, Address argPtr));
extern void Rpc_Init _ARGS_((void));
extern void Rpc_Start _ARGS_((void));
extern void Rpc_MaxSizes _ARGS_((int *maxDataSizePtr, int *maxParamSizePtr));
extern void Rpc_Daemon _ARGS_((void));
extern void Rpc_Server _ARGS_((void));
extern void Rpc_Dispatch _ARGS_((Net_Interface *interPtr, int protocol, 
    Address headerPtr, Address rpcHdrAddr, int packetLength));
extern void Rpc_Timeout _ARGS_((Timer_Ticks time, ClientData data));
extern void Rpc_PrintTrace _ARGS_((int numRecords));
extern ReturnStatus Rpc_DumpTrace _ARGS_((int firstRec, int lastRec, char *fileName));
extern void Rpc_StampTest _ARGS_((void));
extern void Rpc_PrintCallCount _ARGS_((void));
extern void Rpc_PrintServiceCount _ARGS_((void));
extern ReturnStatus Rpc_GetStats _ARGS_((int command, int option, Address argPtr));
extern ReturnStatus Rpc_SendTest _ARGS_((int serverId, int numSends, int size, Address inputPtr, Time *deltaTimePtr));
extern ReturnStatus Rpc_Send _ARGS_((int serverId, Address inputPtr, int size));


#endif /* _RPC */
