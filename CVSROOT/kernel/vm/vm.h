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

#include <list.h>

#ifdef KERNEL
#include <user/vm.h>
#if 0
#include <vmMach.h>
#endif
#include <vmStat.h>
#include <fs.h>
#include <sync.h>
#include <proc.h>
#include <procMigrate.h>
#include <sprite.h>
#else
#if 0
#include <kernel/vmMach.h>
#endif
#include <vmStat.h>
#include <kernel/fs.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/procMigrate.h>
#include <sprite.h>
#endif

/*
 * Structure to represent a translated virtual address
 */
typedef struct Vm_VirtAddr {
    struct Vm_Segment	*segPtr;	/* Segment that address falls into.*/
    int 		page;		/* Virtual page. */
    int 		offset;		/* Offset in the page. */
    int			flags;		/* Flags defined below */
    struct Vm_SegProcList	*sharedPtr;	/* Pointer to shared seg. */
} Vm_VirtAddr;

/*
 * Values for flags field.  Lower 8 bits are for our use, next 8 bits are 
 * machine dependent.
 *
 *	VM_HEAP_PT_IN_USE	The heap segment for the current process had
 *				its page table marked as being in use.
 *	VM_READONLY_SEG		The segment is read only for this process.
 */
#define	VM_HEAP_PT_IN_USE	0x1
#define VM_READONLY_SEG		0x2
/*
 * A page table entry.
 */
typedef unsigned int	Vm_PTE;

/*
 * Flags to set and extract the fields of the PTE.
 *
 *	VM_VIRT_RES_BIT		The page is resident in the segment's virtual
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
 * The end of allocated kernel+data memory.
 */
extern	Address	vmMemEnd;

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
    List_Links			*sharedSegs;	/* Process's shared segs. */
    Address			sharedStart;	/* Start of shared region.  */
    Address			sharedEnd;	/* End of shared region.  */
} Vm_ProcInfo;

/*
 * List of the shared segments.
 * There is one of these entries for each shared segment.
 */
typedef struct Vm_SharedSegTable {
    List_Links          segList;        /* Links of shared segments. */
    int                 serverID;       /* Server of associated file. */
    int                 domain;         /* Domain of associated file. */
    int                 fileNumber;     /* File number of associated file. */
    struct Vm_Segment   *segPtr;        /* Shared segment. */
    int                 refCount;       /* Number of references to segment.
*/
} Vm_SharedSegTable;

/*
 * Shared segments associated with a process.
 * There is one of these entries for each processor-segment mapping.
 */
typedef struct Vm_SegProcList {
    List_Links          segList;        /* Links of shared segments. */
    int                 fd;             /* File descriptor of the mapping. */
    Vm_SharedSegTable   *segTabPtr;     /* Pointer to shared segment table. */
    Address             addr;           /* Start address of segment. */
    int			offset;		/* Page table offset (see vmInt.h). */
    int			fileAddr;	/* Offset into the file. */
    Address             mappedStart;    /* Start of mapped part. */
    Address             mappedEnd;      /* End of mapped part. */
    Fs_Stream           *stream;        /* Stream of mapping. */
    int                 prot;           /* Protections of segment. */
} Vm_SegProcList;

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
 * Maximum number of pageout processes.  This information is needed in
 * order to configure the correct number of Proc_ServerProcs
 */
#define VM_MAX_PAGE_OUT_PROCS	3

/*
 * The initialization procedures.
 */
extern void Vm_BootInit _ARGS_((void));
extern void Vm_Init _ARGS_((void));

/*
 * Procedure for segments
 */
extern void Vm_SegmentIncRef _ARGS_((Vm_Segment *segPtr, Proc_ControlBlock *procPtr));
extern Vm_Segment *Vm_FindCode _ARGS_((Fs_Stream *filePtr, Proc_ControlBlock *procPtr, Vm_ExecInfo **execInfoPtrPtr, Boolean *usedFilePtr));
extern void Vm_InitCode _ARGS_((Fs_Stream *filePtr, register Vm_Segment *segPtr, Vm_ExecInfo *execInfoPtr));
extern void Vm_FlushCode _ARGS_((Proc_ControlBlock *procPtr, Address addr, int numBytes));
extern Vm_Segment *Vm_SegmentNew _ARGS_((int type, Fs_Stream *filePtr, int fileAddr, int numPages, int offset, Proc_ControlBlock *procPtr));
extern ReturnStatus Vm_SegmentDup _ARGS_((register Vm_Segment *srcSegPtr, Proc_ControlBlock *procPtr, Vm_Segment **destSegPtrPtr));
extern void Vm_SegmentDelete _ARGS_((register Vm_Segment *segPtr, Proc_ControlBlock *procPtr));
extern void Vm_ChangeCodeProt _ARGS_((Proc_ControlBlock *procPtr, Address startAddr, int numBytes, Boolean makeWriteable));
extern ReturnStatus Vm_DeleteFromSeg _ARGS_((Vm_Segment *segPtr, int firstPage, int lastPage));

