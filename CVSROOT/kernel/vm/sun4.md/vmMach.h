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
 * Since the index into the segment table is passed to various
 * machine-dependent routines, its size must depend on the hardware size
 * or else be the largest size used, which would waste space.  So far, this
 * has only been generalized for the various types of sun4s.
 */
#ifdef sun4
#ifdef sun4c
typedef	unsigned short	VMMACH_SEG_NUM;	/* should be unsigned char, but fails */
#else
typedef	unsigned short	VMMACH_SEG_NUM;
#endif	/* sun4c */
#endif	/* sun4 */

/*
 * Machine dependent data for each software segment.  The data for each 
 * segment is a hardware segment table that contains one entry for each
 * hardware segment in the software segment's virtual address space.
 * A hardware context is made up of several software segments.  The offset
 * field in this struct is the offset into the context where this software
 * segment begins.  The offset is in terms of hardware segments.
 */
typedef struct VmMach_SegData {
#ifdef sun4
    VMMACH_SEG_NUM	*segTablePtr;
#else
    unsigned char 	*segTablePtr;
#endif /* sun4 */
    int			offset;
    int			numSegs;
} VmMach_SegData;

/*
 * Machine dependent shared memory data
 */
typedef struct VmMach_SharedData {
    int        *allocVector;           /* Allocated block vector. */
    int         allocFirstFree;         /* First free block. */
} VmMach_SharedData;

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
    VmMach_SharedData   sharedData;     /* Data for shared memory. */
} VmMach_ProcData;

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
extern	void		VmMach_AllocCheck();		
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
extern	void		VmMach_GetDevicePage();
extern	ReturnStatus	VmMach_MapKernelIntoUser();
extern	Address		VmMach_DMAAlloc();
extern	void		VmMach_DMAFree();
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
extern	void		VmMach_FlushCode();

extern	ReturnStatus	VmMach_Cmd();

#if defined(sun3) || defined(sun4)
/*
 * Network mapping routines for the Sun 3.
 */
extern	Address	VmMach_NetMemAlloc();
extern	void	VmMach_NetMapPacket();
#endif

/*
 * Shared memory.
 */

extern  ReturnStatus    VmMach_SharedStartAddr();
extern  void            VmMach_SharedSegFinish();
extern  void            VmMach_SharedProcStart();
extern  void            VmMach_SharedProcFinish();


#endif /* _VMMACH */
