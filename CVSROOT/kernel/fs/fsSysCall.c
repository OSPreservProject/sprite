/* 
 * fsSysCall.c --
 *
 *	These are the system call stubs that get called from the
 *	system call trap handler.  These routines make sure that
 *	their arguments are addressable and then call the workhorse
 *	routines to do the actual work.
 *
 * Copyright 1987 Regents of the University of California
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"

#include "fs.h"
#include "fsInt.h"
#include "fsTrace.h"
#include "fsOpTable.h"
#include "fsDevice.h"
#include "fsNameOps.h"
#include "mem.h"

#include "vm.h"
#include "proc.h"


/*
 *----------------------------------------------------------------------
 *
 * Fs_AttachDiskStub --
 *
 *	Stub for the Fs_AttachDisk system call.  This gets ahold of
 *	the filename arguments, opens the disk to be attached, and
 *	calls FsAttachDisk to do the work.
 *
 * Results:
 *	SUCCESS, or an error code from finding junk on the disk.
 *
 * Side effects:
 *	Call Fs_AttachDisk or Fs_DetachDisk.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_AttachDiskStub(deviceFile, localName, flags)
    char *deviceFile;	/* Name of raw disk file to attach */
    char *localName;	/* Local directory name where disk is attached */
    int flags;		/* FS_ATTACH_READ_ONLY, FS_DETATCH, or
			 * FS_ATTACH_LOCAL */
{
    ReturnStatus status;
    int useFlags;		/* Flags for the Fs_Open call */
    Fs_Stream *streamPtr;	/* Stream from the Fs_Open call */
    int	localNameLength;
    int	deviceNameLength;

    if (localName != (char *)NIL && localName != (char *)0) {
	status = Fs_MakeNameAccessible(&localName, &localNameLength);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	localName = (char *)NIL;
    }
    if (deviceFile != (char *)NIL && deviceFile != (char *)0) {
	status = Fs_MakeNameAccessible(&deviceFile, &deviceNameLength);
	if (status != SUCCESS) {
	    if (localName != (char *)NIL) {
		Vm_MakeUnaccessible((Address)localName, localNameLength);
	    }
	    return(status);
	}
    } else {
	deviceFile = (char *)NIL;
    }
    if (flags & FS_DETACH) {
	/*
	 * Flush data associated with the domain and remove the domain
	 * from the prefix table.
	 */
	status = FsDetachDisk(localName);
    } else {
	/*
	 * Attach a local domain into the prefix table.
	 */
	if (deviceFile == (char *)NIL ||
	    localName == (char *)NIL) {
	    status = FS_INVALID_ARG;
	} else {
	    if (flags & FS_ATTACH_READ_ONLY) {
		useFlags = FS_READ;
	    } else {
		useFlags = FS_READ|FS_WRITE;
	    }
	    status = Fs_Open(deviceFile, useFlags, FS_DEVICE, 0, &streamPtr);
	    if (status == SUCCESS) {
		FsDeviceIOHandle *devHandlePtr =
			(FsDeviceIOHandle *)streamPtr->ioHandlePtr;
		status = FsAttachDisk(&devHandlePtr->device, localName, flags);
	    }
	}
    }
    if (deviceFile != (char *)NIL) {
	Vm_MakeUnaccessible((Address)deviceFile, deviceNameLength);
    }
    if (localName != (char *)NIL) {
	Vm_MakeUnaccessible((Address)localName, localNameLength);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_OpenStub --
 *
 *      This is the stub for the Fs_Open system call.  This routine makes
 *	its arguments accessible, then sets up a Stream ID for the Fs_Stream
 *      object returned by (the internal) Fs_Open.  Stream Id's get
 *      passed to read and write calls and identify the stream to the
 *      system.  If FS_CREATE is one of the usage flags then the third
 *      argument, permissions, specifies the base permissions for the
 *      newly created file.
 *
 * Results:
 *	An error code or SUCCESS if successful.
 *
 * Side effects:
 *	The last argument is an out parameter and gets filled in with
 *	the stream id.  
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_OpenStub(pathName, usageFlags, permissions, streamIDPtr)
    char *pathName;		/* The name of the file to open */
    int usageFlags;		/* FS_READ, FS_WRITE, FS_CREATE, FS_TRUNC */
    int permissions;		/* Permission mask to use on creation */
    int *streamIDPtr;		/* This is the user's handle on the open
				 * file used in later filesystem requests */
{
    int		 	streamID;
    Fs_Stream	 	*streamPtr;
    ReturnStatus 	status;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    FS_TRACE(FS_TRACE_OPEN);

    /*
     * Copy the name in from user space to the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    /*
     * Open the file and get a stream descriptor for it.
     */
    usageFlags &= ~FS_KERNEL_FLAGS;
    usageFlags |= (FS_USER | FS_FOLLOW);
    if (fsTracing) {
	usageFlags |= FS_TRACE_FLAG;
    }

    status = Fs_Open(newName, usageFlags, FS_FILE,
		     permissions & 0777, &streamPtr);
    
    FS_TRACE_NAME(FS_TRACE_10, pathName);

    if (status != SUCCESS) {
	return(status);
    }

    /*
     * Save a pointer to the descriptor in a list kept in the proc table.
     * Its index in the list is the "Stream ID".
     */

    status = FsGetStreamID(streamPtr, &streamID);
    if (status != SUCCESS) {
	(void) Fs_Close(streamPtr);
	return(status);
    }

    if (Vm_CopyOut(sizeof(int), (Address) &streamID, 
		   (Address) streamIDPtr) == SUCCESS) {
	FS_TRACE(FS_TRACE_11);
	return(SUCCESS);
    } 
    status = SYS_ARG_NOACCESS;
    FsClearStreamID(streamID, (Proc_ControlBlock *)NIL);
    (void) Fs_UserClose(streamID);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_UserClose --
 *
 *	System call to release a stream Id.  Calls then internal Fs_Close
 *	routine.
 *
 * Results:
 *	An return status.
 *
 * Side effects:
 *	The streamID becomes invalid (until it gets reassigned by
 *	another Fs_Open call).
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_UserClose(streamID)
    int streamID;
{
    ReturnStatus	status;
    Fs_Stream 		*streamPtr;	/* Stream pointer from the process's
				 	 * list of open streams */
    Proc_ControlBlock	*procPtr;	/* This process's control block */
    
    /*
     * Map from the streamID to a streamPtr.
     */

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }

    /*
     * Currently, Fs_Close frees the file object regardless of
     * the error return from the file server, so we go ahead an
     * clear our stream id regardless of the error from Fs_Close.
     */

    FsClearStreamID(streamID, procPtr);
    status = Fs_Close(streamPtr);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_ReadStub --
 *
 *      The stub for the Fs_Read system call.  Calls Fs_UserRead to do the
 *	work, then copies out the argument to the user's address space.
 *
 * Results:
 *	An return status.
 *
 * Side effects:
 *	The amountReadPtr is updated to reflect the amount actually read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_ReadStub(streamID, amountRead, buffer, amountReadPtr)
    int		streamID;	/* The user's index into its open file list */
    int		amountRead;	/* The amount of bytes to read */
    Address	buffer;		/* The storage place for the read */
    int		*amountReadPtr;	/* The amount of bytes actually read */
{
    register ReturnStatus	status;
    Fs_Stream		*streamPtr;	/* The stream to read from */
    Proc_ControlBlock 	*procPtr;	/* This process's control block */

    /*
     * (Begin inlined called to Fs_UserRead.)
     */
    /*
     * Map from stream ID to file pointer and do the read.
     * If the process is not remote, Fs_Read takes care of making
     * the read buffer accessible.
     */

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_Read(streamPtr, buffer, streamPtr->offset, &amountRead);
    } else {
	amountRead = 0;
    }
    /*
     * (End Fs_UserRead.)
     */

    if (Vm_CopyOut(sizeof(int), (Address) &amountRead, 
		   (Address) amountReadPtr) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_UserRead --
 *
 *      The Fs_Read system call.  Read from the file indicated by the
 *      stream id into the buffer.  bufSize indicates how much
 *      data to read, and amountReadPtr is an output parameter that
 *      indicates how much data was read.  A length of zero means
 *      end-of-file.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	The amountReadPtr is updated to reflect the amount actually read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_UserRead(streamID, amountRead, buffer, amountReadPtr)
    int		streamID;	/* The user's index into its open file list */
    int		amountRead;	/* The amount of bytes to read */
    Address	buffer;		/* The storage place for the read */
    int		*amountReadPtr;	/* The amount of bytes actually read */
{
    register ReturnStatus	status;
    Fs_Stream		*streamPtr;	/* The stream to read from */
    Proc_ControlBlock 	*procPtr;	/* This process's control block */

    /*
     * Map from stream ID to file pointer and do the read.
     * If the process is not remote, Fs_Read takes care of making
     * the read buffer accessible.
     */

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	*amountReadPtr = amountRead;
	status = Fs_Read(streamPtr, buffer, streamPtr->offset, amountReadPtr);
    } else {
	*amountReadPtr = 0;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ReadVectorStub --
 *
 *      The stub for the Fs_ReadVector system call.  Calls Fs_UserReadVector 
 *	to do the work, then copies out the argument to the user's address 
 *	space.
 *
 * Results:
 *	An return status.
 *
 * Side effects:
 *	The amountReadPtr is updated to reflect the amount actually read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_ReadVectorStub(streamID, numVectors, userVectorArray, amountReadPtr)
    int		streamID;	/* The user's index into its open file list. */
    int		numVectors;	/* The # of vectors in userVectorArray. */
    Fs_IOVector	userVectorArray[];	
				/* The vectors defining where and how much to
				 * read. */
    int		*amountReadPtr;	/* The amount of bytes actually read */
{
    ReturnStatus status;
    int		amountRead;
    Fs_IOVector *vectorPtr;

    if (numVectors < 0) {
	return(SYS_INVALID_ARG);
    }

    /*
     * Copy the I/O vector into kernel space. The buffer addresses in each
     * vector will be made accessible by the Fs_Read routine.
     */
    vectorPtr = (Fs_IOVector *) Mem_Alloc(numVectors * sizeof(Fs_IOVector));
    if (Vm_CopyIn(numVectors * sizeof(Fs_IOVector), 
		   (Address) userVectorArray,
		   (Address) vectorPtr) != SUCCESS) {
	Mem_Free((Address) vectorPtr);
	return(SYS_ARG_NOACCESS);
    }

    status = Fs_UserReadVector(streamID, numVectors, vectorPtr, &amountRead);

    Mem_Free((Address) vectorPtr);
    if (Vm_CopyOut(sizeof(int), (Address) &amountRead, 
		   (Address) amountReadPtr) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_UserReadVector --
 *
 *      The Fs_ReadVector system call.  Read from the file indicated by
 *      the stream ID into the buffer.  Each I/O vector contains a value
 *      to indicate how much data to read. *AmountReadPtr contains the
 *      total number of bytes read into the vector buffers.
 *      *amountReadPtr will be smaller that the sum of the bufSize fields
 *      in the vector array if an error occurs or an end-of-file is
 *      reached.  A length of zero in *amountReadPtr means an end-of-file
 *      occurred on the first buffer in the vector.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	The *amountReadPtr is updated to reflect the amount actually read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_UserReadVector(streamID, numVectors, vectorPtr, amountReadPtr)
    int		streamID;	/* The user's index into its open file list */
    register int numVectors;	/* The # of vectors in vectorArray */
    register Fs_IOVector *vectorPtr;	
				/* The vectors defining where and how much to 
				 * read. */
    int		*amountReadPtr;	/* Total # of bytes read. */
{
    ReturnStatus	status;
    Fs_Stream		*streamPtr;	/* The stream to read from */
    Proc_ControlBlock 	*procPtr;	/* This process's control block */
    register int	sum = 0;	/* Total # of bytes read. */
    register int	i;

    /*
     * Map from stream ID to file pointer and do the read.
     * If the process is not remote, Fs_Read takes care of making
     * the read buffer accessible.
     */

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);

    if (status == SUCCESS) {
	/*
	 * Doing a Fs_Read for each vector buffer is probably not the
	 * most efficiennt way to do this, but it is simple.
	 */
	for (i= 0; i < numVectors; i++, vectorPtr++) {

	    status = Fs_Read(streamPtr, vectorPtr->buffer, streamPtr->offset, 
				&(vectorPtr->bufSize));
	    /*
	     * Check to see if the read failed or we got an end-of-file.
	     */
	    if ((status != SUCCESS) || (vectorPtr->bufSize == 0)) {
		break;
	    }
	    sum += vectorPtr->bufSize;
	}
    }
    *amountReadPtr = sum;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_WriteStub --
 *
 *	The Fs_Write system call stub.  Calls Fs_UserWrite to do the
 *	work, then copies out the amount written to the user's address space.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	The writeLengthPtr is updated to reflect the amount actually written.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_WriteStub(streamID, writeLength, buffer, writeLengthPtr)
    int streamID;
    int writeLength;
    Address buffer;
    int *writeLengthPtr;
{
    register ReturnStatus status;
    Fs_Stream	*streamPtr;	/* The stream to write to */
    Proc_ControlBlock *procPtr;	/* This process's control block */

    /*
     * (Begin inlined Fs_UserWrite.)
     */
    /*
     * Map from stream ID to streamPtr.  If the process is not remote,
     * Fs_Write takes care of making the user's buffer accessible.
     */
    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_Write(streamPtr, buffer, streamPtr->offset, &writeLength);
    } else {
	writeLength = 0;
    }
    /*
     * (End inlined Fs_UserWrite.)
     */
    if (Vm_CopyOut(sizeof(int), (Address) &writeLength, 
		   (Address) writeLengthPtr) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_UserWrite --
 *
 *	The user interface to Fs_Write.  Write writeLength characters from
 *	buffer to the file identified by streamID.  *writeLengthPtr
 *	is updated to reflect how much was actually written.
 *
 * Results:
 *	A return status. 
 *
 * Side effects:
 *	The writeLengthPtr is updated to reflect the amount actually written.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_UserWrite(streamID, writeLength, buffer, writeLengthPtr)
    int streamID;
    int writeLength;
    Address buffer;
    int *writeLengthPtr;
{
    ReturnStatus status;
    Fs_Stream	*streamPtr;	/* The stream to write to */
    Proc_ControlBlock *procPtr;	/* This process's control block */

    /*
     * Map from stream ID to streamPtr.  If the process is not remote,
     * Fs_Write takes care of making the user's buffer accessible.
     */
    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	*writeLengthPtr = writeLength;
	status = Fs_Write(streamPtr, buffer, streamPtr->offset, writeLengthPtr);
    } else {
	*writeLengthPtr = 0;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_WriteVectorStub --
 *
 *      The stub for the Fs_WriteVector system call.  Calls Fs_UserWriteVector 
 *	to do the work, then copies out the argument to the user's address 
 *	space.
 *
 * Results:
 *	An return status.
 *
 * Side effects:
 *	The *amountWrittenPtr is updated to reflect the amount actually read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_WriteVectorStub(streamID, numVectors, userVectorArray, amountWrittenPtr)
    int		streamID;	/* The user's index into its open file list. */
    int		numVectors;	/* The # of vectors in userVectorArray. */
    Fs_IOVector	userVectorArray[];	
				/* The vectors defining where and how much to
				 * write. */
    int	*amountWrittenPtr;	/* The amount of bytes actually written. */
{
    ReturnStatus status;
    int		amountWritten;
    Fs_IOVector *vectorPtr;

    if (numVectors < 0) {
	return(SYS_INVALID_ARG);
    }

    /*
     * Copy the I/O vector into kernel space. The buffer addresses in each
     * vector will be made accessible by the Fs_Write routine.
     */
    vectorPtr = (Fs_IOVector *) Mem_Alloc(numVectors * sizeof(Fs_IOVector));
    if (Vm_CopyIn(numVectors * sizeof(Fs_IOVector), 
		   (Address) userVectorArray,
		   (Address) vectorPtr) != SUCCESS) {
	Mem_Free((Address) vectorPtr);
	return(SYS_ARG_NOACCESS);
    }

    status = Fs_UserWriteVector(streamID, numVectors, vectorPtr, 
			&amountWritten);

    Mem_Free((Address) vectorPtr);
    if (Vm_CopyOut(sizeof(int), (Address) &amountWritten, 
		   (Address) amountWrittenPtr) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_UserWriteVector --
 *
 *      The Fs_WriteVector system call.  Write from the file indicated by
 *      the stream ID into the buffer.  Each I/O vector contains a value
 *      to indicate how much data to write. *AmountWrittenPtr contains the
 *      total number of bytes written from the vector buffers.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	The *amountWrittenPtr is updated to reflect the amount 
 *	actually written.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_UserWriteVector(streamID, numVectors, vectorPtr, amountWrittenPtr)
    int		streamID;	/* The user's index into its open file list */
    register int numVectors;	/* The # of vectors in vectorArray */
    register Fs_IOVector *vectorPtr;	
				/* The vectors defining where and how much to 
				 * write. */
    int		*amountWrittenPtr;	/* Total # of bytes written. */
{
    ReturnStatus	status;
    Fs_Stream		*streamPtr;	/* The stream to write to. */
    Proc_ControlBlock 	*procPtr;	/* This process's control block. */
    register int	sum = 0;	/* Total # of bytes written. */
    register int	i;

    /*
     * Map from stream ID to file pointer and do the write.
     * If the process is not remote, Fs_Write takes care of making
     * the write buffer accessible.
     */

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);

    if (status == SUCCESS) {
	/*
	 * Doing a Fs_Write for each vector buffer is probably not the
	 * most efficiennt way to do this, but it is simple.
	 */
	for (i= 0; i < numVectors; i++, vectorPtr++) {

	    status = Fs_Write(streamPtr, vectorPtr->buffer, streamPtr->offset, 
				&(vectorPtr->bufSize));
	    /*
	     * Check to see if the write failed or we got an end-of-file.
	     */
	    if ((status != SUCCESS) || (vectorPtr->bufSize == 0)) {
		break;
	    }
	    sum += vectorPtr->bufSize;
	}
    }
    *amountWrittenPtr = sum;
    return(status);
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
 *	A return code.
 *
 * Side effects:
 *	Call Fs_Command
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_CommandStub(command, bufSize, buffer)
    int command;	/* Defined in user include "test.h" */
    int bufSize;	/* Size in bytes of buffer */
    Address buffer;	/* In/Out - Command specific data */
{
    ReturnStatus status;
    Address localBuffer;

    if (bufSize > 0) {
	localBuffer = (Address)Mem_Alloc(bufSize);
	status = Vm_CopyIn(bufSize, buffer, localBuffer);
	if (status != SUCCESS) {
	    return(status);
	}
    } else {
	localBuffer = (Address)NIL;
    }

    status = Fs_Command(command, bufSize, localBuffer);

    if (localBuffer != (Address)NIL) {
	if (status == SUCCESS) {
	    status = Vm_CopyOut(bufSize, localBuffer, buffer);
	}
	Mem_Free(localBuffer);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_MakeDeviceStub --
 *
 *	The Fs_MakeDevice system call stub.
 *
 * Results:
 *	Those of Fs_MakeDevice.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_MakeDeviceStub(pathName, devicePtr, permissions)
    char *pathName;		/* Device file to create */
    Fs_Device *devicePtr;	/* Specifies the device file attributes */
    int permissions;		/* Permissions on the special file */
{
    Fs_Device 			device;
    int				pathNameLength;
    char			newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the device name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }
    if (Vm_CopyIn(sizeof(Fs_Device), (Address)devicePtr, 
		  (Address)&device) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }

    return(Fs_MakeDevice(newName, &device, permissions));
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_MakeDirStub --
 *
 *	The Fs_MakeDir system call stub.
 *
 * Results:
 *	Those of Fs_MakeDir.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_MakeDirStub(pathName, permissions)
    char *pathName;
    int permissions;
{
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the name in from user space to the kernel stack.
     */

    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    return(Fs_MakeDir(newName, permissions));
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RemoveStub --
 *
 *	The stub for the Fs_Remove system call.
 *
 * Results:
 *	Those of Fs_Remove.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_RemoveStub(pathName)
    char *pathName;
{
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the name in from user space to the kernel stack.
     */

    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    return(Fs_Remove(newName));
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RemoveDirStub --
 *
 *	The Fs_RemoveDir system call stub.
 *
 * Results:
 *	The results of Fs_RemoveDir.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_RemoveDirStub(pathName)
    char *pathName;
{
    int		pathNameLength;
    char	newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    return(Fs_RemoveDir(newName));
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_ChangeDirStub --
 *
 *	The stub for the system call that changes the current directory.
 *	The argument specifies a new current directory.  The argument
 *	can be a relative path name.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_ChangeDirStub(pathName)
    char *pathName;
{
    int		pathNameLength;
    char	newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    return(Fs_ChangeDir(newName));
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
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_GetNewIDStub(streamID, newStreamIDPtr)
    int streamID;
    int *newStreamIDPtr;
{
    ReturnStatus 	status;
    int 		newStreamID;

    if (Vm_CopyIn(sizeof(int), (Address) newStreamIDPtr, 
		  (Address) &newStreamID) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    status = Fs_GetNewID(streamID, &newStreamID);
    if (status != SUCCESS) {
	return(status);
    }

    if (Vm_CopyOut(sizeof(int), (Address) &newStreamID, 
			(Address) newStreamIDPtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    } else {
	return(SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetAttributesStub --
 *
 *	Stub for the Fs_GetAttributes system call.
 *
 * Results:
 *	The attributes for the named file.
 *
 * Side effects:
 *	*attrPtr is filled in with the attributes of the file.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_GetAttributesStub(pathName, fileOrLink, attrPtr)
    char *pathName;
    int fileOrLink;
    Fs_Attributes *attrPtr;
{
    ReturnStatus 	status;
    Fs_Attributes	attributes;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    status = Fs_GetAttributes(newName, fileOrLink, &attributes);
    if (status != SUCCESS) {
	return(status);
    }

    if (Vm_CopyOut(sizeof(attributes), (Address) &attributes, 
		   (Address) attrPtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    } else {
	return(SUCCESS);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CheckAccess --
 *
 *	Process the Fs_CheckAccess system all.
 *
 * Results:
 *	SYS_ARG_NOACCESS if pathname not accessible.
 *	FS_INVALID_ARG if pathname too long.
 *	Otherwise:
 *	    SUCCESS if accessible.
 *	    FS_NO_ACCESS if not accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_CheckAccess(pathName, perm, useRealID)
    char 	*pathName;
    int		perm;
    Boolean	useRealID;
{
    ReturnStatus	status;
    Fs_Attributes	attributes;
    Proc_ControlBlock	*procPtr;
    int			pathNameLength;
    char		newName[FS_MAX_PATH_NAME_LENGTH];
    register int	bits;

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());

    status = Fs_GetAttributes(newName, FS_ATTRIB_FILE, &attributes);
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
	for (i = 0; i < procPtr->numGroupIDs; i++) {
	    if (attributes.gid == procPtr->groupIDs[i]) {
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
 *	Get the attributes for the file referred to by the specified
 *	descriptor.
 *
 * Side effects:
 *	 *attrPtr is filled in with the attributes of the file.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_GetAttributesIDStub(streamID, attrPtr)
    int		streamID;	/* The user's index into its open file list */
    Fs_Attributes *attrPtr;	/* point to buffer to hold attributes */
{
    register	ReturnStatus 	status;
    Fs_Stream			*streamPtr;
    Fs_Attributes 		attributes;

    status = FsGetStreamPtr(Proc_GetEffectiveProc(Sys_GetProcessorNumber()),
			    streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }
    status = Fs_GetAttrStream(streamPtr, &attributes);
    if (status != SUCCESS) {
	return(status);
    }

    if (Vm_CopyOut(sizeof(attributes), (Address) &attributes, 
		   (Address) attrPtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    } else {
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetAttributesStub --
 *
 *	Stub for Fs_SetAttributes system call.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_SetAttributesStub(pathName, fileOrLink, attrPtr)
    char *pathName;
    int fileOrLink;
    Fs_Attributes *attrPtr;
{
    int				pathNameLength;
    Fs_Attributes		attr;
    char			newName[FS_MAX_PATH_NAME_LENGTH];

    if (Vm_CopyIn(sizeof(attr), (Address) attrPtr, 
		  (Address) &attr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    return(Fs_SetAttributes(newName, fileOrLink, &attr));
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetAttributesIDStub --
 *
 *	Stub for Fs_SetAttributesID system call.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_SetAttributesIDStub(streamID, attrPtr)
    int streamID;
    Fs_Attributes *attrPtr;
{
    register 	ReturnStatus 	status;
    Fs_Stream			*streamPtr;
    Fs_Attributes		attr;
    FsUserIDs			ids;
    Proc_ControlBlock		*procPtr;

    if (Vm_CopyIn(sizeof(attr), (Address) attrPtr, 
		  (Address) &attr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }

    /*
     * Map from stream ID to file pointer and get the attributes.
     */
    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status == SUCCESS) {
	FsSetIDs(procPtr, &ids);
	status = Fs_SetAttrStream(streamPtr, &attr, &ids);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetDefPermStub --
 *
 *	Set the default permissions on files created by this process.
 *	These defaults get inherited by child processes.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Sets the default permissions mask in the filesystem related
 *	state kept in the proc table.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_SetDefPermStub(permissions, oldPermPtr)
    int permissions;	/*  A mask of 9 permission bits.  Three sets of 3
			 * bits.  The first set is for the owner's permissions,
			 * the second for the group's, and the last is for
			 * everyone else.  The three bits indicate read,
			 * write, and execute permissions, respectively.
			 * A permission bit set in this mask ENABLES the
			 * permission on created files. */
    int *oldPermPtr;	/* Output, the previous value of the permissions mask */
{
    register	Proc_ControlBlock	*procPtr;
    int 				oldPerm;

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    oldPerm = procPtr->filePermissions;
    procPtr->filePermissions = (unsigned int)(permissions & 0777);
    if (Proc_ByteCopy(FALSE, sizeof(int), (Address)&oldPerm, 
		(Address)oldPermPtr) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    } else {
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_HardLinkStub --
 *
 *	Stub for the Fs_HardLink system call.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_HardLinkStub(fileName, linkName)
    char *fileName;	/* Name of the existing file */
    char *linkName;	/* New name to link to the existing file */
{
    ReturnStatus 	status = SUCCESS;
    int			fileNameLength;
    int			linkNameLength;
    char		newFileName[FS_MAX_PATH_NAME_LENGTH];
    char		*newLinkName;

    /*
     * Copy the filename in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, fileName, newFileName,
		       &fileNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (fileNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    /*
     * Maybe I could put the link name onto the stack as well but I am 
     * allocing it here because I'm not sure if the stack is big enough.
     * Should just make the stack bigger I suppose ...
     */
    newLinkName = (char  *) Mem_Alloc(FS_MAX_PATH_NAME_LENGTH);
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, linkName, newLinkName,
		       &linkNameLength) == SUCCESS) {
	if (linkNameLength == FS_MAX_PATH_NAME_LENGTH) {
	    status = FS_INVALID_ARG;
	} else {
	    status = Fs_HardLink(newFileName, newLinkName);
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }

    Mem_Free((Address) newLinkName);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_RenameStub --
 *
 *	Stub for Fs_Rename system call.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_RenameStub(pathName, newName)
    char *pathName;
    char *newName;
{
    ReturnStatus	status;
    int			pathNameLength;
    int			newNameLength;
    char		newPathName[FS_MAX_PATH_NAME_LENGTH];
    char		*newNewName;

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newPathName,
		       &pathNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    /*
     * Maybe I could put the new name onto the stack as well but I am 
     * allocing it here because I'm not sure if the stack is big enough.
     * Should just make the stack bigger I suppose ...
     */
    newNewName = (char  *) Mem_Alloc(FS_MAX_PATH_NAME_LENGTH);
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, newName, newNewName,
		       &newNameLength) == SUCCESS) {
	if (newNameLength == FS_MAX_PATH_NAME_LENGTH) {
	    status = FS_INVALID_ARG;
	} else {
	    status = Fs_Rename(newPathName, newNewName);
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }

    Mem_Free((Address) newNewName);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_SymLinkStub --
 *
 *	Stub for Fs_SymLink system call.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_SymLinkStub(targetName, linkName, remoteFlag)
    char *targetName;
    char *linkName;
    Boolean remoteFlag;
{
    ReturnStatus	status;
    int			targetNameLength;
    int			linkNameLength;
    char		newTargetName[FS_MAX_PATH_NAME_LENGTH];
    char		*newLinkName;

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, targetName, newTargetName,
		       &targetNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (targetNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    /*
     * Maybe I could put the new name onto the stack as well but I am 
     * allocing it here because I'm not sure if the stack is big enough.
     * Should just make the stack bigger I suppose ...
     */
    newLinkName = (char  *) Mem_Alloc(FS_MAX_PATH_NAME_LENGTH);
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, linkName, newLinkName,
		       &linkNameLength) == SUCCESS) {
	if (linkNameLength == FS_MAX_PATH_NAME_LENGTH) {
	    status = FS_INVALID_ARG;
	} else {
	    status = Fs_SymLink(newTargetName, newLinkName, remoteFlag);
	}
    } else {
	status = SYS_ARG_NOACCESS;
    }
    Mem_Free((Address) newLinkName);

    return(status);
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
 *	A return code from the open and read.
 *
 * Side effects:
 *	Fills in the buffer with contents of the link file, and
 *	returns the amount of data stored there in *linkSizePtr.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_ReadLinkStub(linkName, bufSize, buffer, linkSizePtr)
    char *linkName;		/* Name of the link file */
    int bufSize;		/* Size of the user's buffer */
    char *buffer;		/* The buffer to hold the link's value */
    int *linkSizePtr;		/* Return, the amount of data in buffer */
{
    ReturnStatus	status;
    Fs_Stream		*streamPtr;
    int			linkNameLength;
    char		newLinkName[FS_MAX_PATH_NAME_LENGTH];

    /*
     * Copy the name in from user space onto the kernel stack.
     */
    if (Fs_StringNCopy(FS_MAX_PATH_NAME_LENGTH, linkName, newLinkName,
		       &linkNameLength) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (linkNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }

    status = Fs_Open(newLinkName, FS_READ | FS_USER,
		     FS_SYMBOLIC_LINK|FS_REMOTE_LINK, 0, &streamPtr);
    if (status == SUCCESS) {
	status = Fs_Read(streamPtr, buffer, 0, &bufSize);
	if (status == SUCCESS) {
	    status = Proc_ByteCopy(FALSE, sizeof(int), (Address)&bufSize, 
			       (Address)linkSizePtr);
	}
	(void)Fs_Close(streamPtr);
    }
    return(status);
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
 *	A return status.  The output buffer is modified with results
 *	for the command.  The format of the various results is explained
 *	in user/fs.h for generic operations, and in dev/*.h for
 *	device specific operations.
 *
 * Side effects:
 *	Depends on the command.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_IOControlStub(streamID, command, inBufSize, inBuffer,
			   outBufSize, outBuffer)
    int 	streamID;	/* User's handle on the stream */
    int 	command;	/* IOControl command */
    int 	inBufSize;	/* Size of inBuffer */
    Address 	inBuffer;	/* Command specific input parameters */
    int 	outBufSize;	/* Size of outBuffer */
    Address 	outBuffer;	/* Command specific output parameters */
{
    Fs_Stream 	 *streamPtr;
    register ReturnStatus status = SUCCESS;
    register Address localInBuffer;
    register Address localOutBuffer;

    /*
     * Get a stream pointer.
     */
    status = FsGetStreamPtr(Proc_GetEffectiveProc(Sys_GetProcessorNumber()), 
			    streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }

    if (!FsHandleValid(streamPtr->ioHandlePtr)) {
	return(FS_STALE_HANDLE);
    }

    if (inBufSize > IOC_MAX_BYTES || outBufSize > IOC_MAX_BYTES) {
	return(SYS_INVALID_ARG);
    }

    /*
     * The input parameters are copied into kernel
     * space, and the results are copied back out so that the
     * lower level routines don't have to worry about it.
     */
    if ((outBufSize > 0) && (outBuffer != (Address)0) &&
			    (outBuffer != (Address)NIL)){
	localOutBuffer = (Address) Mem_Alloc(outBufSize);
    } else {
	localOutBuffer = (Address)NIL;
	outBufSize = 0;
    }
    if ((inBufSize > 0) && (inBuffer != (Address)0) &&
			   (inBuffer != (Address)NIL)) {
	localInBuffer  = (Address) Mem_Alloc(inBufSize);
    } else {
	localInBuffer = (Address)NIL;
	inBufSize = 0;
    }

    if (Vm_CopyIn(inBufSize, inBuffer, localInBuffer) != SUCCESS) {
	status = SYS_ARG_NOACCESS;
    } else {
	status = Fs_IOControl(streamPtr, command,
			      inBufSize, localInBuffer,
			      outBufSize, localOutBuffer);
	if (status == SUCCESS) {
	    status = Vm_CopyOut(outBufSize, localOutBuffer, outBuffer);
	}
    }
    if (localInBuffer != (Address)NIL) {
	Mem_Free((Address) localInBuffer);
    }
    if (localOutBuffer != (Address)NIL) {
	Mem_Free((Address) localOutBuffer);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_CreatePipeStub --
 *
 *      This is the stub for the Fs_CreatePipe system call.  This routine sets
 *      up stream IDs for the two Fs_Stream objects returned by (the internal)
 *      Fs_CreatePipe: one for reading the pipe and one for writing to it.
 *
 * Results:
 *	A retrun status or SUCCESS if successful.
 *
 * Side effects:
 *	The two arguments are out parameters and get filled in with a
 *	stream id.  Also, opening a pipe sets up state until both ends are 
 *	closed with Fs_Close system calls.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_CreatePipeStub(inStreamIDPtr, outStreamIDPtr)
    int *inStreamIDPtr;		/* Handle that the user can use to read the
				 * pipe. */
    int *outStreamIDPtr;	/* Handle that the user can use to write the
				 * pipe. */
{
    register ReturnStatus	status;
    Fs_Stream			*inStreamPtr;
    int		 		inStreamID;
    Fs_Stream			*outStreamPtr;
    int				outStreamID;

    /*
     * Call the internal routine to create the pipe.
     */
    status = Fs_CreatePipe(&inStreamPtr, &outStreamPtr);
    if (status != SUCCESS) {
	return(status);
    }
    inStreamPtr->flags |= FS_USER;
    outStreamPtr->flags |= FS_USER;

    /*
     * Get stream ids for the two streams.
     */

    status = FsGetStreamID(inStreamPtr, &inStreamID);
    if (status != SUCCESS) {
	(void) Fs_Close(inStreamPtr);
	(void) Fs_Close(outStreamPtr);
	return(status);
    }
    status = FsGetStreamID(outStreamPtr, &outStreamID);
    if (status != SUCCESS) {
	FsClearStreamID(inStreamID, (Proc_ControlBlock *)NIL);
	(void) Fs_Close(inStreamPtr);
	(void) Fs_Close(outStreamPtr);
	return(status);
    }

    /*
     * Copy out the stream ids to the user.
     */

    if (Vm_CopyOut(sizeof(int), (Address) &inStreamID, 
		   (Address) inStreamIDPtr) == SUCCESS &&
	Vm_CopyOut(sizeof(int), (Address) &outStreamID, 
		   (Address) outStreamIDPtr) == SUCCESS) {
	return(SUCCESS);
    }

    /*
     * If couldn't copy out then clean up state and return an error.
     */

    FsClearStreamID(inStreamID, (Proc_ControlBlock *)NIL);
    FsClearStreamID(outStreamID, (Proc_ControlBlock *)NIL);
    (void) Fs_Close(inStreamPtr);
    (void) Fs_Close(outStreamPtr);

    return(SYS_ARG_NOACCESS);
}