/*
 * Procedures for pages.
 */
extern ReturnStatus Vm_PageIn _ARGS_((Address virtAddr, Boolean protFault));
extern void Vm_Clock _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
extern int Vm_GetPageSize _ARGS_((void));
extern ReturnStatus Vm_TouchPages _ARGS_ ((int firstPage, int numPages));
ENTRY int Vm_GetRefTime _ARGS_ ((void));

/*
 * Procedures for page tables.
 */
extern void Vm_ValidatePages _ARGS_((Vm_Segment *segPtr, int firstPage, int lastPage, Boolean zeroFill, Boolean clobber));

/*
 * Procedure to allocate bytes of memory
 */
extern Address Vm_BootAlloc _ARGS_((int numBytes));
extern Address Vm_RawAlloc _ARGS_((int numBytes));

/*
 * Procedures for process migration.
 */
extern ReturnStatus Vm_InitiateMigration _ARGS_((Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr));
extern ReturnStatus Vm_EncapState _ARGS_((register Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr, Address bufferPtr));
extern ReturnStatus Vm_DeencapState _ARGS_((register Proc_ControlBlock *procPtr, Proc_EncapInfo *infoPtr, Address buffer));
extern ReturnStatus Vm_FinishMigration _ARGS_((register Proc_ControlBlock *procPtr, int hostID, Proc_EncapInfo *infoPtr, Address bufferPtr, int failure));
extern ReturnStatus Vm_EncapSegInfo _ARGS_((int segNum,
	Vm_SegmentInfo *infoPtr));

/*
 * Procedure for the file system.
 */
extern int Vm_MapBlock _ARGS_((Address addr));
extern int Vm_UnmapBlock _ARGS_((Address addr, Boolean retOnePage, unsigned int *pageNumPtr));
extern void Vm_FileChanged _ARGS_((Vm_Segment **segPtrPtr));
extern void Vm_FsCacheSize _ARGS_((Address *startAddrPtr, Address *endAddrPtr));

/*
 * System calls.
 */
extern ReturnStatus Vm_PageSize _ARGS_((int *pageSizePtr));
extern ReturnStatus Vm_CreateVA _ARGS_((Address address, int size));
extern ReturnStatus Vm_DestroyVA _ARGS_((Address address, int size));
extern ReturnStatus Vm_Cmd _ARGS_((int command, int arg));
extern ReturnStatus Vm_GetSegInfo _ARGS_((Proc_PCBInfo *infoPtr,
	Vm_SegmentID segID, int infoSize, Address segBufPtr));

/*
 * Procedures to get to user addresses.
 */
extern ReturnStatus Vm_CopyIn _ARGS_((register int numBytes,
	Address sourcePtr, Address destPtr));
extern ReturnStatus Vm_CopyOut _ARGS_((register int numBytes,
	Address sourcePtr, Address destPtr));
extern ReturnStatus Vm_CopyInProc _ARGS_((int numBytes,
	register Proc_ControlBlock *fromProcPtr, Address fromAddr,
	Address toAddr, Boolean toKernel));
extern ReturnStatus Vm_CopyOutProc _ARGS_((int numBytes, Address fromAddr,
	Boolean fromKernel, register Proc_ControlBlock *toProcPtr,
	Address toAddr));
extern ReturnStatus Vm_StringNCopy _ARGS_((int numBytes,
	Address sourcePtr, Address destPtr, int *bytesCopiedPtr));
extern void Vm_MakeAccessible _ARGS_((int accessType, int numBytes,
	Address startAddr, register int *retBytesPtr,
	register Address *retAddrPtr));
extern void Vm_MakeUnaccessible _ARGS_((Address addr, int numBytes));

