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

#include "vmMach.h"
#include "vmStat.h"
#include "list.h"
#include "fs.h"
#include "procAOUT.h"

/*
 * A pte with a zero value.
 */

Vm_PTE	vm_ZeroPTE;

/*
 * The type of segment.
 */

#define VM_SYSTEM	0
#define VM_CODE		1
#define VM_HEAP		2
#define VM_STACK	3

/*
 * All kernel code and data is stored in the system segment which is
 * the first segment in the segment table.  Hence it has segment id 0.
 */

#define VM_SYSTEM_SEGMENT       0

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
 * Values for the vm flags in the proc table.
 *
 * No flags currently but if there are these must be in the low order two
 * bytes because machine dependent ones are the high order two bytes.
 */

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

#define	VM_OBJ_FILE_NAME_LENGTH	50

/*
 * The segment table structure.  This shouldn't be external but lint
 * complains like crazy if we try to hide it.  So to make lint happy ...
 */

typedef struct Vm_Segment {
    List_Links		links;		/* Links used to put the segment
    					   table entry in list of free segments
					   or list of inactive segments. */
    int			segNum;		/* The number of this segment. */
    int 		refCount;	/* Number of processes using this 
					   segment */
    Fs_Stream		*filePtr;	/* Pointer to the file that pages are
					   being demanded loaded from. */
				        /* Name of object file for code 
					 * segments. */
    char		objFileName[VM_OBJ_FILE_NAME_LENGTH];
    Fs_Stream		*swapFilePtr;	/* Structure for an opened swap file.*/
    char		*swapFileName;  /* The filename associated with the
					 * swap file. */
    int			offset;		/* Explained above. */
    int			fileAddr;	/* The address in the object file where
					   data or code for this segment 
					   begins. */
    int           	type;		/* CODE, STACK, HEAP, or SYSTEM */
    int			numPages;	/* Explained above. */
    int			resPages;	/* Number of pages in physical memory
					 * for this segment. */
    int			ptSize;		/* Number of pages in the page table */
    Vm_PTE		*ptPtr;		/* Pointer to the page table for this 
					   segment */
    struct VmMachData	*machData;	/* Pointer to machine dependent data */
    int			flags;		/* Flags to give information about the
					   segment table entry. */
    List_Links		procListHdr;	/* Header node for list of processes
					   sharing this segment. */
    List_Links		*procList;	/* Pointer to list of processes 
					   sharing this segment. */
    int			notExpandCount;	/* The number of times that this 
					   segment has been prevented from
					   expanding. */
    ClientData		fileHandle;	/* Handle for object file. */
    Vm_ExecInfo		execInfo;	/* Information to allow reuse of 
					 * sticky segments. */
} Vm_Segment;

/*
 * Virtual memory bit map structure.
 */

typedef struct {
    Address	baseAddr;	/* Base virtual address to start 
				   allocating at. */
    Address	endAddr;	/* Last possible virtual address plus one. */
} Vm_DevBuffer;

/*
 * The initialization procedure.
 */

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
extern	void	Vm_InitPageTable();

/*
 * Procedure to allocate bytes of memory after boot time.
 */
extern	Address	Vm_RawAlloc();

/*
 * Procedures for process migration.
 */
extern	ReturnStatus	Vm_MigrateSegment();
extern	ReturnStatus	Vm_FreezeSegments();
extern	void		Vm_MigSegmentDelete();

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
 * Procedures for recovery.
 */
extern	void		Vm_OpenSwapDirectory();
extern	void		Vm_Recovery();

#endif _VM
