/* 
 * devSmem.c --
 *
 *	Stubs to implement /dev/smem.  Allow reading and writing
 *      to kernel memory.
 *
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include <vmPmaxConst.h>
#include <dbg.h>
#define NOGAP

static char smemBuf[1024];
extern int vm_PageSize;
extern int mach_KernStackSize;
extern int vmBlockCacheEndAddr;
extern int vmBlockCacheBaseAddr;
extern int vmStackEndAddr;
extern int vmStackBaseAddr;
extern int vmMemEnd;
extern int vmBootEnd;
extern int mach_KernStart;
extern int mach_CodeStart;
extern Mach_DebugState mach_DebugState;
static void     DebugToRegState _ARGS_((Mach_DebugState *debugPtr,
                        Mach_RegState *regPtr));


/*
 *----------------------------------------------------------------------
 *
 *  Dev_SmemRead --
 *
 *	Return number of bytes read and SUCCESS if nonzero bytes returned.
 *      Return SYS_ARG_NOACCESS if supplied address is unreadable, and
 *      SYS_INVALID_ARG if supplied address is not valid.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SmemRead(devicePtr, readPtr, replyPtr)
    Fs_Device *devicePtr;
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
  int status, bytesLeft;
  int kernelAddress;
  int numPages, bytesInPage;
  char *bufPtr;
  StopInfo stopInfo;
  Dbg_DumpBounds currentBounds;
  
  stopInfo.codeStart = (int)mach_CodeStart;
  stopInfo.trapType = (mach_DebugState.causeReg & MACH_CR_EXC_CODE) >>
    MACH_CR_EXC_CODE_SHIFT;
  DebugToRegState(&mach_DebugState, &(stopInfo.regs));
  currentBounds.pageSize = vm_PageSize;
  currentBounds.stackSize = mach_KernStackSize;
  currentBounds.kernelCodeStart = (unsigned int) mach_KernStart;
  currentBounds.kernelCodeSize  =
    (unsigned int) (vmBootEnd - mach_KernStart);
  currentBounds.kernelDataStart  = VMMACH_VIRT_CACHED_START;
  currentBounds.kernelDataSize   = (unsigned int)
    (vmMemEnd - VMMACH_VIRT_CACHED_START);
  currentBounds.kernelStacksStart = (unsigned int)vmStackBaseAddr;
  currentBounds.kernelStacksSize = (unsigned int)
    (vmStackEndAddr - vmStackBaseAddr);
  currentBounds.fileCacheStart   = (unsigned int)vmBlockCacheBaseAddr;
  currentBounds.fileCacheSize    = (unsigned int) (vmBlockCacheEndAddr -
					    vmBlockCacheBaseAddr);
  kernelAddress = readPtr->offset;
  bytesLeft = readPtr->length;
  bufPtr = readPtr->buffer;

  /* Get part or all of stop info if requested. */
  if (kernelAddress < sizeof(StopInfo)) {
    if (bytesLeft + kernelAddress <= sizeof(StopInfo)) {
      bcopy(((char *)&stopInfo) + kernelAddress, readPtr->buffer, readPtr->length);
      replyPtr->length = readPtr->length;
      return(SUCCESS);
    }
    bcopy(((char *)&stopInfo) + kernelAddress, readPtr->buffer, sizeof(StopInfo) - kernelAddress);
    bytesLeft -= sizeof(StopInfo) - kernelAddress;
    bufPtr += sizeof(StopInfo) - kernelAddress;
    kernelAddress = sizeof(StopInfo);
  }
  
  /* Get part or all of dump bounds if requested. */
  if (kernelAddress < (sizeof(StopInfo) + sizeof(Dbg_DumpBounds))) {
    if (bytesLeft + kernelAddress <= sizeof(StopInfo) + sizeof(Dbg_DumpBounds)) {
      bcopy(((char *)&currentBounds) + sizeof(StopInfo) - kernelAddress, bufPtr, bytesLeft);
      replyPtr->length = readPtr->length;
      return(SUCCESS);
    }
    bcopy(((char *)&currentBounds) + sizeof(StopInfo) - kernelAddress, bufPtr, sizeof(Dbg_DumpBounds));
    bytesLeft -= sizeof(StopInfo) + sizeof(Dbg_DumpBounds) - kernelAddress;
    bufPtr += sizeof(StopInfo) + sizeof(Dbg_DumpBounds) - kernelAddress;
    kernelAddress = sizeof(StopInfo) + sizeof(Dbg_DumpBounds);
  }

  /* Set address to beginning of kernel memory. */
  kernelAddress += mach_KernStart - (sizeof(StopInfo) + sizeof(Dbg_DumpBounds));

  /* Make sure offset is valid. */
