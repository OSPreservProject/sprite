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

extern	ReturnStatus	Fs_RpcPrefix();
extern	ReturnStatus	Fs_RpcOpen();
extern	ReturnStatus	Fs_RpcClose();
extern	ReturnStatus	Fs_RpcRead();
extern	ReturnStatus	Fs_RpcWrite();
extern	ReturnStatus	Fs_RpcTransferHandle();
extern	ReturnStatus	Fs_RpcGetAttr();
extern	ReturnStatus	Fs_RpcSetAttr();
extern	ReturnStatus	Fs_RpcRemove();
extern	ReturnStatus	Fs_RpcMakeDir();

extern	ReturnStatus	Fs_RpcDevOpen();
extern	ReturnStatus	Fs_RpcDevClose();
#endif _FS_RPC_STUBS
