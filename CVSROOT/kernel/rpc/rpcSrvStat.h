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

#include "user/rpc.h"

extern Rpc_SrvStat rpcSrvStat;
extern Rpc_SrvStat rpcTotalSrvStat;

void RpcResetSrvStat();

void Rpc_PrintSrvStat();
void Rpc_StartSrvTrace();
void Rpc_EndSrvTrace();

#endif _RPCSRVSTAT
