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

#include "net.h"
#include "sys.h"
#include "status.h"

#include "rpcCall.h"

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
 * Flags for the Rpc_RebootNotify.
 */
#define RPC_WHEN_HOST_DOWN		0x1
#define RPC_WHEN_HOST_REBOOTS		0x2

/*
 * Forward declarations
 */
ReturnStatus	Rpc_Call();
void		Rpc_Reply();
void		Rpc_ErrorReply();
void		Rpc_FreeMem();

ReturnStatus	Rpc_Echo();
ReturnStatus	Rpc_Ping();
ReturnStatus	Rpc_EchoTest();
ReturnStatus	Rpc_GetTime();

ReturnStatus	Test_RpcStub();

void		Rpc_Init();
void		Rpc_Start();
void		Rpc_MaxSizes();

void		Rpc_Daemon();
void		Rpc_Server();
int		Rpc_Dispatch();
void		Rpc_Timeout();

void		Rpc_PrintTrace();
ReturnStatus	Rpc_DumpTrace();
void		Rpc_StampTest();

void		Rpc_HostNotify();
int		Rpc_WaitForHost();
Boolean		Rpc_HostIsDown();

#endif _RPC
