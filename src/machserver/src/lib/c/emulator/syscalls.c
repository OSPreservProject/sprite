/* 
 * syscalls.c --
 *
 *	Facade routines to call the MIG-generated stubs for the Sprite 
 *	server.  The routines in this file mostly correspond to native
 *	Sprite system calls.  The MIG-generated stubs are all loaded
 *	together in one file, so these routines might as well be, too.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/syscalls.c,v 1.14 92/07/17 16:33:39 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <mach/message.h>
#include <bstring.h>
#include <net.h>
#include <rpc.h>
#include <spriteEmu.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <status.h>
#include <stdio.h>
#include <string.h>
#include <sys.h>
#include <sys/types.h>
#include <test.h>
#include <vm.h>
#include <vmStat.h>

/* ARGSUSED */
ReturnStatus
Fs_AttachDisk(devName, dirName, flags)
    char *devName;
    char *dirName;
    int flags;
{
    /* Currently unsupported. */
    return FAILURE;
}

ReturnStatus 
Fs_Close(streamID)
    int streamID;		/* the file to close */
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Boolean sigPending;

    kernStatus = Fs_CloseStub(SpriteEmu_ServerPort(), streamID,
			      &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Fs_Command(command, bufSize, buffer)
    int command;
    int bufSize;
    Address buffer;
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Boolean sigPending;

    kernStatus = Fs_CommandStub(SpriteEmu_ServerPort(), command,
				(vm_size_t)bufSize, (vm_address_t)buffer,
				&status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Fs_GetAttributes(pathName, fileOrLink, attrPtr)
    char *pathName;		/* The name of the file to open */
    int fileOrLink;		/* FS_ATTRIB_FILE or FS_ATTRIB_LINK */
    Fs_Attributes *attrPtr;
{
    kern_return_t kernStatus;
    ReturnStatus status;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    kernStatus = Fs_GetAttributesStub(SpriteEmu_ServerPort(), pathName,
				      pathNameLength, fileOrLink, &status,
				      attrPtr, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Fs_GetAttributesID(streamID, attrPtr)
    int		streamID;	/* The user's index into its open file list */
    Fs_Attributes *attrPtr;	/* point to buffer to hold attributes */
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Boolean sigPending;

    kernStatus = Fs_GetAttributesIDStub(SpriteEmu_ServerPort(), streamID,
					&status, attrPtr, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Fs_Open(pathName, usageFlags, permissions, streamIDPtr)
    char *pathName;		/* The name of the file to open */
    int usageFlags;		/* FS_READ, FS_WRITE, FS_CREATE, FS_TRUNC */
    int permissions;		/* Permission mask to use on creation */
    int *streamIDPtr;		/* This is the user's handle on the open
				 * file used in later filesystem requests */
{
    kern_return_t kernStatus;
    ReturnStatus status;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    kernStatus = Fs_OpenStub(SpriteEmu_ServerPort(), pathName,
			     pathNameLength, usageFlags, permissions,
			     &status, streamIDPtr, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Fs_SetAttr(pathName, fileOrLink, attrPtr, flags)
    char *pathName;		/* The name of the file to open */
    int fileOrLink;		/* Whether or not to follow symbolic links */
    Fs_Attributes *attrPtr;	/* New values for attributes */
    int flags;			/* What attributes to set */
{
    kern_return_t kernStatus;
    ReturnStatus status;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    kernStatus = Fs_SetAttrStub(SpriteEmu_ServerPort(), pathName,
			     pathNameLength, fileOrLink, *attrPtr, flags,
			     &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Fs_SetAttrID(streamID, attrPtr, flags)
    int streamID;		/* References file to manipulate */
    Fs_Attributes *attrPtr;	/* New values for attributes */
    int flags;			/* What attributes to set */
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Boolean sigPending;

    kernStatus = Fs_SetAttrIDStub(SpriteEmu_ServerPort(), streamID,
				  *attrPtr, flags, &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Fs_SymLink(targetName, linkName, remoteFlag)
    char *targetName;	/* Name of the file to link to */
    char *linkName;	/* The name of the new link file that's created */
    Boolean remoteFlag;	/* TRUE => link will be a REMOTE_LINK */
{
    kern_return_t kernStatus;
    ReturnStatus status;
    mach_msg_type_number_t targetNameLength = strlen(targetName) + 1;
    mach_msg_type_number_t linkNameLength = strlen(linkName) + 1;
    Boolean sigPending;

    kernStatus = Fs_SymLinkStub(SpriteEmu_ServerPort(), targetName,
				targetNameLength, linkName, linkNameLength,
				remoteFlag, &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Net_InstallRoute(size, routeInfoPtr)
    int size;
    Net_RouteInfo *routeInfoPtr;
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Boolean sigPending;

    kernStatus = Net_InstallRouteStub(SpriteEmu_ServerPort(), size,
				      (vm_address_t)routeInfoPtr, &status,
				      &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Proc_Detach(statusCode)
    int statusCode;
{
    Boolean sigPending;
    ReturnStatus status;

    status = Utils_MapMachStatus(Proc_DetachStub(SpriteEmu_ServerPort(),
						 statusCode, &sigPending));
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Proc_GetHostIDs(virtualHostPtr, physicalHostPtr)
    int	*virtualHostPtr;   	/* Buffer to hold virtual host ID. */
    int	*physicalHostPtr;   	/* Buffer to hold physical host ID. */
{
    kern_return_t kernStatus;
    ReturnStatus status = SUCCESS;
    Boolean sigPending;
    int dummyVirt;		/* in case virtualHostPtr is nil */
    int dummyPhys;		/* in case physicalHostPtr is nil */

    if (virtualHostPtr == NULL) {
	virtualHostPtr = &dummyVirt;
    }
    if (physicalHostPtr == NULL) {
	physicalHostPtr = &dummyPhys;
    }
    kernStatus = Proc_GetHostIDsStub(SpriteEmu_ServerPort(),
				     virtualHostPtr, physicalHostPtr, 
				     &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus 
Proc_GetPCBInfo(firstPid, lastPid, hostID, infoSize, bufferPtr, 
		argsPtr, trueNumBuffersPtr)
    Proc_PID 		firstPid;	     /* First pid to get info for. */
    Proc_PID		lastPid;	     /* Last pid to get info for. */
    int			hostID;		     /* Host ID to get info for. */
    int			infoSize;   	     /* Size of structure */
    Address	 	bufferPtr;	     /* Pointer to buffers. */
    Proc_PCBArgString	*argsPtr;	     /* Pointer to argument strings. */
    int 		*trueNumBuffersPtr;  /* The actual number of buffers 
						used.*/
{
    kern_return_t kernStatus;
    ReturnStatus status;
    int dummy;			/* in case caller doesn't want num buffers */
    Boolean sigPending;

    if (trueNumBuffersPtr == NULL) {
	trueNumBuffersPtr = &dummy;
    }
    kernStatus = Proc_GetPCBInfoStub(SpriteEmu_ServerPort(), firstPid,
				     lastPid, hostID, infoSize,
				     (vm_address_t)bufferPtr,
				     (vm_address_t)argsPtr, &status,
				     trueNumBuffersPtr, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Sig_Send(sigNum, id, isFamily)
    int		sigNum;		/* The signal to send. */
    Proc_PID	id;		/* The id number of the process or process
				   family. */
    Boolean	isFamily;	/* Whether the id is a process id or a process
				   group id. */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;

    kernStatus = Sig_SendStub(SpriteEmu_ServerPort(), sigNum, id, isFamily,
			      &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }

    return status;
}

ReturnStatus
Sig_SetHoldMask(newMask, oldMaskPtr)
    int	newMask;	/* Mask to set the hold mask to. */
    int	*oldMaskPtr;	/* Where to store the old mask. */
{
    kern_return_t kernStatus;
    int dummyMask;		/* in case the old mask wasn't requested */
    Boolean sigPending;

    if (oldMaskPtr == NULL) {
	oldMaskPtr = &dummyMask;
    }
    kernStatus = Sig_SetHoldMaskStub(SpriteEmu_ServerPort(), newMask,
				     oldMaskPtr, &sigPending);
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return Utils_MapMachStatus(kernStatus);
}

ReturnStatus
Sys_GetMachineInfo(size, bufferPtr)
    int size;
    Address bufferPtr;
{
    boolean_t sigPending;
    kern_return_t kernStatus;
    ReturnStatus status;

    kernStatus = Sys_GetMachineInfoStub(SpriteEmu_ServerPort(), size,
					(vm_address_t)bufferPtr, &status,
					&sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Sys_GetTimeOfDay(timePtr, offsetPtr, dstPtr)
    Time *timePtr;
    int *offsetPtr;
    Boolean *dstPtr;
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Time dummyTime;		/* in case user's ptr is nil */
    int dummyOffset;		/* ditto */
    Boolean dummyDst;		/* ditto */
    Boolean sigPending;

    if (timePtr == NULL) {
	timePtr = &dummyTime;
    }
    if (offsetPtr == NULL) {
	offsetPtr = &dummyOffset;
    }
    if (dstPtr == NULL) {
	dstPtr = &dummyDst;
    }

    kernStatus = Sys_GetTimeOfDayStub(SpriteEmu_ServerPort(), timePtr,
				      offsetPtr, dstPtr, &status, &sigPending);
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }

    return status;
}

ReturnStatus
Sys_SetTimeOfDay(timePtr, offset, dst)
    Time *timePtr;
    int offset;
    Boolean dst;
{
    kern_return_t kernStatus;
    ReturnStatus status;
    Boolean sigPending;

    kernStatus = Sys_SetTimeOfDayStub(SpriteEmu_ServerPort(), *timePtr,
				      offset, dst, &status, &sigPending);
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }

    return status;
}

ReturnStatus
Sys_Shutdown(flags, rebootString)
    int flags;
    char *rebootString;		/* name of file to reboot with */
{
    boolean_t sigPending;
    ReturnStatus status;

    if (rebootString != NULL) {
	fprintf(stderr, "Sys_Shutdown: reboot string not supported.\n");
    }
    status = Utils_MapMachStatus(Sys_ShutdownStub(SpriteEmu_ServerPort(),
						  flags, &sigPending));
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Sys_Stats(command, option, argPtr)
    int command;
    int option;
    Address argPtr;
{
    ReturnStatus status;
    Boolean sigPending;
    kern_return_t kernStatus;

    kernStatus = Sys_StatsStub(SpriteEmu_ServerPort(), command, option,
			       (vm_address_t)argPtr, &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

void
Test_GetString(buf, bufLen)
    char *buf;
    int bufLen;
{
    (void)Test_GetStringStub(SpriteEmu_ServerPort(), (vm_address_t)buf,
			     bufLen);
}

/* ARGSUSED */
void
Test_MemCheck(file)
    char *file;			/* currently ignored */
{
    (void)Test_MemCheckStub(SpriteEmu_ServerPort());
}

void
Test_PutDecimal(n)
    int n;
{
    (void)Test_PutDecimalStub(SpriteEmu_ServerPort(), n);
}

void
Test_PutHex(n)
    int n;
{
    (void)Test_PutHexStub(SpriteEmu_ServerPort(), n);
}

void
Test_PutMessage(s)
    char *s;
{
    (void)Test_PutMessageStub(SpriteEmu_ServerPort(), s);
}

void
Test_PutOctal(n)
    int n;
{
    (void)Test_PutOctalStub(SpriteEmu_ServerPort(), n);
}

void
Test_PutString(buf, bufLen)
    char *buf;
    int bufLen;
{
    (void)Test_PutStringStub(SpriteEmu_ServerPort(), (vm_address_t)buf,
			     bufLen);
}

void
Test_PutTime(now, overwrite)
    time_t now;
    Boolean overwrite;
{
    (void)Test_PutTimeStub(SpriteEmu_ServerPort(), (int)now, overwrite);
}

void
Test_Return1()
{
    (void)Test_Return1Stub(SpriteEmu_ServerPort());
}

void
Test_Return2()
{
    (void)Test_Return2Stub(SpriteEmu_ServerPort());
}

ReturnStatus
Test_Rpc(command, argPtr)
    int command;
    Address argPtr;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;
    
    kernStatus = Test_RpcStub(SpriteEmu_ServerPort(), command,
			      (vm_address_t)argPtr, &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

ReturnStatus
Vm_Cmd(command, length, arg)
    int command;
    int length;
    int arg;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;
    vm_address_t outBuf;
    mach_msg_type_number_t outBufLength;

    /* 
     * Which stub we use depends on which command is invoked.
     */
    switch (command) {
    case VM_DO_COPY_IN_INBAND:
	kernStatus = Vm_CmdInbandStub(SpriteEmu_ServerPort(), command,
				      length, (Sys_CharArray)arg, 
				      (mach_msg_type_number_t)length,
				      &outBuf, &outBufLength, &status,
				      &sigPending);
	if (outBuf != 0) {
	    (void)vm_deallocate(mach_task_self(), outBuf, outBufLength);
	}
	break;
    case VM_DO_COPY_OUT_INBAND:
	kernStatus = Vm_CmdInbandStub(SpriteEmu_ServerPort(), command,
				      length, (Sys_CharArray)0, 0, &outBuf,
				      &outBufLength, &status, &sigPending);
	bcopy(outBuf, arg, length);
	if (outBuf != 0) {
	    (void)vm_deallocate(mach_task_self(), outBuf, outBufLength);
	}
	break;
    default:
	kernStatus = Vm_CmdStub(SpriteEmu_ServerPort(), command,
				(vm_size_t)length, (vm_address_t)arg, &status,
				&sigPending);
	break;
    }

    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}

/* 
 * Eventually there should be a MIG call to the server to get information 
 * about the VM segments (aka Mach memory objects).  For the time being, 
 * though, just zero out the record.
 */

ReturnStatus
Vm_GetSegInfo(infoPtr, segID, infoSize, segBufPtr)
    Proc_PCBInfo *infoPtr;	/* User's copy of PCB. */
    Vm_SegmentID segID;		/* Segment number of get info for if no pcb */
    int		infoSize;	/* Size of segment info structures */
    Address	segBufPtr;	/* Where to store segment information.*/
{
#ifdef lint
    infoPtr = infoPtr;
    segID = segID;
#endif
    bzero(segBufPtr, infoSize);
    return SUCCESS;
}

ReturnStatus
Vm_PageSize(pageSizePtr)
    int *pageSizePtr;
{
    *pageSizePtr = vm_page_size;
    return SUCCESS;
}

ReturnStatus
Vm_MapFile(fileName, readOnly, offset, length, startAddrPtr)
    char *fileName;
    Boolean readOnly;
    off_t offset;
    vm_size_t length;
    Address *startAddrPtr;
{
    kern_return_t kernStatus;
    mach_msg_type_number_t fileNameLength = strlen(fileName) + 1;
    ReturnStatus status;
    Boolean sigPending;

    if (fileNameLength > FS_MAX_PATH_NAME_LENGTH) {
	return FS_INVALID_ARG;
    }
    kernStatus = Vm_MapFileStub(SpriteEmu_ServerPort(), fileName,
				fileNameLength, readOnly, offset, length,
				&status, (vm_address_t *)startAddrPtr,
				&sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    return status;
}