/* 
 * Procedures for recovery.
 */
extern void Vm_OpenSwapDirectory _ARGS_((ClientData data,
	Proc_CallInfo *callInfoPtr));
extern void Vm_Recovery _ARGS_((void));

/*
 * Miscellaneous procedures.
 */
extern Address Vm_GetKernelStack _ARGS_((int invalidPage));
extern void Vm_FreeKernelStack _ARGS_((Address stackBase));
extern void Vm_ProcInit _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus Vm_PinUserMem _ARGS_((int mapType, int numBytes,
	register Address addr));
extern void Vm_UnpinUserMem _ARGS_((int numBytes, Address addr));
extern void Vm_ReservePage _ARGS_((unsigned int pfNum));
extern Boolean VmMach_VirtAddrParse _ARGS_((Proc_ControlBlock *procPtr,
	Address virtAddr, register Vm_VirtAddr *transVirtAddrPtr));

/*
 * Routines to provide access to internal virtual memory stuff for the machine
 * dependent code.
 */
extern unsigned int Vm_KernPageAllocate _ARGS_((void));
extern void Vm_KernPageFree _ARGS_((unsigned int pfNum));
extern unsigned int Vm_GetKernPageFrame _ARGS_((int pageFrame));

/*
 * Virtual memory tracing routines and variables.
 */
extern	Boolean		vm_Tracing;
extern void Vm_StoreTraceTime _ARGS_((Timer_Ticks timeStamp));

/*
 * Shared memory routines.
 */
extern ReturnStatus Vm_Mmap _ARGS_((Address startAddr, int length, int prot,
	int share, int streamID, int fileAddr, Address *mappedAddr));
extern ReturnStatus Vm_Munmap _ARGS_((Address startAddr, int length,
	int noError));
extern ReturnStatus Vm_Msync _ARGS_((Address startAddr, int length));
extern ReturnStatus Vm_Mlock _ARGS_((Address startAddr, int length));
extern ReturnStatus Vm_Munlock _ARGS_((Address startAddr, int length));
extern ReturnStatus Vm_Mincore _ARGS_((Address startAddr, int length,
	char *retVec));
extern ReturnStatus Vm_Mprotect _ARGS_((Address startAddr, int length,
	int prot));
extern void Vm_CleanupSharedFile _ARGS_((Proc_ControlBlock *procPtr,
	Fs_Stream *streamPtr));
extern void Vm_CleanupSharedProc _ARGS_((Proc_ControlBlock *procPtr));
extern void Vm_DeleteSharedSegment _ARGS_((Proc_ControlBlock *procPtr,
	Vm_SegProcList *segProcPtr));
extern void Vm_CopySharedMem _ARGS_((Proc_ControlBlock *parentProcPtr,
	Proc_ControlBlock *childProcPtr));

/*
 * Machine-dependent routines exported to other modules.
 */
/*
 * Device mapping.
 */
extern Address VmMach_DMAAlloc _ARGS_((int numBytes, Address srcAddr));
extern void VmMach_DMAFree _ARGS_((int numBytes, Address mapAddr));
extern ReturnStatus VmMach_MapKernelIntoUser _ARGS_((unsigned int
        kernelVirtAddr, int numBytes, unsigned int userVirtAddr,
        unsigned int *realVirtAddrPtr));

/*
 * Routines to manage contexts.
 */
extern void VmMach_FreeContext _ARGS_((register Proc_ControlBlock *procPtr));
extern void VmMach_ReinitContext _ARGS_((register Proc_ControlBlock *procPtr));
extern ClientData VmMach_SetupContext _ARGS_((register Proc_ControlBlock
        *procPtr));

/*
 * Initialization
 */
extern void VmMach_BootInit _ARGS_((int *pageSizePtr, int *pageShiftPtr,
        int *pageTableIncPtr, int *kernMemSizePtr, int *numKernPagesPtr,
        int *maxSegsPtr, int *maxProcessesPtr));
extern Address VmMach_AllocKernSpace _ARGS_((Address baseAddr));
extern void VmMach_Init _ARGS_((int firstFreePage));

/*
 * Segment creation, expansion, and destruction.
 */
extern void VmMach_SegInit _ARGS_((struct Vm_Segment *segPtr));
extern void VmMach_SegExpand _ARGS_((register struct Vm_Segment *segPtr,
        int firstPage, int lastPage));
