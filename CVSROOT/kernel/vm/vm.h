/*
 * vm.h --
 *
 *     Virtual memory data structures and procedure headers exported by
 *     the virtual memory module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VM
#define _VM

#include "list.h"

#ifdef KERNEL
#include "user/vm.h"
#include "vmMach.h"
#include "vmStat.h"
#include "fs.h"
#include "procAOUT.h"
#include "sync.h"
#else
#include <kernel/vmMach.h>
#include <vmStat.h>
#include <kernel/fs.h>
#include <kernel/procAOUT.h>
#include <kernel/sync.h>
#endif

/*
 * Structure to represent a translated virtual address
 */
typedef struct {
    struct Vm_Segment	*segPtr;	/* Segment that address falls into.*/
    int 		page;		/* Virtual page. */
    int 		offset;		/* Offset in the page. */
    int			flags;		/* Flags defined below */
} Vm_VirtAddr;

/*
 * Values for flags field.  Lower 8 bits are for our use, next 8 bits are 
 * machine dependent.
 *
 *	VM_HEAP_PT_IN_USE	The heap segment for the current process had
 *				its page table marked as being in use.
 */
#define	VM_HEAP_PT_IN_USE	0x1
/*
 * A page table entry.
 */
typedef unsigned int	Vm_PTE;

/*
 * Flags to set and extract the fields of the PTE.
 *
 *	VM_VIRT_RES_BIT		The page is resident in the segments virtual
 *				address space.
 *	VM_PHYS_RES_BIT		The page is physically resident in memory.
 *      VM_ZERO_FILL_BIT	The page should be filled on demand with zeros.
 *	VM_ON_SWAP_BIT		The page is on swap space.
 *	VM_IN_PROGRESS_BIT	A page fault is occuring on this page.
 *	VM_COR_BIT		The page is copy-on-reference.
 *	VM_COW_BIT		The page is copy-on-write.
 *	VM_REFERENCED_BIT	The page has been referenced.
 *	VM_MODIFIED_BIT		The page has been modified.
 *      VM_READ_ONLY_PROT	The page is read-only.
 *	VM_COR_CHECK_BIT	The page is marked read-only after a cor fault
 *				to determine if the page will in fact get
 *				modified.
 *	VM_PAGE_FRAME_FIELD	The virtual page frame that this page is 
 *				resident in.
 */
#define	VM_VIRT_RES_BIT		0x80000000
#define VM_PHYS_RES_BIT		0x40000000
#define VM_ZERO_FILL_BIT	0x20000000
#define VM_ON_SWAP_BIT		0x10000000
#define VM_IN_PROGRESS_BIT	0x08000000
#define VM_COR_BIT		0x04000000
#define VM_COW_BIT		0x02000000
#define VM_REFERENCED_BIT	0x01000000
#define VM_MODIFIED_BIT		0x00800000
#define VM_READ_ONLY_PROT	0x00400000
#define VM_COR_CHECK_BIT	0x00200000
#define VM_PREFETCH_BIT		0x00100000
#define VM_PAGE_FRAME_FIELD	0x000fffff

/*
 * Macro to get a page frame out of a PTE.
 */
#define Vm_GetPageFrame(pte) ((unsigned int) ((pte) & VM_PAGE_FRAME_FIELD))

/*
 * The page size.
 */
extern	int	vm_PageSize;

/*
 * The type of accessibility desired when making a piece of data user
 * accessible.  VM_READONLY_ACCESS means that the data will only be read and
 * will not be written.  VM_OVERWRITE_ACCESS means that the entire block of
 * data will be overwritten.  VM_READWRITE_ACCESS means that the data 
 * will both be read and written.
 */
#define	VM_READONLY_ACCESS		1
#define	VM_OVERWRITE_ACCESS		2
#define	VM_READWRITE_ACCESS		3

/*
 * Structure that contains relevant info from the aout header to allow
 * reuse of sticky segments.
 */
typedef struct {
    int	heapPages;
    int	heapPageOffset;
    int	heapFileOffset;
    int	bssFirstPage;
    int	bssLastPage;
    int	entry;
} Vm_ExecInfo;

/*
 * The segment table structure.  Details about the segment table and
 * some of the fields in here are defined in vmInt.h.
 *
 * NOTE: Process migration requires that the five fields offset, fileAddr,
 *       type, numPages and ptSize be contiguous.
 */
