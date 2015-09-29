/* 
 * vmSysCall.c --
 *
 *	VM-related user "system" calls.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmSysCall.c,v 1.3 92/07/16 18:05:32 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <sys/types.h>

#include <fs.h>
#include <proc.h>
#include <sig.h>
#include <spriteSrvServer.h>
#include <vm.h>
#include <vmInt.h>


/*
 *----------------------------------------------------------------------
 *
 * Vm_MapFileStub --
 *
 *	System call stub to map a file into the user's address space.  
 *	See Vm_MapFile.
 *
 * Results:
 *	Returns a Mach status code, usually KERN_SUCCESS.  Fills in 
 *	the Sprite status code, the address that the file was
 *	mapped into, and the "pending signals" flag.
 *	The Sprite status is FS_NO_ACCESS if the file is to be mapped
 *	read-only and the given offset plus length run off the end of the
 *	file.
 *
 * Side effects:
 *	Maps the requested portion of the file into the user's address 
 *	space.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Vm_MapFileStub(serverPort, fileName, fileNameLength, readOnly, offset,
	       length, statusPtr, startAddrPtr, sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    Fs_PathName fileName;	/* name of file to map */
    mach_msg_type_number_t fileNameLength; /* extra parameter to appease MIG */
    boolean_t readOnly;		/* map the file read-only or read-write? */
    off_t offset;		/* where in the file to start mapping. 
				 * NB: not necessarily page-aligned */
    vm_size_t length;		/* how much of the file to map */
    ReturnStatus *statusPtr;	/* OUT: Sprite return status */
    vm_address_t *startAddrPtr;	/* OUT: where file was mapped to */
    boolean_t *sigPendingPtr;	/* OUT: is there a pending signal */
{
    Proc_ControlBlock *procPtr;
    kern_return_t kernStatus = KERN_SUCCESS;

#ifdef lint
    fileNameLength = fileNameLength;
#endif

    procPtr = Proc_SyscallToPCB(serverPort);
    if (procPtr == NULL) {
	kernStatus = KERN_INVALID_CAPABILITY;
    } else {
	kernStatus = Vm_MapFile(procPtr, fileName, readOnly, offset, length,
				statusPtr, (Address *)startAddrPtr);
    }

    *sigPendingPtr = Sig_Pending(procPtr);
    return kernStatus;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CreateVAStub --
 *
 *	System call stub to validate (and create, if necessary) a given 
 *	range of addresses.  See Vm_CreateVA.
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
Vm_CreateVAStub(serverPort, address, bytes, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* server request port */
    vm_address_t address;	/* starting address to validate */
    vm_size_t bytes;		/* number of bytes to validate */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a pending signal */
{
#ifdef lint
    serverPort = serverPort;
#endif

    *statusPtr = Vm_CreateVA((Address)address, bytes);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CmdStub --
 *
 *	MIG stub for the VM command catch-all.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending 
 *	signal" flag.
 *
 * Side effects:
 *	Depends on the command.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Vm_CmdStub(serverPort, command, length, arg, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* server request port */
    int command;		/* the specific command */
    vm_size_t length;		/* buffer length if arg is a buffer address */
    vm_address_t arg;		/* some argument for the command */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a pending signal */
{
#ifdef lint
    serverPort = serverPort;
#endif

    *statusPtr = Vm_Cmd(command, length, (Address)arg);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Vm_CmdInbandStub --
 *
 *	Like Vm_CmdStub, but let MIG take care of moving the argument/result 
 *	buffers.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending 
 *	signal" flag.
 *
 * Side effects:
 *	Depends on the command.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Vm_CmdInbandStub(serverPort, command, option, inBuf, inBufLength, outBufPtr,
		 outBufLengthPtr, outBufDeallocPtr, statusPtr, sigPendingPtr)
    mach_port_t serverPort;	/* server request port */
    int command;		/* the specific command */
    int option;			/* user-specified option (e.g., length) */
    Sys_CharArray inBuf;	/* input buffer */
    mach_msg_type_number_t inBufLength;	/* bytes in inBuf */
    vm_address_t *outBufPtr;	/* OUT: results buffer */
    mach_msg_type_number_t *outBufLengthPtr; /* OUT: bytes in outBuf */
    boolean_t *outBufDeallocPtr; /* OUT: deallocate outBuf after reply */
    ReturnStatus *statusPtr;	/* OUT: Sprite return code */
    boolean_t *sigPendingPtr;	/* OUT: is there a pending signal */
{
#ifdef lint
    serverPort = serverPort;
#endif

    *statusPtr = VmCmdInband(command, option, (int)inBufLength,
			     (Address)inBuf, (int *)outBufLengthPtr,
			     (Address *)outBufPtr, outBufDeallocPtr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}
