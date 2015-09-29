/*
 * vmTypes.h --
 *
 *	Type declarations for the VM module.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/vm/RCS/vmTypes.h,v 1.19 92/07/09 15:46:09 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _VMTYPES
#define _VMTYPES

#include <sprite.h>
#include <mach.h>
#include <list.h>
#include <sys/types.h>

#ifdef SPRITED
#include <syncTypes.h>
#include <sys.h>
#else
#include <sprited/syncTypes.h>
#include <sprited/sys.h>
#endif

/* 
 * To avoid an include loop, we don't include fs.h here.  This means that 
 * we have to say "struct Fs_Stream *" down below.
 */

/* 
 * Flag to have the VM code keep track of the size of a segment's backing 
 * file.  See the notes for Vm_Segment.  Warning: not all of the code that
 * is used when this flag is disabled has been tested.
 */
#define VM_KNOWS_SWAP_FILE_SIZE	1

/* 
 * The heap segment is backed by two files.  The initialization file 
 * (the object file of the program being run) is used once for each 
 * page.  After that the page is backed by the swap file.  This map 
 * tells where to get each page.  It is an array of booleans, one per 
 * page, up to the number of pages available from the initialization 
 * file.
 */
    
typedef struct {
    int		arraySize;	/* number of elements in useInitFile array 
				 * (indexed 0..arraySize-1) */
    char	*useInitPtr;	/* array of "boolean"; TRUE means use 
				 * the initialization file */
} VmFileMap;


/* XXX - these should go back into the user vmTypes.h */
/* 
 * The segment type determines how the backing files are used.
 * 
 * VM_SYSTEM - unused.
 * VM_CODE - This segment holds program text; the swap file is the 
 * 	     object file being run, which is made read-only.  The 
 * 	     segment size only large enough to map the code portion of 
 * 	     the file, starting from the beginning of the file.
 * 	     There is no initialization file.  The segment may be
 * 	     shared among multiple processes.  The swap file pointer can be 
 * 	     nil, if the segment is being cached in the hopes it will be 
 * 	     reused before it gets paged out.
 * VM_HEAP - This segment holds the program heap (data and bss 
 * 	     sections).  The swap file is read-write.  The segment may
 * 	     be larger than the swap file.  The initialization file is
 * 	     typically the data section of a binary file.  The segment 
 * 	     should not be shared.  For performance, the swap file is not 
 * 	     actually created or opened until necessary.
 * VM_STACK - This segment holds the program stack.  The swap file is 
 * 	     read-write, and the segment may be larger than the swap
 * 	     file.  The mapping from segment offset to swap file
 * 	     offset is machine-dependent, though typically the segment
 * 	     grows backwards.  There is no initialization file.  The 
 * 	     segment should not be shared.  For performance, the swap file
 * 	     is not actually created or opened until necessary.
 * VM_SHARED - This segment is used for user mapped files.  It is 
 * 	     treated like VM_HEAP, but there is no initialization 
 * 	     file, and the segment may be shared among multiple 
 * 	     processes.
 */

typedef int Vm_SegmentType;	/* defines the type of segment */

#define VM_SYSTEM	0
#define VM_CODE		1
#define VM_HEAP		2
#define VM_STACK	3
#define VM_SHARED	4
#define VM_NUM_SEGMENT_TYPES	(VM_SHARED + 1)


/* 
 * For heap segments, the Sprite external pager gets bits from an
 * object file.  It needs to know where in the object file to start,
 * and how much of the object file is valid.  It also needs a map
 * showing which pages in the swap file are valid.
 */

typedef struct {
    off_t	initStart;	/* where in the initialization file to begin */
    int		initLength;	/* how many bytes are valid */
    struct Fs_Stream *initFilePtr; /* the initialization file */
    char	*initFileName;	/* and its name */
    VmFileMap	*mapPtr;	/* pointer to per-page map, telling 
				 * which file to use */
} VmHeapSegInfo;

/* 
 * The stack segment remembers what its base address in the user 
 * process is.  This part of the segment will probably never be 
 * mapped, but it's useful for converting from an address in the 
 * process to an offset in the segment.  (A similar base address isn't 
 * needed for heap segments because that value is already in the 
 * VmMappedSegment structure for the heap.)
 */

typedef struct {
    Address	baseAddr;	/* corresponds to offset 0 of the segment */
} VmStackSegInfo;

/* 
 * Code segments keep a copy of the a.out header information, to simplify 
 * reuse of existing an existing segment.  We use "struct Proc_ObjInfo" to 
 * avoid a circular dependency between vmTypes.h and procTypes.h.
 */

typedef struct {
    struct Proc_ObjInfo *execInfoPtr;
} VmCodeSegInfo;

/* 
 * This union contains type-specific information for a segment. 
 */