typedef struct Vm_Segment {
    List_Links		links;		/* Links used to put the segment
					 * table entry in list of free segments,
					 * list of inactive segments or list
					 * of copy-on-write segments. */
    int			segNum;		/* The number of this segment. */
    int 		refCount;	/* Number of processes using this 
					 * segment */
    Sync_Condition	condition;	/* Condition to wait on for this
					 * segment. */
    Fs_Stream		*filePtr;	/* Pointer to the file that pages are
					 * being demanded loaded from. */
				        /* Name of object file for code 
					 * segments. */
    char		objFileName[VM_OBJ_FILE_NAME_LENGTH];
    Fs_Stream		*swapFilePtr;	/* Structure for an opened swap file.*/
    char		*swapFileName;  /* The filename associated with the
					 * swap file. */
    int			offset;		/* Explained in vmInt.h. */
    int			fileAddr;	/* The address in the object file where
					 * data or code for this segment 
					 * begins. */
    int           	type;		/* CODE, STACK, HEAP, or SYSTEM */
    int			numPages;	/* Explained in vmInt.h. */
    int			ptSize;		/* Number of pages in the page table */
    int			resPages;	/* Number of pages in physical memory
					 * for this segment. */
    Vm_PTE		*ptPtr;		/* Pointer to the page table for this 
					 * segment */
    struct VmMach_SegData *machPtr;	/* Pointer to machine dependent data */
    int			flags;		/* Flags to give information about the
					 * segment table entry. */
    List_Links		procListHdr;	/* Header node for list of processes
					 * sharing this segment. */
    List_Links		*procList;	/* Pointer to list of processes 
					 * sharing this segment. */
    int			ptUserCount;	/* The number of current users of this
					 * page table. */
    ClientData		fileHandle;	/* Handle for object file. */
    Vm_ExecInfo		execInfo;	/* Information to allow reuse of 
					 * sticky segments. */
    struct VmCOWInfo	*cowInfoPtr;	/* Pointer to copy-on-write list 
					 * header. */
    int			numCOWPages;	/* Number of copy-on-write pages that
					 * this segment references. */
    int			numCORPages;	/* Number of copy-on-ref pages that
					 * this segment references. */
    Address		minAddr;	/* Minimum address that the segment
					 * can ever have. */
    Address		maxAddr;	/* Maximium address that the segment
					 * can ever have. */
    int			traceTime;	/* The last trace interval that this
					 * segment was active. */
} Vm_Segment;

/*
 * Pointer to the system segment.
 */
extern 	Vm_Segment	*vm_SysSegPtr;

/*
 * Information stored by each process.
 */
typedef struct Vm_ProcInfo {
    Vm_Segment			*segPtrArray[VM_NUM_SEGMENTS];
    int				numMakeAcc;	/* Nesting level of make
						 * make accessibles for this
						 * process. */
    struct VmMach_ProcData	*machPtr;	/* Pointer to machine dependent
						 * data. */
    int				vmFlags;	/* Flags defined below. */
} Vm_ProcInfo;

/*
 * Values for the vmFlags field.
 *
 * VM_COPY_IN_PROGRESS          Data is being copied from/to this process
 *                              to/from the kernel's VAS.
 */
#define VM_COPY_IN_PROGRESS             0x01

/*
 * Maximum number of pages that a user process can wire down with the
 * Vm_PinUserMem call.
 */
#define	VM_MAX_USER_MAP_PAGES	4

/*
 * Copy-on-write level.
 */
extern	Boolean	vm_CanCOW;

/*
 * The initialization procedures.
 */
extern	void	Vm_BootInit();
extern	void	Vm_Init();

/*
 * Procedure for segments
 */
extern	void 	 	Vm_SegmentIncRef();
extern	Vm_Segment	*Vm_FindCode();
extern	void		Vm_InitCode();
extern	Vm_Segment  	*Vm_SegmentNew();
extern	ReturnStatus 	Vm_SegmentDup();
extern	void		Vm_SegmentDelete();
extern	void		Vm_ChangeCodeProt();
extern	ReturnStatus	Vm_DeleteFromSeg();

/*
 * Procedures for pages.
 */
extern	ReturnStatus	Vm_PageIn();
extern	void		Vm_PageOut();
extern	void		Vm_Clock();
extern	int		Vm_GetPageSize();

/*
 * Procedures for page tables.
 */
extern	void		Vm_ValidatePages();

/*
 * Procedure to allocate bytes of memory
 */
extern	Address		Vm_BootAlloc();
extern	Address		Vm_RawAlloc();

/*
 * Procedures for process migration.
 */
extern	ReturnStatus	Vm_InitiateMigration();
extern	ReturnStatus	Vm_EncapState();
extern	ReturnStatus	Vm_DeencapState();
extern	ReturnStatus	Vm_FinishMigration();

/*
 * Procedure for the file sytem.
 */
extern	int		Vm_MapBlock();
extern	int		Vm_UnmapBlock();
extern	void		Vm_FileChanged();
extern	void		Vm_FsCacheSize();

/*
 * System calls.
 */
extern	ReturnStatus	Vm_PageSize();
extern	ReturnStatus	Vm_CreateVA();
extern	ReturnStatus	Vm_DestroyVA();
extern	ReturnStatus	Vm_Cmd();
extern	ReturnStatus	Vm_GetSegInfo();

/*
 * Procedures to get to user addresses.
 */
extern	ReturnStatus	Vm_CopyIn();
extern	ReturnStatus	Vm_CopyOut();
extern	ReturnStatus	Vm_CopyInProc();
extern	ReturnStatus	Vm_CopyOutProc();
extern	ReturnStatus	Vm_StringNCopy();
extern	void		Vm_MakeAccessible();
extern	void		Vm_MakeUnaccessible();

/* 
 * Procedures for recovery.
 */
extern	void		Vm_OpenSwapDirectory();
extern	void		Vm_Recovery();

/*
 * Miscellaneous procedures.
 */
extern	Address		Vm_GetKernelStack();
extern	void		Vm_FreeKernelStack();
extern	void		Vm_ProcInit();
extern	ReturnStatus	Vm_PinUserMem();
extern	void		Vm_UnpinUserMem();
extern	void		Vm_ReservePage();

/*
 * Routines to provide access to internal virtual memory stuff for the machine
 * dependent code.
 */
extern	unsigned int	Vm_KernPageAllocate();
extern	void		Vm_KernPageFree();
extern	unsigned int	Vm_GetKernPageFrame();

/*
 * Virtual memory tracing routines are variables.
 */
extern	Boolean		vm_Tracing;
extern	void		Vm_StoreTraceTime();

#endif /* _VM */
