/*
 * fsUnixStubs.h --
 *
 *	These are the Unix compatibility stubs for the Fs module.
 *
 * Copyright (C) 1991 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FS_UNIX_STUBS
#define _FS_UNIX_STUBS

#include <sprite.h>
#include <user/sys/socket.h>
#include <user/net/if.h>
#include <user/netinet/in.h>
#include <user/sys/stat.h>
#include <user/sys/time.h>
#include <user/spriteTime.h>
#include <fs.h>

extern int Fs_NewReadStub _ARGS_((int streamID, char *buffer, int numBytes));
extern int Fs_NewWriteStub _ARGS_((int streamID, char *buffer, int numBytes));
extern int Fs_NewOpenStub _ARGS_((char *pathName, int unixFlags,
	int permissions));
extern int Fs_NewCloseStub _ARGS_((int streamID));
extern int Fs_CreatStub _ARGS_((char *pathName, int permissions));
extern int Fs_ReadvStub _ARGS_((int streamID, register struct iovec *iov,
	int iovcnt));
extern int Fs_WritevStub _ARGS_((int streamID, register struct iovec *iov,
	int iovcnt));
extern int Fs_LinkStub _ARGS_((char *fileName, char *linkName));
extern int Fs_UnlinkStub _ARGS_((char *pathName));
extern int Fs_ChdirStub _ARGS_((char *pathName));
extern int Fs_ChmodStub _ARGS_((char *path, int mode));
extern int Fs_FchmodStub _ARGS_((int fd, int mode));
extern int Fs_ChownStub _ARGS_((char *path, int owner, int group));
extern int Fs_FchownStub _ARGS_((int fd, int owner, int group));
extern int Fs_LseekStub _ARGS_((int streamID, long offset, int whence));
extern int Fs_AccessStub _ARGS_((char *pathName, int mode));
extern int Fs_FsyncStub _ARGS_((int fd));
extern int Fs_StatStub _ARGS_((char *pathName, struct stat *attrsBufPtr));
extern int Fs_LstatStub _ARGS_((char *pathName, struct stat *attrsBufPtr));
extern int Fs_FstatStub _ARGS_((int streamID, struct stat *attrsBufPtr));
extern int Fs_DupStub _ARGS_((int oldStreamID));
extern int Fs_Dup2Stub _ARGS_((int oldStreamID, int newStreamID));
extern int Fs_PipeStub _ARGS_((void));
extern int Fs_SymlinkStub _ARGS_((char *target, char *link));
extern int Fs_ReadlinkStub _ARGS_((char *linkName, char *buffer, int bufSize));
extern int Fs_IoctlStub _ARGS_((int streamID, int request, char *buf));
extern int Fs_IoctlInt _ARGS_((int streamID, int command, int inBufSize,
        Address inBuf, int outBufSize, Address outBuf, Fs_IOReply *reply,
        int *err));
extern int Fs_NewRenameStub _ARGS_((char *pathName, char *newName));
extern int Fs_FtruncateStub _ARGS_((int streamID, unsigned long length));
extern int Fs_TruncateStub _ARGS_((char *path, unsigned long length));
extern int Fs_FlockStub _ARGS_((int streamID, int operation));
extern int Fs_MkdirStub _ARGS_((char *pathName, int permissions));
extern int Fs_RmdirStub _ARGS_((char *pathName));
extern int Fs_UtimesStub _ARGS_((char *pathName, struct timeval *tvp));
extern int Fs_SyncStub _ARGS_((void));
extern int Fs_GetdtablesizeStub _ARGS_((void));
extern int Fs_GetdoptStub _ARGS_((void));
extern int Fs_SetdoptStub _ARGS_((void));
extern int Fs_FcntlStub _ARGS_((int fd, int cmd, int arg));
extern int Fs_SocketStub _ARGS_((int domain, int type, int protocol));
extern ReturnStatus Wait _ARGS_((int socketID, Boolean readSelect,
	Time *timeOutPtr));
extern int Fs_ConnectStub _ARGS_((int socketID, struct sockaddr *namePtr,
	int nameLen));
extern int Fs_SocketpairStub _ARGS_((int d, int type, int protocol, int sv[2]));
extern int Fs_GetdentsStub _ARGS_((int fd, char *buf, int nbytes));
extern int Fs_GetdirentriesStub _ARGS_((int fd, char *buf, int nbytes,
	long *basep));
extern int Fs_AcceptStub _ARGS_((int socketID, struct sockaddr_in *addrPtr,
	int *addrLenPtr));
extern int Fs_BindStub _ARGS_((int socketID, struct sockaddr *namePtr,
	int nameLen));
extern int Fs_ConnectStub _ARGS_((int socketID, struct sockaddr *namePtr,
	int nameLen));
extern int Fs_GetpeernameStub _ARGS_((int socketID, struct sockaddr *namePtr,
	int *nameLenPtr));
extern int Fs_GetsocknameStub _ARGS_((int socketID, struct sockaddr *namePtr,
	int *nameLenPtr));
extern int Fs_GetsockoptStub _ARGS_((int socketID, int level, int optName,
	char *optVal, int *optLenPtr));
extern int Fs_SetsockoptStub _ARGS_((int socketID, int level, int optName,
	char *optVal, int optLen));
extern ReturnStatus Fs_ListenStub _ARGS_((int socketID, int backlog));
extern int Fs_RecvStub _ARGS_((int socketID, char *bufPtr, int bufSize,
	int flags));
extern int Fs_RecvfromStub _ARGS_((int socketID, char *bufPtr, int bufSize,
	int flags, struct sockaddr *senderPtr, int *senderLenPtr));
extern int Fs_RecvmsgStub _ARGS_((int socketID, struct msghdr *msgPtr,
	int flags));
extern int Fs_SendStub _ARGS_((int socketID, char *bufPtr, int bufSize,
	int flags));
extern int Fs_SendtoStub _ARGS_((int socketID, char *bufPtr, int bufSize, int flags, struct sockaddr *destPtr, int destLen));
extern int Fs_SendmsgStub _ARGS_((int socketID, struct msghdr *msgPtr, int flags));
extern ReturnStatus Fs_ShutdownStub _ARGS_((int socketID, int action));
int Fs_NewOpenStubInt _ARGS_((char *pathName, int unixFlags,
	int permissions));
extern ReturnStatus Fs_SelectStub _ARGS_ ((int numStreams,
    Time *userTimeoutPtr, int *userReadMaskPtr, int *userWriteMaskPtr,
    int *userExceptMaskPtr, int *numReadyPtr));
extern ReturnStatus Fs_Select _ARGS_ ((int numStreams, Time *timeoutPtr,
    int *inReadMaskPtr, int *outReadMaskPtr, int *inWriteMaskPtr,
    int *outWriteMaskPtr, int *inExceptMaskPtr, int *outExceptMaskPtr,
    int *numReadyPtr, int *doTimeoutPtr));
extern int Fs_NewSelectStub _ARGS_((int numStreams, int *userReadMaskPtr,
	int *userWriteMaskPtr, int *userExceptMaskPtr, Time *userTimeoutPtr));
extern ReturnStatus Fs_Select _ARGS_((int numStreams, Time *timeoutPtr,
	int *inReadMaskPtr, int *outReadMaskPtr, int *inWriteMaskPtr,
	int *outWriteMaskPtr, int *inExceptMaskPtr, int *outExceptMaskPtr,
	int *numReadyPtr, int *doTimeoutPtr));

#endif /* _FS_UNIX_STUBS */