typedef union {
    VmHeapSegInfo	heapInfo;
    VmStackSegInfo	stackInfo;
    VmCodeSegInfo	codeInfo;
} VmPerTypeInfo;


/* 
 * State of the segment.
 * VM_SEGMENT_OK - the segment is okay; all operations are allowed.
 * VM_SEGMENT_DYING - the segment is being shutdown.  Only writebacks 
 * 		      (to flush dirty pages) are allowed.
 * VM_SEGMENT_DEAD - the segment is history; no further I/O or mapping 
 * 		     operations are allowed.  It will go away when its
 * 		     reference count goes to zero and it's no longer 
 * 		     registered with the kernel.
 */

typedef int Vm_SegmentState;

#define VM_SEGMENT_OK		1
#define VM_SEGMENT_DYING	2
#define VM_SEGMENT_DEAD		3


/* 
 * Flags telling what the segment is doing.
 * VM_CLEANING_SEGMENT - we have asked the kernel to write back any 
 *      	   	 dirty pages and are waiting for the operation 
 *      	   	 to complete.
 * VM_SEGMENT_ACTIVE   - A process is currently running, handling messages 
 *			 from a segment's queue.
 * VM_SEGMENT_NOT_IN_SET - The segment has been removed from the system 
 *                       request port set to keep a bound on the number of 
 *                       unprocessed requests.
 * VM_COPY_SOURCE      - The segment is being copied to another segment 
 * 			 (instrumentation).
 * VM_COPY_TARGET      - The segment is being created by copying another 
 * 			 segment (used for instrumentation and to avoid 
 * 			 unnecessary FS requests).
 * 
 * XXX if we want to support a mapped-file stdio, then we have to be 
 * able to, e.g., map a text file read-only.  In that case we probably 
 * need a flag telling whether the segment is read-only or not, so 
 * that a read-only mapped file can later be used a text file.  Also, 
 * for code segments the segment length would have to be the length of 
 * the entire file, not just long enough to map the code.
 */

typedef int Vm_SegmentFlags;

#define VM_CLEANING_SEGMENT		0x001
#define VM_SEGMENT_ACTIVE		0x004
#define VM_SEGMENT_NOT_IN_SET		0x008
#define VM_COPY_SOURCE			0x010
#define VM_COPY_TARGET			0x020


/* 
 * The segment is the basic VM abstraction.  It corresponds to a 
 * native Sprite segment and under Mach is known as a memory object.
 * It backs some region of memory with a file, called a swap file.
 * There is a one-to-one correspondence between swap files and
 * segments: that is, there is at most one segment for a swap file in
 * the system.  Normally the swapFilePtr is used to refer to the swap file. 
 * The swapFileHandle is used if the swap file has been closed (e.g., for 
 * sticky segments).  It is a ClientData for historical reasons.
 * 
 * There is also an "initialization file", which is used to supply pages if
 * the swap file doesn't have them.  See the notes for Vm_SegmentType for
 * the relation ship between the files and the segment type.
 * 
 * The control port is used for making lock requests.  It is also used 
 * as a unique ID to handle races between memory object creation and 
 * destruction.  If it is non-null, it means that the kernel is aware 
 * of the segment and the segment should not be destroyed.
 * 
 * For some segment types, the size of the segment may be larger than
 * the size of the backing file.  This can happen if someone has
 * mapped the segment read-write and specified a longer length.  The
 * file will be lengthened, using zero-fill pages, as necessary.  The
 * segment size recorded here is in bytes and is always equal to some
 * integer number of pages.  
 * 
 * The swap file size is obtained from the file service when the segment is 
 * created and is thereafter maintained by the VM code.  There are two 
 * reasons for doing this: (1) performance (avoid an RPC to the server for 
 * every page read), and (2) correctness (I have seen a bug where 
 * Fs_GetAttrStream would return a size of 0 for a 32MB file).  The 
 * drawback of this scheme is that it destroys any chance of maintaining 
 * consistency for mapped files, but then Sprite never supported that 
 * anyway.
 * 
 * Note: once the files for a segment are set, they shouldn't be 
 * changed.  This lets us use the files while the segment is 
 * unlocked.
 *
 * Requests for a segment are put on a per-segment queue, so as not to have 
 * a permanent thread for each segment, and to avoid tying up other
 * segments if one segment gets stuck.  When a request is dequeued, the
 * request buffer comes first, followed by the reply buffer.
 */

