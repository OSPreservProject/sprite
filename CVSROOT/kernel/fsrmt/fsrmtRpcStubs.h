/*
 * fsRpcStubs.h --
 *
 *	Procedure headers for file system rpcs.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FS_RPC_STUBS
#define _FS_RPC_STUBS

#include <rpc.h>

extern ReturnStatus Fsrmt_RpcGetAttrPath _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcSetAttrPath _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcGetAttr _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcSetAttr _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcGetIOAttr _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcSetIOAttr _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcDevOpen _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcPrefix _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcOpen _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcReopen _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcClose _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcRemove _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcMakeDir _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcMakeDev _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_Rpc2Path _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcRead _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcWrite _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcSelectStub _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcIOControl _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcBlockCopy _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcDomainInfo _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));
extern ReturnStatus Fsrmt_RpcMigrateStream _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));


#endif _FS_RPC_STUBS
