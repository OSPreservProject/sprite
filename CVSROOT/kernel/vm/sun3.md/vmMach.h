/*
 * vmMach.h
 *
 *     	Machine dependent virtual memory data structures and procedure 
 *	headers.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMMACH
#define _VMMACH

#ifdef KERNEL
#include "vmSunConst.h"
#else
#include <kernel/vmSunConst.h>
#endif

/*
 * Machine dependent data for each software segment.  The data for each 
 * segment is a hardware segment table that contains one entry for each
 * hardware segment in the software segment's virtual address space.
 * A hardware context is made up of several software segments.  The offset
 * field in this struct is the offset into the context where this software
 * segment begins.  The offset is in terms of hardware segments.
 */
typedef struct VmMach_SegData {
    unsigned char 	*segTablePtr;
    int			offset;
    int			numSegs;
} VmMach_SegData;

/*
 * Machine dependent data for each process.
 */
typedef struct VmMach_ProcData {
    struct VmMach_Context *contextPtr;	/* The context for the process. */
    struct Vm_Segment	*mapSegPtr;	/* Pointer to segment which is mapped
					 * into this processes address
					 * space. */
    unsigned int	mapHardSeg;	/* Address in the mapped seg where 
					 * the mapping begins. */
} VmMach_ProcData;

/*
 * Device buffer structure.
 */
typedef struct {
    Address	baseAddr;	/* Base virtual address to start 
				   allocating at. */
    Address	endAddr;	/* Last possible virtual address plus one. */
} VmMach_DevBuffer;

/*
 * Machine dependent functions.
 */
/*
 * Initialization
 */
extern	void		VmMach_BootInit();
extern	Address		VmMach_AllocKernSpace();
extern	void		VmMach_Init();
/*
 * Segment creation, expansion, and destruction.
 */
extern	void		VmMach_SegInit();
extern	void		VmMach_SegExpand();
extern	void		VmMach_SegDelete();
/*
 * Process initialization.
 */
extern	void		VmMach_ProcInit();
/*
 * Manipulating protection.
 */
extern	void		VmMach_SetSegProt();
extern	void		VmMach_SetPageProt();
extern	void		VmMach_SetProtForDbg();
/*
 * Reference and modify bits.
 */
extern	void		VmMach_GetRefModBits();
extern	void		VmMach_ClearRefBit();
extern	void		VmMach_ClearModBit();		
/*
 * Page validation and invalidation.
 */
extern	void		VmMach_PageValidate();
extern	void		VmMach_PageInvalidate();
/*
 * Routine to parse a virtual address.
 */
extern	Boolean		VmMach_VirtAddrParse();

/*
 * Routines to manage contexts.
 */
extern	ClientData	VmMach_SetupContext();
extern	void		VmMach_FreeContext();
extern	void		VmMach_ReinitContext();
/*
 * Routines to copy data to/from user space.
 */
extern	ReturnStatus	VmMach_CopyIn();
extern	ReturnStatus	VmMach_CopyOut();
extern	ReturnStatus	VmMach_CopyInProc();
extern	ReturnStatus	VmMach_CopyOutProc();
extern	ReturnStatus	VmMach_StringNCopy();
/*
 * Routines for the INTEL device driver.
 */
extern	void		VmMach_MapIntelPage();
extern	void		VmMach_UnmapIntelPage();
/*
 * Device mapping.
 */
extern	Address		VmMach_MapInDevice();
extern	void		VmMach_DevBufferInit();
extern	Address		VmMach_DevBufferAlloc();
extern	Address		VmMach_DevBufferMap();
extern	void		VmMach_GetDevicePage();
extern	ReturnStatus	VmMach_MapKernelIntoUser();
/*
 * Tracing.
 */
extern	void		VmMach_Trace();
/*
 * Pinning and unpinning user memory pages.
 */
extern	void		VmMach_PinUserPages();
extern	void		VmMach_UnpinUserPages();
/*
 * Cache flushing.
 */
extern	void		VmMach_FlushPage();

#endif _VMMACH
