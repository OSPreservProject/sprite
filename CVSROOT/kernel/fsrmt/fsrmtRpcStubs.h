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

extern Fs_RpcPrefix();
extern Fs_RpcOpen();
extern Fs_RpcRead();
extern Fs_RpcWrite();
extern Fs_RpcClose();
extern Fs_RpcRemove();
extern Fs_Rpc2Path();
extern Fs_RpcMakeDir();
extern Fs_RpcRemove();
extern Fs_RpcMakeDev();
extern Fs_Rpc2Path();
extern Fs_RpcSymLink();
extern Fs_RpcGetAttr();
extern Fs_RpcSetAttr();
extern Fs_RpcGetAttrPath();
extern Fs_RpcSetAttrPath();
extern Fs_RpcGetIOAttr();
extern Fs_RpcSetIOAttr();
extern Fs_RpcDevOpen();
extern Fs_RpcSelectStub();
extern Fs_RpcIOControl();
extern Fs_RpcConsist();
extern Fs_RpcConsistReply();
extern Fs_RpcBlockCopy();
extern Fs_RpcMigrateStream();
extern Fs_RpcReleaseStream();
extern Fs_RpcReopen();
extern Fs_RpcRecovery();
extern Fs_RpcDomainInfo();
#endif /* _FS_RPC_STUBS */
