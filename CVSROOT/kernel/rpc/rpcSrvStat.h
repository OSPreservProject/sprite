/*
 * rpcSrvStat.h --
 *
 *      Statistics are taken to trace the behavior of the server side of
 *      the RPC system.  The information is recorded in a record of event
 *      counts.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCSRVSTAT
#define _RPCSRVSTAT

#include <user/rpc.h>

extern Rpc_SrvStat rpcSrvStat;
extern Rpc_SrvStat rpcTotalSrvStat;

extern void RpcResetSrvStat _ARGS_((void));
extern void Rpc_PrintSrvStat _ARGS_((void));

#ifdef notdef
extern void Rpc_StartSrvTrace _ARGS_((void));
extern void Rpc_EndSrvTrace _ARGS_((void));
#endif notdef

#endif /* _RPCSRVSTAT */
