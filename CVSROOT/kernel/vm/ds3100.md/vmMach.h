/*
 * vmMach.h
 *
 *     	Machine dependent virtual memory data structures and procedure
 *	headers.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#ifndef _VMMACH
#define _VMMACH

/*
 * Machine dependent data for each software segment.
 */
typedef struct VmMach_SegData {
    int	dummy;
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
    struct Vm_Segment	*mapSegPtr;	/* Pointer to segment which is mapped
					 * into this processes address
					 * space. */
    unsigned int	mappedPage;	/* Page in the mapped seg where
					 * the mapping begins. */
    int			pid;		/* Which pid is used to map this
					 * process. */
    unsigned int	modPage;	/* A TLB modified fault occured on this
					 * virtual page - set the modify bit
					 * in the TLB entry if we try to
					 * validate this VA. */
    VmMach_SharedData	sharedData;	/* Data for shared memory. */
} VmMach_ProcData;

/*
 * TLB Map.
 */
extern unsigned *vmMach_KernelTLBMap;

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
extern	ReturnStatus	VmMach_MapKernelIntoUser();
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
/*
 * Routines to map and unmap kernel data into/out-of the user's address space.
 */
extern	Address		VmMach_UserMap();
extern	void		VmMach_UserUnmap();

extern	ReturnStatus	VmMach_Cmd();
extern	void		VmMach_Trace();
extern	void		VmMach_MakeNonCacheable();

/*
 * Shared memory.
 */ 
extern  ReturnStatus	VmMach_SharedStartAddr(); 
extern  void		VmMach_SharedSegFinish(); 
extern	void		VmMach_SharedProcStart();
extern	void		VmMach_SharedProcFinish();

#endif /* _VMMACH */
