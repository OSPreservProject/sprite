/* 
 * fsSysCall.c --
 *
 *	These are the system call stubs that get called by the MIG stub
 *	routine.  These routines gather up arguments and then call the
 *	workhorse routines to do the actual work.
 *
 * Copyright 1987, 1991 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/fs/RCS/fsSysCall.c,v 1.5 92/07/16 18:06:57 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <ckalloc.h>
#include <mach.h>
#include <sys/file.h>

#include <fs.h>
#include <fsMach.h>
#include <fsutil.h>
#include <fsutilTrace.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fslcl.h>
#include <fsdm.h>
#include <sig.h>
#include <spriteSrvServer.h>
#include <vm.h>
#include <proc.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_AttachDiskStub --
 *
 *	Stub for the Fs_AttachDisk system call.  This gets ahold of
 *	the filename arguments, opens the disk to be attached, and
 *	calls Fsdm_AttachDisk to do the work.
 *
 * Results:
 *	Returns KERN_SUCCESS.  The Sprite code will be SUCCESS or an error 
 *	code from finding junk on the disk.  Fills in the "pending signals" 
 *	flag.
 *
 * Side effects:
 *	Call Fs_AttachDisk or Fs_DetachDisk.
 *
 *----------------------------------------------------------------------
 */
    
