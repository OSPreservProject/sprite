/*
 * fslclInt.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLCLINT
#define _FSLCLINT

#include <fscache.h>
#include <fsio.h>
#include <fsioFile.h>

#include <stdio.h>


/*
 * Image of a new directory.
 */
extern char *fslclEmptyDirBlock;


/*
 * Declarations for the Local Domain lookup operations called via
 * the switch in Fsprefix_LookupOperation.  These are called with a pathname.
 */
extern ReturnStatus FslclExport _ARGS_((Fs_HandleHeader *hdrPtr, int clientID,
		Fs_FileID *ioFileIDPtr, int *dataSizePtr, 
		ClientData *clientDataPtr));
extern ReturnStatus FslclOpen _ARGS_((Fs_HandleHeader *prefixHandlePtr,
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclLookup _ARGS_((Fs_HandleHeader *prefixHdrPtr, 
		char *relativeName, Fs_FileID *rootIDPtr, int useFlags,
		int type, int clientID, Fs_UserIDs *idPtr, int permissions, 
		int fileNumber, Fsio_FileIOHandle **handlePtrPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclGetAttrPath _ARGS_((Fs_HandleHeader *prefixHandlePtr,
		char *relativeName, Address argsPtr, Address resultsPtr,
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclSetAttrPath _ARGS_((Fs_HandleHeader *prefixHandlePtr,
		char *relativeName, Address argsPtr, Address resultsPtr,
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclMakeDevice _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr,
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclMakeDir _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclRemove _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclRemoveDir _ARGS_((Fs_HandleHeader *prefixHandle, 
		char *relativeName, Address argsPtr, Address resultsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr));
extern ReturnStatus FslclRename _ARGS_((Fs_HandleHeader *prefixHandle1, 
		char *relativeName1, Fs_HandleHeader *prefixHandle2, 
		char *relativeName2, Fs_LookupArgs *lookupArgsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr, Boolean *name1ErrorPtr));
extern ReturnStatus FslclHardLink _ARGS_((Fs_HandleHeader *prefixHandle1, 
		char *relativeName1, Fs_HandleHeader *prefixHandle2,
		char *relativeName2, Fs_LookupArgs *lookupArgsPtr, 
		Fs_RedirectInfo **newNameInfoPtrPtr, Boolean *name1ErrorPtr));

/*
 * Declarations for the Local Domain attribute operations called via
 * the fsAttrOpsTable switch.  These are called with a fileID.
 */
#ifdef SOSP91
extern ReturnStatus FslclGetAttr _ARGS_((Fs_FileID *fileIDPtr, int clientID, 
		Fs_Attributes *attrPtr, int hostID, int userID));
extern ReturnStatus FslclSetAttr _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_Attributes *attrPtr, Fs_UserIDs *idPtr, int flags, int
		clientID, int hostID, int userID));
#else
extern ReturnStatus FslclGetAttr _ARGS_((Fs_FileID *fileIDPtr, int clientID, 
		Fs_Attributes *attrPtr));
extern ReturnStatus FslclSetAttr _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_Attributes *attrPtr, Fs_UserIDs *idPtr, int flags));
#endif

extern void FslclAssignAttrs _ARGS_((Fsio_FileIOHandle *handlePtr,
		Boolean isExeced, Fs_Attributes *attrPtr));

#endif /* _FSLCLINT */
