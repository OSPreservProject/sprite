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

#ifdef KERNEL
#include <netTypes.h>
#else
#include <kernel/netTypes.h>
#endif /* KERNEL */

/*
 * Currently this is the only data structure in this file.  It's here due
 * to conflicts between internal and exported header files.
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


#endif /* _RPC_TYPES */
