/*
 * rpcInt.h --
 *
 *	Internal declarations for the RPC module.  This has data
 *	structures that are common to both the client and server
 *	parts of the RPC system.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCINT
#define _RPCINT

#include "rpcPacket.h"

/*
 * A general On/Off switch set via the Sys_Stats SYS_RPC_ENABLE_SERVICE command.
 */
extern Boolean rpcServiceEnabled;

/*
 * A boot ID for this host.  This is initialized one time when we boot.
 * It is included in the header of out-going messages, and
 * other machines detect that we have re-booted when this changes.
 */
extern unsigned int rpcBootID;

/*
 * An RPC message is composed of three parts:  the RPC control information,
 * the first data area, ``parameters'', and the second data area, ``data''.
 * A set of three buffer scatter/gather elements is used to specify
 * a complete message.
 */
typedef struct RpcBufferSet {
    Net_ScatterGather	rpcHdrBuffer;
    Net_ScatterGather	paramBuffer;
    Net_ScatterGather	dataBuffer;
} RpcBufferSet;

/*
 * The servers keep preallocated buffer space for client requests.
 * These constants define how large these buffers are.
 */
#define RPC_MAX_PARAM_SIZE	 1024
#define RPC_MAX_DATA_SIZE	16384

/*
 * A set of masks used by the client and server dispatcher to see
 * if a fragmented message is complete.
 */
extern unsigned int rpcCompleteMask[];

extern	void	RpcScatter();
#endif _RPCINT
