/*
 * fsrecov.h --
 *
 *	Declarations of file handle recovery from recovery box.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSRECOV
#define _FSRECOV
#include <fsrecovTypes.h>
#include <fslcl.h>
#include <fsrecovDirLog.h>


/* constants */

/* procedures */
extern void Fsrecov_InitState _ARGS_((void));
extern void Fsrecov_SetupHandles _ARGS_((void));
extern ReturnStatus Fsrecov_AddHandle _ARGS_((Fs_HandleHeader *hdrPtr,
	Fs_FileID *otherIDPtr, int clientID, unsigned int useFlags,
	int clientData, Boolean addRef));
extern ReturnStatus Fsrecov_DeleteHandle _ARGS_((Fs_HandleHeader *hdrPtr, int clientID, unsigned int flags));
extern ReturnStatus Fsrecov_GetHandle _ARGS_((Fs_FileID fileID, int clientID, Fsrecov_HandleState *objectPtr, Boolean checksum));
extern ReturnStatus Fsrecov_UpdateHandle _ARGS_((Fs_FileID fileID, int clientID, Fsrecov_HandleState *objectPtr));
extern Boolean Fsrecov_ThisType _ARGS_((Fs_HandleHeader *hdrPtr, int clientID));
extern unsigned short Fsrecov_Checksum _ARGS_((register int len, Address bufPtr));
void Fsrecov_DirOpInit _ARGS_((void));
#include <fsdm.h>
ClientData Fsrecov_DirOpStart _ARGS_((int opFlags,
	Fsio_FileIOHandle *dirHandlePtr, int dirOffset, char *name, int nameLen,
	int fileNumber, int type, Fsdm_FileDescriptor *fileDescPtr));

void Fsrecov_DirOpEnd _ARGS_((int opFlags, Fsio_FileIOHandle *dirHandlePtr,
	int dirOffset, char *name, int nameLen, int fileNumber,
	int type, Fsdm_FileDescriptor *fileDescPtr,
	ClientData clientData, ReturnStatus status));
void Fsrecov_UpdateLog _ARGS_((int timeStamp));
void Fsrecov_ProcessLog _ARGS_((void));
void Fsrecov_DirOpRemove _ARGS_((ClientData clientData));
void Fsrecov_GetComponent _ARGS_((List_Links *itemPtr, char **componentPtr,
	int *lengthPtr));
ReturnStatus Fsrecov_TestCmd _ARGS_((int option, Address argPtr));
#endif /* _FSRECOV */