extern void VmMach_SegDelete _ARGS_((register struct Vm_Segment *segPtr));

/*
 * Process initialization.
 */
extern void VmMach_ProcInit _ARGS_((register struct Vm_ProcInfo *vmPtr));

/*
 * Manipulating protection.
 */
extern void VmMach_SetSegProt _ARGS_((register struct Vm_Segment *segPtr,
        register int firstPage, int lastPage, Boolean makeWriteable));
extern void VmMach_SetPageProt _ARGS_((register struct Vm_VirtAddr
        *virtAddrPtr, Vm_PTE softPTE));

/*
 * Reference and modify bits.
 */
extern void VmMach_GetRefModBits _ARGS_((register struct Vm_VirtAddr
        *virtAddrPtr, unsigned int virtFrameNum, register Boolean *refPtr,
        register Boolean *modPtr));
extern void VmMach_ClearRefBit _ARGS_((register struct Vm_VirtAddr
	*virtAddrPtr, unsigned int virtFrameNum));
extern void VmMach_ClearModBit _ARGS_((register struct Vm_VirtAddr
	*virtAddrPtr, unsigned int virtFrameNum));
extern void VmMach_AllocCheck _ARGS_((register struct Vm_VirtAddr
	*virtAddrPtr, unsigned int virtFrameNum, register Boolean *refPtr,
        register Boolean *modPtr));

/*
 * Page validation and invalidation.
 */
extern void VmMach_PageValidate _ARGS_((register struct Vm_VirtAddr
	*virtAddrPtr, Vm_PTE pte));
extern void VmMach_PageInvalidate _ARGS_((register struct Vm_VirtAddr
	*virtAddrPtr, unsigned int virtPage, Boolean segDeletion));

/*
 * Routine to parse a virtual address.
 */
extern Boolean VmMach_VirtAddrParse _ARGS_((Proc_ControlBlock *procPtr,
        Address virtAddr, register struct Vm_VirtAddr *transVirtAddrPtr));

/*
 * Routines to copy data to/from user space.
 */
extern ReturnStatus VmMach_CopyInProc _ARGS_((int numBytes,
        Proc_ControlBlock *fromProcPtr, Address fromAddr,
        struct Vm_VirtAddr *virtAddrPtr, Address toAddr, Boolean toKernel));
extern ReturnStatus VmMach_CopyOutProc _ARGS_((int numBytes,
        Address fromAddr, Boolean fromKernel, Proc_ControlBlock *toProcPtr,
        Address toAddr, struct Vm_VirtAddr *virtAddrPtr));

/*
 * Tracing.
 */
extern void VmMach_Trace _ARGS_((void));

/*
 * Pinning and unpinning user memory pages.
 */
extern void VmMach_PinUserPages _ARGS_((int mapType, struct Vm_VirtAddr
        *virtAddrPtr, int lastPage));
extern void VmMach_UnpinUserPages _ARGS_((struct Vm_VirtAddr *virtAddrPtr,
        int lastPage));
/*
 * Cache flushing.
 */
extern void VmMach_FlushPage _ARGS_((struct Vm_VirtAddr *virtAddrPtr,
        Boolean invalidate));
extern void VmMach_FlushCode _ARGS_((Proc_ControlBlock *procPtr,
        struct Vm_VirtAddr *virtAddrPtr, unsigned virtPage, int numBytes));
extern void VmMach_FlushByteRange _ARGS_((Address virtAddr, int numBytes));
/*
 * Migration.
 */
extern void VmMach_HandleSegMigration _ARGS_((struct Vm_Segment *segPtr));

extern ReturnStatus VmMach_Cmd _ARGS_((int command, int arg));

/*
 * Shared memory.
 */
extern void VmMach_SharedSegFinish _ARGS_((Proc_ControlBlock *procPtr,
        Address addr));
extern void VmMach_SharedProcStart _ARGS_((Proc_ControlBlock *procPtr));
extern void VmMach_SharedProcFinish _ARGS_((Proc_ControlBlock *procPtr));
extern void VmMach_CopySharedMem _ARGS_((Proc_ControlBlock *parentProcPtr,
        Proc_ControlBlock *childProcPtr));
extern ReturnStatus VmMach_SharedStartAddr _ARGS_((Proc_ControlBlock *procPtr,
        int size, Address *reqAddr));


#endif /* _VM */