typedef struct Vm_Segment {
    List_Links	links;		/* list of in-use segments */
    int		magic;		/* magic number to verify is really a 
				 * segment */
    Sync_Lock	lock;		/* lock for exclusive access */
    Sync_Condition condition;	/* condition variable for completion of 
				 * long operations */
    int		refCount;	/* reference count */
    int		residentPages;	/* pages given to the kernel and not
				 * returned */
    Vm_SegmentFlags flags;
    mach_port_t	requestPort;	/* this is the port that requests come 
				 * in on */
    mach_port_t	controlPort;	/* see notes above */
    mach_port_t namePort;	/* "name" for vm_region calls */
    Vm_SegmentType type;	/* code, heap, mapped file, etc. */
    Vm_SegmentState state;	/* ok, dead, etc. */
    struct Fs_Stream *swapFilePtr; /* I/O stream for the backing file (may be 
				    * nil) */
    char	*swapFileName;	/* name of the backing file (allocated 
				 * storage) */
    ClientData	swapFileHandle; /* the backing file; normally use 
				 * swapFilePtr */ 
			/* the next 3 fields uniquely identify the swap file */
    int		swapFileServer;	/* attributes.serverID */
    int		swapFileDomain;	/* attributes.domain */
    int		swapFileNumber;	/* attributes.fileNumber */
#if VM_KNOWS_SWAP_FILE_SIZE
    off_t	swapFileSize;	/* cached size of swap file */
#endif
    vm_size_t	size;		/* segment size; see notes above */
    List_Links	requestHdr;	/* header for the request queue */
    List_Links	*requestList;
    int queueSize;		/* num of requests (buffer pairs) in queue */
    VmPerTypeInfo typeInfo;	/* type-dependent information */
} Vm_Segment;

#define VM_SEGMENT_MAGIC_NUMBER	0xa7210891


/* 
 * The Vm_TaskInfo defines VM-related information for each task.  
 * Currently we just keep a list of each segment that is mapped into
 * the task, along with the range of addresses that the segment might
 * back.  The segment list has two purposes.  First, when we clear out
 * a process's address space, we're more likely to manage the segments'
 * reference counts correctly.  Second, it lets us determine when to 
 * grow the process's stack.  Because we expect to need the code, 
 * heap, and stack segments fairly frequently, we keep a distinguised 
 * set of pointers for those segments.  XXX The stack segment should 
 * be per-process, not per-task.
 * 
 * Note that we don't keep track of the exact mappings for a segment.  
 * For example, if a process maps a segment then later unmaps a 
 * portion of it, we'll still think it has the entire segment mapped.  
 * This has three potential problems:
 * 
 * (1) when clearing a process's address space (at exec), we might try 
 * to deallocate memory that's not allocated.  Since we're already 
 * planning to just deallocate the entire range in one shot, this 
 * shouldn't be a problem.  
 * 
 * (2) we might hold onto resources longer than necessary.  For
 * example, if a user program deallocates a mapped file without
 * notifying us, the reference for the segment won't go away until the
 * process dies or does an exec.  Hopefully this won't be much of a
 * problem in practice.  If it is, one solution might be to mark the 
 * segment in memory_object_terminate, and have a background process 
 * run through the process table looking for such marked segments.
 * 
 * (3) we might fail to extend the stack correctly.  This could happen 
 * if a segment is mapped at a high address, near the stack, and then 
 * later freed without notifying us.  If the stack then tries to
 * extend into memory formerly occupied by this mapped segment, we 
 * will refuse to extend the stack and generate an incorrect 
 * exception.  This should not be a problem in practice.  If it is, 
 * one solution would be to query the kernel for the current 
 * allocations in the faulting task and use that information, rather 
 * than the list of mapped segments, to decide whether to extend the 
 * stack. 
 */

typedef struct {
    List_Links	links;		/* links to rest of list */
    Vm_Segment	*segPtr;	/* reference to the segment */
    Address	mappedAddr;	/* first address mapped by the segment */
    vm_size_t	length;		/* how many bytes are mapped by the segment */
} VmMappedSegment;

typedef struct {
    List_Links	mappedListHdr;	/* header for VmMappedSegment list */
    int execHeapPages;		/* pages in heap at previous exec */
    int execStackPages;		/* pages in stack at previous exec */
    VmMappedSegment *codeInfoPtr; /* pointer to info for code segment */
    VmMappedSegment *heapInfoPtr; /* pointer to info for heap segment */
    VmMappedSegment *stackInfoPtr; /* pointer to info for stack segment */
} Vm_TaskInfo;


/*
 * The type of accessibility desired when making a piece of data user
 * accessible.  VM_READONLY_ACCESS means that the data will only be read and
 * will not be written.  VM_OVERWRITE_ACCESS means that the entire block of
 * data will be overwritten.  VM_READWRITE_ACCESS means that the data 
 * will both be read and written.
 */

typedef int Vm_Accessibility;

#define	VM_READONLY_ACCESS		1
#define	VM_OVERWRITE_ACCESS		2
#define	VM_READWRITE_ACCESS		3

#endif /* _VMTYPES */
