/*
 * rpcTyes.h --
 *
 *	Declarations of data structures for the rpc module.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPC_TYPES
#define _RPC_TYPES

#include <sprite.h>
#ifdef KERNEL
#include <netTypes.h>
#else
#include <kernel/netTypes.h>
#endif /* KERNEL */

/*
 * This data structure is here due to conflicts between internal and
 * exported header files.
 */
/*
 * An RPC message is composed of three parts:  the RPC control information,
 * the first data area, ``parameters'', and the second data area, ``data''.
 * A set of three buffer scatter/gather elements is used to specify
 * a complete message. A fourth part of the message is the transport
 * protocol header buffer that proceed any message.
 */
typedef struct RpcBufferSet {
    Net_ScatterGather   protoHdrBuffer;
    Net_ScatterGather   rpcHdrBuffer;
    Net_ScatterGather   paramBuffer;
    Net_ScatterGather   dataBuffer;
} RpcBufferSet;

/*
 * The form in which the user expects the server tracing info.
 */
typedef struct  RpcServerUserStateInfo {
    int         index;
    int         clientID;
    int         channel;
    int         state;
    int         num;
    Time        time;
} RpcServerUserStateInfo;

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

#endif /* _RPC_TYPES */
