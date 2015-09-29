/*
 * fsioRpc.h --
 *
 *	Declarations of RPC calls for the fsio module.
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
 * $Header: /r3/kupfer/spriteserver/src/sprited/fsio/RCS/fsioRpc.h,v 1.3 91/12/01 21:58:35 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _FSIORPC
#define _FSIORPC

#include <rpc.h>

/* procedures */

#ifdef SPRITED_MIGRATION
extern ReturnStatus Fsio_RpcStreamMigClose _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsio_RpcStreamMigCloseNew _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
#else
#define Fsio_RpcStreamMigClose	Rpc_NotImplemented
#define Fsio_RpcStreamMigCloseNew	Rpc_NotImplemented
#endif

#endif /* _FSIORPC */
