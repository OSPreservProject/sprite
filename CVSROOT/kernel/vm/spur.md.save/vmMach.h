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

#include "vmSpurConst.h"

typedef unsigned int	VmMachPTE;	

/*
 * Machine dependent data for each software segment.
 */
typedef struct VmMach_SegData {
    VmMachPTE	*ptBasePtr;	/* Base of segments page table. */
    VmMachPTE	*pt2BasePtr;	/* Base of page table that maps the segments
				 * page table. */
    int		firstPTPage;	/* First page table's page to be mapped. */
    int		lastPTPage;	/* Last page table's page to be mapped.  */
    int		RPTPM;		/* Root page table physical page number. */
    unsigned 	createTime;	/* The time that this segment was created. */
} VmMach_SegData;

/*
 * Machine dependent data for each process.
 */
typedef struct VmMach_ProcData {
    int			segNums[VMMACH_NUM_SEGMENTS];
    int			RPTPMs[VMMACH_NUM_SEGMENTS];
    unsigned int	segRegMask;
    unsigned int	mappedSegMask;
    struct Vm_Segment	*mapSegPtr;
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
extern	void		VmMach_SetupContext();
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
 * Device mapping.
 */
extern	Address		VmMach_MapInDevice();
extern	ReturnStatus	VmMach_MapKernelIntoUser();
/*
 * Tracing.
 */
extern	void		VmMach_Trace();
/*
 * Pinning and unpinning user memory pages.
 */
extern	void		VmMach_PinUserPage();
extern	void		VmMach_UnpinUserPage();

#endif _VMMACH
