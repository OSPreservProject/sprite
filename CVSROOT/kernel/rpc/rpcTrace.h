/*
 * rpcTrace.h --
 *
 *	The Trace utilities are used to keep a circular trace of packets
 *	sent and received through the RPC protocol.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCTRACE
#define _RPCTRACE

#include "rpcInt.h"
#include "rpcPacket.h"
#include "spriteTime.h"
#include "trace.h"
#include "sync.h"

/*
 * RPC_TRACE_LEN determines how big the circular buffer of trace records is.
 */
#define RPC_TRACE_LEN	100

/*
 * Converting to use general trace routines.
 */
#define RpcTrace(rpcHdrPtr, type, comment) \
	Trace_Insert(rpcTraceHdrPtr, type, (ClientData) rpcHdrPtr)

/*
 * Macros to make the trace calls take up less room and be easier
 * to change.
 */
#ifndef CLEAN
#define RPC_NIL_TRACE(type, comment) \
    if (rpc_Tracing) { \
	RpcTrace((Address)NIL, type, comment); \
    }
#define RPC_TRACE(rpcHdrPtr, type, comment) \
    if (rpc_Tracing) { \
	RpcTrace(rpcHdrPtr, type, comment); \
    }
#else
#define RPC_NIL_TRACE(type, comment)
#define RPC_TRACE(rpcHdrPtr, type, comment)
#endif

/*
 * This is the trace record written to the trace file by Rpc_DumpTrace
 */
typedef struct Rpc_Trace {
    RpcHdr rpcHdr;	/* An optional copy of the rpc Header of the packet */
    Time   time;	/* The time at which the trace record was made */
    int	   type;	/* A type field which is interpreted by the print
			 * routine.  Defined below */
} Rpc_Trace;

/*
 * The original header on the trace file.
 */
typedef struct Rpc_TraceHdr1 {
    int version;		/* == 1 */
    Time rpcDeltaTime;
    int numRecords;
} Rpc_TraceHdr1;

/*
 * A new header that also includes time stamps
 */
typedef struct Rpc_TraceHdr2 {
    int version;		/* == 2 */
    Time rpcDeltaTime;
    int numRecords;
    int emptyStampMicroseconds;
    int fullStampMicroseconds;
} Rpc_TraceHdr2;

typedef Rpc_TraceHdr2 Rpc_TraceFileHdr;

/* #define RPC_TRACE_VERSION	1	/* Jan 29 '86 */
/* #define RPC_TRACE_VERSION	2	/* Jan 31 '86 */
#define RPC_TRACE_VERSION	3	/* Nov 19 '86 */

/*
 * Tracing related defines.  These are values for the second
 * parameter of RpcTrace.  They get placed in the type field of an
 * Rpc_Trace struct.
 */
#define RPC_INPUT	1
#define RPC_OUTPUT	2
#define RPC_CLIENT_A	3
#define RPC_CLIENT_B	4
#define RPC_CLIENT_C	5
#define RPC_CLIENT_D	6
#define RPC_CLIENT_E	7
#define RPC_CLIENT_F	8
#define RPC_CLIENT_a	103
#define RPC_CLIENT_b	104
#define RPC_CLIENT_c	105
#define RPC_CLIENT_d	106
#define RPC_CLIENT_e	107
#define RPC_CLIENT_f	108
#define RPC_CLIENT_OUT	9
#define RPC_SERVER_A	10
#define RPC_SERVER_B	11
#define RPC_SERVER_C	12
#define RPC_SERVER_D	13
#define RPC_SERVER_E	14
#define RPC_SERVER_F	15
#define RPC_SERVER_a	110
#define RPC_SERVER_b	111
#define RPC_SERVER_c	112
#define RPC_SERVER_d	113
#define RPC_SERVER_e	114
#define RPC_SERVER_f	115
#define RPC_SERVER_OUT	16

#define	RPC_ETHER_OUT	17

extern Trace_Header *rpcTraceHdrPtr;
extern Boolean rpc_Tracing;

/*
 * Cached results of tests.
 */
extern Time rpcDeltaTime;

#endif /* _RPCTRACE */
