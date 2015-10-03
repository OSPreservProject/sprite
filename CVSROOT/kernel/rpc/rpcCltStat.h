/*
 * rpcCltStat.h --
 *
 *	Statistics are taken to trace the behavior of the RPC system.
 *	The information is recorded in a record of event counts.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RPCCLTSTAT
#define _RPCCLTSTAT

#include <user/rpc.h>

extern Rpc_CltStat rpcCltStat;
extern Rpc_CltStat rpcTotalCltStat;

extern void RpcResetCltStat _ARGS_((void));

extern void Rpc_PrintCltStat _ARGS_((void));

#ifdef notdef
extern void Rpc_EnterProcess _ARGS_((void));
extern void Rpc_LeaveProcess _ARGS_((void));
#endif notdef

#endif /* _RPCCLTSTAT */