#ifdef NOGAP
  if (kernelAddress > vmBootEnd) {
    kernelAddress += VMMACH_VIRT_CACHED_START - vmBootEnd;
  }
  if (kernelAddress > vmMemEnd) {
    kernelAddress += vmStackBaseAddr - vmMemEnd;
  }
  if (kernelAddress > vmStackEndAddr) {
    kernelAddress += vmBlockCacheBaseAddr - vmStackEndAddr;
  }
  if (kernelAddress > vmBlockCacheEndAddr) {
    return (SYS_INVALID_ARG);
  }
#else
  if (kernelAddress > vmBootEnd &&
      kernelAddress < VMMACH_VIRT_CACHED_START) {
    return(SYS_INVALID_ARG);
  }n
  if (kernelAddress > vmMemEnd &&
      kernelAddress < vmStackBaseAddr) {
    return(SYS_INVALID_ARG);
  }
  if (kernelAddress > vmStackEndAddr &&
      kernelAddress < vmBlockCacheBaseAddr) {
    return(SYS_INVALID_ARG);
  }
  if (kernelAddress > vmBlockCacheEndAddr) {
    return(SYS_INVALID_ARG);
  }
#endif
  
  /* Find number of pages that request spans. */
  numPages = ((kernelAddress + bytesLeft - 1) >> VMMACH_PAGE_SHIFT) - 
    (kernelAddress >> VMMACH_PAGE_SHIFT);
  if (!Dbg_InRange(kernelAddress, bytesLeft, FALSE)) {
    replyPtr->length = 0;
    return(SYS_ARG_NOACCESS);
  }
  bytesInPage = vm_PageSize - (kernelAddress & (vm_PageSize-1));
  if (bytesLeft < bytesInPage) {
    bcopy(kernelAddress, bufPtr, bytesLeft);
    replyPtr->length = readPtr->length;
    return(SUCCESS);
  }
  bcopy(kernelAddress, bufPtr, bytesInPage);
  bytesLeft -= bytesInPage;
  bufPtr += vm_PageSize;

  /* Make each page accessible, if possible and copy it into 
     return buffer. */
  while (numPages > 0) {
    if (!Dbg_InRange(kernelAddress, bytesLeft, FALSE)) {
      replyPtr->length = 0;
      return(SYS_ARG_NOACCESS);
    }
    if (bytesLeft < vm_PageSize) {
      bcopy(kernelAddress, bufPtr, bytesLeft);
      replyPtr->length = readPtr->length;
      return(SUCCESS);
    }
    bcopy(kernelAddress, bufPtr, vm_PageSize);
    bytesLeft -= vm_PageSize;
    bufPtr += vm_PageSize;
    numPages--;
  }
  replyPtr->length = readPtr->length;
  return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_SmemWrite --
 *
 *	Writes if it can, and returns SUCCESS if it wrote.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Dev_SmemWrite(devicePtr, writePtr, replyPtr)
    Fs_Device *devicePtr;
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
  int status, bytesLeft;
  int kernelAddress;
  int numPages, bytesInPage;
  char *bufPtr;
  StopInfo stopInfo;
  Dbg_DumpBounds currentBounds;
  
  stopInfo.codeStart = (int)mach_CodeStart;
  stopInfo.trapType = (mach_DebugState.causeReg & MACH_CR_EXC_CODE) >>
    MACH_CR_EXC_CODE_SHIFT;
  DebugToRegState(&mach_DebugState, &(stopInfo.regs));
  currentBounds.pageSize = vm_PageSize;
  currentBounds.stackSize = mach_KernStackSize;
  currentBounds.kernelCodeStart = (unsigned int) mach_KernStart;
  currentBounds.kernelCodeSize  =
    (unsigned int) (vmBootEnd - mach_KernStart);
  currentBounds.kernelDataStart  = VMMACH_VIRT_CACHED_START;
  currentBounds.kernelDataSize   = (unsigned int)
    (vmMemEnd - VMMACH_VIRT_CACHED_START);
  currentBounds.kernelStacksStart = (unsigned int)vmStackBaseAddr;
  currentBounds.kernelStacksSize = (unsigned int)
    (vmStackEndAddr - vmStackBaseAddr);
  currentBounds.fileCacheStart   = (unsigned int)vmBlockCacheBaseAddr;
  currentBounds.fileCacheSize    = (unsigned int) (vmBlockCacheEndAddr -
					    vmBlockCacheBaseAddr);
  kernelAddress = writePtr->offset;
  bytesLeft = writePtr->length;
  bufPtr = writePtr->buffer;

  /* Get part or all of stop info if requested. */
  if (kernelAddress < sizeof(StopInfo)) {
    if (bytesLeft + kernelAddress <= sizeof(StopInfo)) {
      bcopy(writePtr->buffer, ((char *)&stopInfo) + kernelAddress, writePtr->length);
      replyPtr->length = writePtr->length;
      return(SUCCESS);
    }
    bcopy(writePtr->buffer, ((char *)&stopInfo) + kernelAddress, sizeof(StopInfo) - kernelAddress);
    bytesLeft -= sizeof(StopInfo) - kernelAddress;
    bufPtr += sizeof(StopInfo) - kernelAddress;
    kernelAddress = sizeof(StopInfo);
  }
  
  /* Get part or all of dump bounds if requested. */
  if (kernelAddress < (sizeof(StopInfo) + sizeof(Dbg_DumpBounds))) {
    if (bytesLeft + kernelAddress <= sizeof(StopInfo) + sizeof(Dbg_DumpBounds)) {
      bcopy(bufPtr, ((char *)&currentBounds) + sizeof(StopInfo) - kernelAddress, bytesLeft);
      replyPtr->length = writePtr->length;
      return(SUCCESS);
    }
    bcopy(bufPtr, ((char *)&currentBounds) + sizeof(StopInfo) - kernelAddress, sizeof(Dbg_DumpBounds));
    bytesLeft -= sizeof(StopInfo) + sizeof(Dbg_DumpBounds) - kernelAddress;
    bufPtr += sizeof(StopInfo) + sizeof(Dbg_DumpBounds) - kernelAddress;
    kernelAddress = sizeof(StopInfo) + sizeof(Dbg_DumpBounds);
  }

  /* Set address to beginning of kernel memory. */
  kernelAddress += mach_KernStart - (sizeof(StopInfo) + sizeof(Dbg_DumpBounds));

  /* Make sure offset is valid. */