kern_return_t
Fs_AttachDiskStub(serverPort, deviceName, devNameLength, localName,
		  localNameLength, flags, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *deviceName;		/* Name of raw disk device file to attach */
    mach_msg_type_number_t devNameLength; /* extra parameter to appease MIG */
    char *localName;		/* Local directory where disk is attached */
    mach_msg_type_number_t localNameLength; /* extra parm to appease MIG */
    int flags;		/* FS_ATTACH_READ_ONLY, FS_DETATCH, FS_ATTACH_LOCAL */
    ReturnStatus *statusPtr;	/* OUT: Sprite status */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifndef SPRITED_LOCALDISK
#ifdef lint
    statusPtr = statusPtr;
    flags = flags;
    localNameLength = localNameLength;
    localName = localName;
    devNameLength = devNameLength;
    deviceName = deviceName;
    serverPort = serverPort;
    sigPendingPtr = sigPendingPtr;
#endif
    return KERN_FAILURE;	/* temporarily disabled */
#else /* SPRITED_LOCALDISK */
    int useFlags;		/* Flags for the Fs_Open call */
    Fs_Stream *streamPtr;	/* Stream from the Fs_Open call */

#ifdef lint
    serverPort = serverPort;
    localNameLength = localNameLength;
    devNameLength = devNameLength;
#endif

    *statusPtr = SUCCESS;
    if (localNameLenght == 0) {
	localName = (char *)NIL;
    }
    if (devNameLength == 0) {
	deviceName = (char *)NIL;
    }

    if (flags & FS_DETACH) {
	/*
	 * Flush data associated with the domain and remove the domain
	 * from the prefix table.
	 */
	*statusPtr = Fsdm_DetachDisk(localName);
    } else {
	/*
	 * Attach a local domain into the prefix table.
	 */
	if (deviceName == (char *)NIL ||
	    localName == (char *)NIL) {
	    *statusPtr = FS_INVALID_ARG;
	} else {
	    if (flags & FS_ATTACH_READ_ONLY) {
		useFlags = FS_READ;
	    } else {
		useFlags = FS_READ|FS_WRITE;
	    }
	    *statusPtr = Fs_Open(deviceName, useFlags, FS_DEVICE, 0,
				 &streamPtr);
	    if (*statusPtr == SUCCESS) {
		*statusPtr = Fsdm_AttachDiskByHandle(streamPtr->ioHandlePtr, 
						     localName, flags);
	    }
	}
    }

    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
#endif /* SPRITED_LOCALDISK */
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_OpenStub --
 *
 *      This is the stub for the Fs_Open system call.  This routine sets up
 *      a Stream ID for the Fs_Stream object returned by (the internal)
 *      Fs_Open.  Stream Id's get passed to read and write calls and
 *      identify the stream to the system.  If FS_CREATE is one of the
 *      usage flags then the third argument, permissions, specifies the
 *      base permissions for the newly created file.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, the 
 *	user's new stream ID, and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_OpenStub(serverPort, pathName, pathNameLength, usageFlags, permissions,
	    statusPtr, streamIDPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* The name of the file to open */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    int usageFlags;		/* FS_READ, FS_WRITE, FS_CREATE, FS_TRUNC */
    int permissions;		/* Permission mask to use on creation */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    int *streamIDPtr;		/* OUT: This is the user's handle on the open
				 * file used in later filesystem requests */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    Fs_Stream	 	*streamPtr;
    ReturnStatus	status;

#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif
    FSUTIL_TRACE(FSUTIL_TRACE_OPEN);

    /*
     * Open the file and get a stream descriptor for it.
     */
    usageFlags &= ~FS_KERNEL_FLAGS;
    usageFlags |= (FS_USER | FS_FOLLOW);
    if (fsutil_Tracing) {
	usageFlags |= FSUTIL_TRACE_FLAG;
    }

    status = Fs_Open(pathName, usageFlags, FS_FILE,
		     permissions & 0777, &streamPtr);
    
    FSUTIL_TRACE_NAME(FSUTIL_TRACE_OPEN_DONE_2, pathName);

    /*
     * Save a pointer to the descriptor in a list kept in the proc table.
     * Its index in the list is the "Stream ID".
     */

    if (status == SUCCESS) {
	status = Fs_GetStreamID(streamPtr, streamIDPtr);
	if (status != SUCCESS) {
	    (void) Fs_Close(streamPtr);
	}
    }

    if (status == SUCCESS) {
	FSUTIL_TRACE(FSUTIL_TRACE_OPEN_DONE_3);
    }

    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CloseStub --
 *
 *	System call to release a stream Id.  Calls the internal Fs_Close
 *	routine.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and the 
 *	"pending signals" flag.
 *
 * Side effects:
 *	The streamID becomes invalid (until it gets reassigned by
 *	another Fs_Open call).
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_CloseStub(serverPort, streamID, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int streamID;
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    ReturnStatus	status;
    Fs_Stream 		*streamPtr;	/* Stream pointer from the process's
				 	 * list of open streams */
    Proc_ControlBlock	*procPtr;	/* This process's control block */
    
#ifdef lint
    serverPort = serverPort;
#endif

    /*
     * Map from the streamID to a streamPtr.
     */

    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	/*
	 * Fudge the return status.  A close() can only return EBADF or
	 * EINTR, so return something that maps to EBADF even if it
	 * doesn't make sense here.  Sprite system calls are going
	 * away soon anyway.
	 */
	if (status != GEN_EINTR) {
	    status = FS_NEW_ID_TOO_BIG;
	}
	goto bailOut;
    }

    /*
     * Look after any shared memory associated with the file.
     */
    Vm_CleanupSharedFile(procPtr, streamPtr);

    /*
     * Clear the user-level streamID and then close the underlying stream.
     */

    Fs_ClearStreamID(streamID, procPtr);
    status = Fs_Close(streamPtr);

 bailOut:
    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(procPtr);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_ReadStub --
 *
 *      The stub for the Fs_Read system call.  
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, the 
 *	count of bytes read, and the "pending signals" flag.
 *
 * Side effects:
 *	Copies the read data into the user's buffer.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_ReadStub(serverPort, streamID, amountRead, buffer, statusPtr,
	    amountReadPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int		streamID;	/* The user's index into its open file list */
    int		amountRead;	/* The amount of bytes to read */
    vm_address_t buffer;	/* The user buffer to read into */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    int		*amountReadPtr;	/* OUT: The amount of bytes actually read */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    register ReturnStatus	status;
    Fs_Stream		*streamPtr;	/* The stream to read from */
    Proc_ControlBlock 	*procPtr;	/* This process's control block */

#ifdef lint
    serverPort = serverPort;
#endif

    /*
     * Map from stream ID to file pointer and do the read.
     * If the process is not remote, Fs_Read takes care of making
     * the read buffer accessible.
     */

    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	*amountReadPtr = amountRead;
	status = Fs_Read(streamPtr, (Address)buffer, streamPtr->offset,
			 amountReadPtr);
    } else {
	*amountReadPtr = 0;
    }

    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(procPtr);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_WriteStub --
 *
 *	The Fs_Write system call stub.  
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, the 
 *	count of bytes written, and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_WriteStub(serverPort, streamID, writeLength, buffer, statusPtr,
	     writeLengthPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int streamID;		/* which file to write to */
    int writeLength;		/* how big the user's buffer is */
    vm_address_t buffer;	/* the user's buffer */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    int *writeLengthPtr;	/* OUT: number of bytes actually written */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    register ReturnStatus status;
    Fs_Stream	*streamPtr;	/* The stream to write to */
    Proc_ControlBlock *procPtr;	/* This process's control block */

#ifdef lint
    serverPort = serverPort;
#endif

    /*
     * Map from stream ID to streamPtr.  If the process is not remote,
     * Fs_Write takes care of making the user's buffer accessible.
     */
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	*writeLengthPtr = writeLength;
	status = Fs_Write(streamPtr, (Address)buffer, streamPtr->offset,
			  writeLengthPtr);
    } else {
	*writeLengthPtr = 0;
    }

    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(procPtr);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CommandStub --
 *
 *	Stub for the Fs_Command system call.  This copies in a buffer
 *	containing arguments for the particular command, and copies
 *	out whatever Fs_Command leaves in the buffer after its done.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending
 *	signals" flag.
 *
 * Side effects:
 *	Whatever Fs_Command does.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_CommandStub(serverPort, command, bufSize, buffer, statusPtr,
	       sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int command;		/* Defined in user include "fsCmd.h" */
    vm_size_t bufSize;		/* Size in bytes of buffer */
    vm_address_t buffer;	/* In/Out - Command specific data */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    ReturnStatus status = SUCCESS;
    Address localBuffer = (Address)NIL;

#ifdef lint
    serverPort = serverPort;
#endif

    if (bufSize != 0) {
	localBuffer = (Address)ckalloc(bufSize);
	status = Vm_CopyIn((int)bufSize, (Address)buffer, localBuffer);
    }

    if (status == SUCCESS) {
	status = Fs_Command(command, (int)bufSize, localBuffer);
    }

    if (localBuffer != (Address)NIL) {
	if (status == SUCCESS) {
	    status = Vm_CopyOut((int)bufSize, localBuffer, (Address)buffer);
	}
	ckfree(localBuffer);
    }
    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_MakeDeviceStub --
 *
 *	The Fs_MakeDevice system call stub.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in Sprite status code and "pending 
 *	signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_MakeDeviceStub(serverPort, pathName, pathNameLength, device,
		  permissions, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* Device file to create */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    Fs_Device device;		/* Specifies the device file attributes */
    int permissions;		/* Permissions on the special file */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_MakeDevice(pathName, &device, permissions);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_MakeDirStub --
 *
 *	The Fs_MakeDir system call stub.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in Sprite status code and "pending 
 *	signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_MakeDirStub(serverPort, pathName, pathNameLength, permissions,
	       statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* directory to create */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    int permissions;
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_MakeDir(pathName, permissions);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RemoveStub --
 *
 *	The stub for the Fs_Remove system call.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite result code from
 *	Fs_Remove and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_RemoveStub(serverPort, pathName, pathNameLength, statusPtr,
	      sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* the file to remove */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    ReturnStatus *statusPtr;	/* OUT: result code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_Remove(pathName);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RemoveDirStub --
 *
 *	The Fs_RemoveDir system call stub.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite result code from
 *	Fs_RemoveDir and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_RemoveDirStub(serverPort, pathName, pathNameLength, statusPtr,
		 sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* the directory to remove */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    ReturnStatus *statusPtr;	/* OUT: result code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_RemoveDir(pathName);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ChangeDirStub --
 *
 *	Change the current working directory for the current user process.
 *	The argument specifies a new current directory.  The argument
 *	can be a relative path name.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in a Sprite status code and "pending 
 *	signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_ChangeDirStub(serverPort, pathName, pathNameLength, statusPtr,
		 sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* new cwd */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    ReturnStatus *statusPtr;	/* OUT: result code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_ChangeDir(pathName);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_GetNewIDStub --
 *
 *	The stub for the system call that duplicates a stream ID.
 *	See Fs_GetNewID.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, the new 
 *	stream ID, and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_GetNewIDStub(serverPort, streamID, newStreamIDPtr, statusPtr,
		sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int streamID;		/* stream to duplicate */
    int *newStreamIDPtr;	/* IN: FS_ANYID or stream to close;
				 * OUT: the copy stream ID */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif

    *statusPtr = Fs_GetNewID(streamID, newStreamIDPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetAttributesStub --
 *
 *	Stub for the Fs_GetAttributes system call.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending 
 *	signals" flag.  If successful, fills in the attributes for the 
 *	named file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_GetAttributesStub(serverPort, pathName, pathNameLength, fileOrLink,
		     statusPtr, attrPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* file to get attributes for */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    int fileOrLink;
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    Fs_Attributes *attrPtr;	/* OUT: file's attributes */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_GetAttributes(pathName, fileOrLink, attrPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CheckAccess --
 *
 *	Process the Fs_CheckAccess system all.
 *
 * Results:
 *	SUCCESS if accessible.
 *	FS_NO_ACCESS if not accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_CheckAccess(pathName, perm, useRealID)
    char 	*pathName;	/* name of file to check (server addr) */
    int		perm;		/* Sprite permission flags */
    Boolean	useRealID;
{
    ReturnStatus	status;
    Fs_Attributes	attributes;
    Proc_ControlBlock	*procPtr;
    register int	bits;

    procPtr = Proc_GetEffectiveProc();

    status = Fs_GetAttributes(pathName, FS_ATTRIB_FILE, &attributes);
    if (status != SUCCESS) {
	return(status);
    } else if (perm == FS_EXISTS) {
	/*
	 * Only checking for existence, not read or write access.
	 */
	return(SUCCESS);
    } else if ((useRealID && (procPtr->userID == 0)) ||
		(!useRealID && (procPtr->effectiveUserID == 0))) {
	/*
	 * Let the super-user do anything.
	 */
	return(SUCCESS);
    }
    /*
     * Translate from FS_READ etc, into permission bits, ie. FS_WORLD_READ.
     * The world permissions are shifted left to the owner or group permissions
     * if the process's userID or one of its group IDs matches the file's.
     */
    bits = 0;
    if (perm & FS_READ) {
	bits |= FS_WORLD_READ;
    }
    if (perm & FS_WRITE) {
	bits |= FS_WORLD_WRITE;
    }
    if (perm & FS_EXECUTE) {
	bits |= FS_WORLD_EXEC;
    }

    if ((useRealID && attributes.uid == procPtr->userID) ||
        (!useRealID && attributes.uid == procPtr->effectiveUserID)) {
	bits <<= 6;
    } else {
	int	i;
	for (i = 0; i < procPtr->fsPtr->numGroupIDs; i++) {
	    if (attributes.gid == procPtr->fsPtr->groupIDs[i]) {
		bits <<= 3;
		break;
	    }
	}
    }
    if ((attributes.permissions & bits) == bits) {
	return (GEN_SUCCESS);
    } else {
	return (FS_NO_ACCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetAttributesIDStub --
 *
 *	Stub for the Fs_GetAttributesID system call.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, the 
 *	attributes of the given file, and the "pending signals" flag.
 *
 * Side effects:
 *	 None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_GetAttributesIDStub(serverPort, streamID, statusPtr, attrPtr,
		       sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int		streamID;	/* The user's index into its open file list */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    Fs_Attributes *attrPtr;	/* OUT: the file's attributes */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    register	ReturnStatus 	status;
    Fs_Stream			*streamPtr;

#ifdef lint
    serverPort = serverPort;
#endif

    status = Fs_GetStreamPtr(Proc_GetEffectiveProc(), streamID, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_GetAttrStream(streamPtr, attrPtr);
    }

    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_SetAttrStub --
 *
 *	Change attributes for the named file.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and the 
 *	"pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_SetAttrStub(serverPort, pathName, pathNameLength, fileOrLink, newAttr,
	       flags, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;		/* Name of file to manipulate */
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    int fileOrLink;		/* Whether or not to follow symbolic links */
    Fs_Attributes newAttr;	/* New values for attributes */
    int flags;			/* What attributes to set */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_SetAttributes(pathName, fileOrLink, &newAttr, flags);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetAttrIDStub --
 *
 *	Stub for Fs_SetAttrID system call: set file attributes.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending 
 *	signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_SetAttrIDStub(serverPort, streamID, newAttr, flags, statusPtr,
		 sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int streamID;		/* which file to change */
    Fs_Attributes newAttr;	/* New attributes for the file */
    int flags;			/* What attributes to set */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    register 	ReturnStatus 	status;
    Fs_Stream			*streamPtr;
    Fs_UserIDs			ids;
    Proc_ControlBlock		*procPtr;

#ifdef lint
    serverPort = serverPort;
#endif

    /*
     * Map from stream ID to file pointer and get the attributes.
     */
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	Fs_SetIDs(procPtr, &ids);
	status = Fs_SetAttrStream(streamPtr, &newAttr, &ids, flags);
    }

    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(procPtr);
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetDefPermStub --
 *
 *	Set the default permissions on files created by this process.
 *	These defaults get inherited by child processes.  ("umask" in 
 *	UNIX.)
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the old default permissions and the 
 *	"pending signals" flag.
 *
 * Side effects:
 *	Sets the default permissions mask in the filesystem related
 *	state kept in the proc table.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_SetDefPermStub(serverPort, permissions, oldPermPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int permissions;	/*  A mask of 9 permission bits.  Three sets of 3
			 * bits.  The first set is for the owner's permissions,
			 * the second for the group's, and the last is for
			 * everyone else.  The three bits indicate read,
			 * write, and execute permissions, respectively.
			 * A permission bit set in this mask ENABLES the
			 * permission on created files. */
    int *oldPermPtr;	/* OUT: the previous value of the permissions mask */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    register	Proc_ControlBlock	*procPtr;

#ifdef lint
    serverPort = serverPort;
#endif

    procPtr = Proc_GetEffectiveProc();
    *oldPermPtr = procPtr->fsPtr->filePermissions;
    procPtr->fsPtr->filePermissions = (unsigned int)(permissions & 0777);

    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_HardLinkStub --
 *
 *	Stub for the Fs_HardLink system call.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in a Sprite status code and "pending 
 *	signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_HardLinkStub(serverPort, fileName, fileNameLength, linkName,
		linkNameLength, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *fileName;	/* Name of the existing file */
    mach_msg_type_number_t fileNameLength; /* extra parameter to appease MIG */
    char *linkName;	/* New name to link to the existing file */
    mach_msg_type_number_t linkNameLength; /* extra parameter to appease MIG */
    ReturnStatus *statusPtr;	/* OUT: Sprite result code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    fileNameLength = fileNameLength;
    linkNameLength = linkNameLength;
#endif

    *statusPtr = Fs_HardLink(fileName, linkName);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_RenameStub --
 *
 *	Rename a file (or directory).
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and the 
 *	"pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_RenameStub(serverPort, pathName, pathNameLength, newName, newNameLength,
	      statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *pathName;
    mach_msg_type_number_t pathNameLength; /* extra parameter to appease MIG */
    char *newName;
    mach_msg_type_number_t newNameLength; /* extra parameter to appease MIG */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    newNameLength = newNameLength;
    pathNameLength = pathNameLength;
#endif

    *statusPtr = Fs_Rename(pathName, newName);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_SymLinkStub --
 *
 *	Create a symbolic link.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and the 
 *	"pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_SymLinkStub(serverPort, targetName, targetNameLength, linkName,
	       linkNameLength, remoteFlag, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *targetName;		/* what the link points to */
    mach_msg_type_number_t targetNameLength; /* extra parm to appease MIG */
    char *linkName;		/* name of link to create */
    mach_msg_type_number_t linkNameLength; /* extra parameter to appease MIG */
    Boolean remoteFlag;		/* make a remote link? */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    targetNameLength = targetNameLength;
    linkNameLength = linkNameLength;
#endif

    *statusPtr = Fs_SymLink(targetName, linkName, remoteFlag);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_ReadLinkStub --
 *
 *	Stub for Fs_ReadLink system call.  The named file should be a
 *	remote link or a symbolic link file.  It is opened and its
 *	contents (another file's name) are read and stored in the buffer.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite return code from the
 *	open and read, the number of bytes written into the user buffer,
 *	and the "pending signals" flag.
 *
 * Side effects:
 *	Fills in the user buffer with the contents of the link file.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_ReadLinkStub(serverPort, linkName, linkNameLength, bufSize, buffer,
		statusPtr, linkSizePtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    char *linkName;		/* Name of the link file */
    mach_msg_type_number_t linkNameLength; /* extra parameter to appease MIG */
    int bufSize;		/* Size of the user's buffer */
    vm_address_t buffer;	/* buffer to hold link's value (user addr) */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    int *linkSizePtr;		/* OUT: the amount of data in buffer */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    ReturnStatus	status;
    Fs_Stream		*streamPtr = NULL;
    char		newLinkName[FS_MAX_PATH_NAME_LENGTH];

#ifdef lint
    serverPort = serverPort;
    linkNameLength = linkNameLength;
#endif

    status = Fs_Open(linkName, FS_READ | FS_USER,
		     FS_SYMBOLIC_LINK|FS_REMOTE_LINK, 0, &streamPtr);
    if (status != SUCCESS) {
	goto bailOut;
    }

    status = Fs_Read(streamPtr, (Address)buffer, 0, &bufSize);
    if (status != SUCCESS) {
	goto bailOut;
    }

    if (bufSize > FS_MAX_PATH_NAME_LENGTH) {
	printf("Fs_ReadLinkStub: symbolic link %s has a bogus length (%d).\n", 
	       linkName, bufSize);
	status = FS_INVALID_ARG;
	goto bailOut;
    }
		
    /*
     * Sprite's link count includes the terminating null character
     * in the character count return while Unix doesn't.  Make our
     * count backward-compatible with Unix.
     */
    status = Vm_CopyIn(bufSize, (Address)buffer, (Address)newLinkName);
    if (status == SUCCESS && newLinkName[bufSize-1] == '\0') {
	bufSize--;
    }

    *linkSizePtr = bufSize;

 bailOut:
    if (streamPtr != NULL) {
	(void)Fs_Close(streamPtr);
    }
    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_IOControlStub --
 *
 *	Stub for the Fs_IOControl system call.  IOControl commands
 *	that affect Fs_Stream objects are done here.  The rest of
 *	the commands are passed down to the stream type specific
 *	IOControl procedures.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in a Sprite return status and "pending 
 *	signals" flag.  The output buffer is modified with results for the
 *	command.  The format of the various results is explained in
 *	user/fs.h for generic operations, and in dev/ *.h for device
 *	specific operations.
 *
 * Side effects:
 *	Depends on the command.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_IOControlStub(serverPort, streamID, command, inBufSize, inBuffer,
			   outBufSize, outBuffer, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    int 	streamID;	/* User's handle on the stream */
    int 	command;	/* IOControl command */
    int 	inBufSize;	/* Size of inBuffer */
    vm_address_t inBuffer;	/* Command specific input parameters */
    int 	outBufSize;	/* Size of outBuffer */
    vm_address_t outBuffer;	/* Command specific output parameters */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    Proc_ControlBlock *procPtr;
    Fs_ProcessState *fsPtr;
    Fs_Stream 	 *streamPtr;
    register ReturnStatus status = SUCCESS;
    Address	localInBuffer = (Address)NIL;
    Address	localOutBuffer = (Address)NIL;
    Fs_IOCParam ioctl;
    Fs_IOReply reply;

#ifdef lint
    serverPort = serverPort;
#endif

    /*
     * Get a stream pointer.
     */
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	goto done;
    }

    if (!Fsutil_HandleValid(streamPtr->ioHandlePtr)) {
	status = FS_STALE_HANDLE;
	goto done;
    }

    ioctl.command = command;
    ioctl.format = fsMach_Format;
    ioctl.procID = procPtr->processID;
    ioctl.familyID = procPtr->familyID;
    ioctl.uid = procPtr->effectiveUserID;

    /*
     * Fast path for non-generic I/O controls to pseudo-devices.
     * We don't copy in/out the user's parameter blocks because the
     * pseudo-device code does direct cross-address-space copy later.
     * We also skip the check against large parameter blocks so arbitrary
     * amounts of data can be fed to and from a pseudo-device.
     */
    if ((streamPtr->ioHandlePtr->fileID.type == FSIO_LCL_PSEUDO_STREAM) &&
	(command > IOC_GENERIC_LIMIT)) {
	ioctl.inBufSize = inBufSize;
	ioctl.inBuffer = (Address)inBuffer;
	ioctl.outBufSize = outBufSize;  
	ioctl.outBuffer = (Address)outBuffer;
	ioctl.flags = FS_USER_IN|FS_USER_OUT;
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	goto done;
    }

    if (inBufSize > IOC_MAX_BYTES || outBufSize > IOC_MAX_BYTES) {
	status = SYS_INVALID_ARG;
	goto done;
    }
    ioctl.flags = 0;	/* We'll copy buffer's into/out of the kernel */

    /*
     * The input parameters are copied into kernel
     * space, and the results are copied back out so that the
     * lower level routines don't have to worry about it.
     */
    if ((outBufSize > 0) && (outBuffer != (vm_address_t)USER_NIL) &&
			    (outBuffer != (vm_address_t)NIL)){
	ioctl.outBuffer = localOutBuffer = (Address) ckalloc(outBufSize);
	ioctl.outBufSize = outBufSize;
    } else {
	ioctl.outBuffer = (Address)NIL;
	ioctl.outBufSize = outBufSize = 0;
    }
    if ((inBufSize > 0) && (inBuffer != (vm_address_t)USER_NIL) &&
			   (inBuffer != (vm_address_t)NIL)) {
#ifdef SOSP91
	/*
	 * Allocate space after the arguments on an IOC_REPOSITION for the
	 * current offset.
	 */
	ioctl.inBuffer  = localInBuffer = (Address) 
				ckalloc(inBufSize + 3 * sizeof(int));
#else
	ioctl.inBuffer  = localInBuffer = (Address) ckalloc(inBufSize);
#endif
	ioctl.inBufSize = inBufSize;
    } else {
	ioctl.inBuffer = (Address)NIL;
	ioctl.inBufSize = 0;
    }

    if (ioctl.inBufSize &&
	Vm_CopyIn(ioctl.inBufSize, (Address)inBuffer, ioctl.inBuffer) 
		!= SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else {
	status = Fs_IOControl(streamPtr, &ioctl, &reply);
	if (status == SUCCESS) {
	    /*
	     * Post process the set/get flags stuff because the close-on-exec
	     * flag is not kept down at the stream level, but up along
	     * with the streamID.
	     */
	    fsPtr = procPtr->fsPtr;
	    switch(command) {
		case IOC_GET_FLAGS: {
		    if (fsPtr->streamFlags[streamID] & FS_CLOSE_ON_EXEC) {
			*(int *)ioctl.outBuffer |= IOC_CLOSE_ON_EXEC;
		    }
		    break;
		}
		case IOC_SET_BITS:
		case IOC_SET_FLAGS: {
		    int flags;
		    flags = *(int *)ioctl.inBuffer;

		    if (flags & IOC_CLOSE_ON_EXEC) {
			fsPtr->streamFlags[streamID] |= FS_CLOSE_ON_EXEC;
		    } else if (command == IOC_SET_FLAGS) {
			fsPtr->streamFlags[streamID] &= ~FS_CLOSE_ON_EXEC;
		    }
		    break;
		}
		case IOC_CLEAR_BITS:{
		    int flags;
		    flags = *(int *)ioctl.inBuffer;
		    if (flags & IOC_CLOSE_ON_EXEC) {
			fsPtr->streamFlags[streamID] &= ~FS_CLOSE_ON_EXEC;
		    }
		    break;
		}
	    }
	    if (outBufSize) {
		status = Vm_CopyOut(reply.length, ioctl.outBuffer,
				    (Address)outBuffer);
	    }
	}
    }
    if (localInBuffer != (Address)NIL) {
	ckfree(localInBuffer);
    }
    if (localOutBuffer != (Address)NIL) {
	ckfree(localOutBuffer);
    }

 done:
    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(procPtr);
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CreatePipeStub --
 *
 *      This is the stub for the Fsio_CreatePipe system call.  This routine
 *      sets up stream IDs for the two Fs_Stream objects returned by (the
 *      internal) Fsio_CreatePipe: one for reading the pipe and one for
 *      writing to it.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code, the 
 *	two new stream IDs, and the "pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
kern_return_t
Fs_CreatePipeStub(serverPort, statusPtr, inStreamIDPtr, outStreamIDPtr,
		  sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    int *inStreamIDPtr;		/* OUT: Handle for reading the pipe. */
    int *outStreamIDPtr;	/* OUT: Handle for writing the pipe. */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
    register ReturnStatus	status;
    Fs_Stream			*inStreamPtr;
    Fs_Stream			*outStreamPtr;

#ifdef lint
    serverPort = serverPort;
#endif

    /*
     * Call the internal routine to create the pipe.
     */
    status = Fsio_CreatePipe(&inStreamPtr, &outStreamPtr);
    if (status != SUCCESS) {
	goto bailOut;
    }
    inStreamPtr->flags |= FS_USER;
    outStreamPtr->flags |= FS_USER;

    /*
     * Get stream ids for the two streams.
     */

    status = Fs_GetStreamID(inStreamPtr, inStreamIDPtr);
    if (status != SUCCESS) {
	(void) Fs_Close(inStreamPtr);
	(void) Fs_Close(outStreamPtr);
	goto bailOut;
    }
    status = Fs_GetStreamID(outStreamPtr, outStreamIDPtr);
    if (status != SUCCESS) {
	Fs_ClearStreamID(*inStreamIDPtr, (Proc_ControlBlock *)NIL);
	(void) Fs_Close(inStreamPtr);
	(void) Fs_Close(outStreamPtr);
	goto bailOut;
    }

 bailOut:
    *statusPtr = status;
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_AccessStub --
 *
 *	UNIX "access" system call - determine if the current user process 
 *	is allowed the requested access to the named file.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and
 *	"pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Fs_AccessStub(serverPort, fileName, fileNameLength, mode, statusPtr,
	      pendingSigPtr)
    mach_port_t serverPort;	/* request port */
    Fs_PathName fileName;	/* name of file to check */
    mach_msg_type_number_t fileNameLength; /* extra parameter to appease MIG */
    int mode;			/* UNIX access mode to test for */
    ReturnStatus *statusPtr;	/* OUT: Sprite return status */
    boolean_t *pendingSigPtr;	/* OUT: is there a signal pending */
{
    int spriteMode;

#ifdef lint
    serverPort = serverPort;
    fileNameLength = fileNameLength;
#endif

    /* 
     * Map from UNIX access mode to Sprite permissions flags.
     */
    if (mode == F_OK) {
	spriteMode = FS_EXISTS;
    } else {
	spriteMode = ((mode&R_OK)?FS_READ:0) | ((mode&W_OK)?FS_WRITE:0) |
	    ((mode&X_OK)?FS_EXECUTE:0);
    }

    *statusPtr = Fs_CheckAccess(fileName, spriteMode, TRUE);
    *pendingSigPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}
