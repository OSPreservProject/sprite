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

extern Fsrmt_RpcPrefix();
extern Fsrmt_RpcOpen();
extern Fsrmt_RpcRead();
extern Fsrmt_RpcWrite();
extern Fsrmt_RpcClose();
extern Fsrmt_RpcRemove();
extern Fsrmt_Rpc2Path();
extern Fsrmt_RpcMakeDir();
extern Fsrmt_RpcMakeDev();
extern Fs_RpcSymLink();
extern Fsrmt_RpcGetAttr();
extern Fsrmt_RpcSetAttr();
extern Fsrmt_RpcGetAttrPath();
extern Fsrmt_RpcSetAttrPath();
extern Fsrmt_RpcGetIOAttr();
extern Fsrmt_RpcSetIOAttr();
extern Fsrmt_RpcDevOpen();
extern Fsrmt_RpcSelectStub();
extern Fsrmt_RpcIOControl();
extern Fsrmt_RpcBlockCopy();
extern Fsrmt_RpcMigrateStream();
extern Fsio_RpcStreamMigClose();
extern Fsrmt_RpcReopen();
extern Fsrmt_RpcDomainInfo();
#endif _FS_RPC_STUBS