#ifdef NOGAP
  if (kernelAddress > vmBootEnd) {
    kernelAddress += VMMACH_VIRT_CACHED_START - vmBootEnd;
  }
  if (kernelAddress > vmMemEnd) {
    kernelAddress += vmStackBaseAddr - vmMemEnd;
  }
  if (kernelAddress > vmStackEndAddr) {
    kernelAddress += vmBlockCacheBaseAddr - vmStackEndAddr;
  }
  if (kernelAddress > vmBlockCacheEndAddr) {
    return (SYS_INVALID_ARG);
  }
#else
  if (kernelAddress > vmBootEnd &&
      kernelAddress < VMMACH_VIRT_CACHED_START) {
    return(SYS_INVALID_ARG);
  }n
  if (kernelAddress > vmMemEnd &&
      kernelAddress < vmStackBaseAddr) {
    return(SYS_INVALID_ARG);
  }
  if (kernelAddress > vmStackEndAddr &&
      kernelAddress < vmBlockCacheBaseAddr) {
    return(SYS_INVALID_ARG);
  }
  if (kernelAddress > vmBlockCacheEndAddr) {
    return(SYS_INVALID_ARG);
  }
#endif
  
  /* Find number of pages that request spans. */
  numPages = ((kernelAddress + bytesLeft - 1) >> VMMACH_PAGE_SHIFT) - 
    (kernelAddress >> VMMACH_PAGE_SHIFT);
  if (!Dbg_InRange(kernelAddress, bytesLeft, FALSE)) {
    replyPtr->length = 0;
    return(SYS_ARG_NOACCESS);
  }
  bytesInPage = vm_PageSize - (kernelAddress & (vm_PageSize-1));
  if (bytesLeft < bytesInPage) {
    bcopy(bufPtr, kernelAddress, bytesLeft);
    replyPtr->length = writePtr->length;
    return(SUCCESS);
  }
  bcopy(bufPtr, kernelAddress, bytesInPage);
  bytesLeft -= bytesInPage;
  bufPtr += vm_PageSize;

  /* Make each page accessible, if possible and copy it into 
     return buffer. */
  while (numPages > 0) {
    if (!Dbg_InRange(kernelAddress, bytesLeft, FALSE)) {
      replyPtr->length = 0;
      return(SYS_ARG_NOACCESS);
    }
    if (bytesLeft < vm_PageSize) {
      bcopy(bufPtr, kernelAddress, bytesLeft);
      replyPtr->length = writePtr->length;
      return(SUCCESS);
    }
    bcopy(bufPtr, kernelAddress, vm_PageSize);
    bytesLeft -= vm_PageSize;
    bufPtr += vm_PageSize;
    numPages--;
  }
  replyPtr->length = writePtr->length;
  return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SmemIOControl --
 *
 *	This procedure handles IOControls for /dev/smem and other
 *	devices.  It refuses all IOControls except for a few of
 *	the generic ones, for which it does nothing.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Dev_SmemIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	        *devicePtr;
    Fs_IOCParam		*ioctlPtr;
    Fs_IOReply		*replyPtr;
{
    if ((ioctlPtr->command == IOC_GET_FLAGS)
	|| (ioctlPtr->command == IOC_SET_FLAGS)
	|| (ioctlPtr->command == IOC_SET_BITS)
	|| (ioctlPtr->command == IOC_CLEAR_BITS)
	|| (ioctlPtr->command == IOC_REPOSITION)) {
	return SUCCESS;
    }
    return GEN_NOT_IMPLEMENTED;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SmemSelect --
 *
 *	This procedure handles selects for /dev/smem and other
 *	devices that are always ready.
 *
 * Results:
 *	The device is indicated to be readable and writable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Dev_SmemSelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device	*devicePtr;	/* Ignored. */
    int	*readPtr;		/* Read bit to clear if not readable */
    int	*writePtr;		/* Write bit to clear if not readable */
    int	*exceptPtr;		/* Except bit to clear if not readable */
{
    *exceptPtr = 0;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DebugToRegState --
 *
 *      Converts a Mach_DebugState to Mach_RegState.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static void
DebugToRegState(debugPtr, regPtr)
    Mach_DebugState     *debugPtr;
    Mach_RegState       *regPtr;
{
    regPtr->pc = (Address) debugPtr->excPC;
    bcopy((char *) debugPtr->regs, (char *) regPtr->regs,
        MACH_NUM_GPRS * sizeof(int));
    bcopy((char *) debugPtr->fpRegs, (char *) regPtr->fpRegs,
        MACH_NUM_FPRS * sizeof(int));
    regPtr->fpStatusReg = debugPtr->fpCSR;
    regPtr->mfhi = debugPtr->multHi;
    regPtr->mflo = debugPtr->multLo;
}
