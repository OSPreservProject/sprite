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

#include "user/rpc.h"

extern Rpc_CltStat rpcCltStat;
extern Rpc_CltStat rpcTotalCltStat;

void RpcResetCltStat();

void Rpc_PrintCltStat();
void Rpc_EnterProcess();
void Rpc_LeaveProcess();

#endif /* _RPCCLTSTAT */
