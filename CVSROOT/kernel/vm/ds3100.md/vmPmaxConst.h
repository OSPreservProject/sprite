/*
 * vmSunConst.h --
 *
 *     	Virtual memory constants for the PMAX.  See the "R2000 Risc 
 *	Architecture" manual or the "PMAX Desktop Workstation Functional
 *	Specification" for the definitions of these constants.
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

#ifndef _VMPMAXCONST
#define _VMPMAXCONST

/*
 * The low part of the TLB entry.
 */
#define VMMACH_TLB_PF_NUM		0xfffff000
#define VMMACH_TLB_NON_CACHEABLE_BIT	0x00000800
#define VMMACH_TLB_MOD_BIT		0x00000400
#define VMMACH_TLB_VALID_BIT		0x00000200
#define VMMACH_TLB_GLOBAL_BIT		0x00000100

#define VMMACH_TLB_ENTRY_WRITEABLE	0x00000080

#define VMMACH_TLB_PHYS_PAGE_SHIFT	12

/*
 * The high part of the TLB entry.
 */
#define VMMACH_TLB_VIRT_PAGE_NUM	0xfffff000
#define VMMACH_TLB_PID			0x00000fc0
#define VMMACH_TLB_PID_SHIFT		6

#define VMMACH_TLB_VIRT_PAGE_SHIFT	12

/*
 * The shift to put the index in the right spot.
 */
#define VMMACH_TLB_INDEX_SHIFT		8

/*
 * The cache block size.
 */
#define VMMACH_CACHE_BLOCK_SIZE		32

/*
 * Hardware dependent constants for pages and segments:
 *
 * VMMACH_CLUSTER_SIZE		The number of hardware pages per virtual page.
 * VMMACH_CLUSTER_SHIFT		The log base 2 of VMMACH_CLUSTER_SIZE.
 * VMMACH_PAGE_SIZE		The size of each virtual page.
 * VMMACH_PAGE_SIZE_INT		The size of each hardware page.
 * VMMACH_PAGE_SHIFT		The log base 2 of VMMACH_PAGE_SIZE.
 * VMMACH_PAGE_SHIFT_INT	The log base 2 of VMMACH_MACH_PAGE_SIZE.
 * VMMACH_OFFSET_MASK		Mask to get to the offset of a virtual address.
 * VMMACH_OFFSET_MASK_INT	Mask to get to the offset of a virtual address.
 * VMMACH_PAGE_MASK		Mask to get to the Hardware page number within a
 *				hardware segment.
 * VMMACH_SEG_SIZE		The size of each hardware segment.
 * VMMACH_SEG_SHIFT		The log base 2 of VMMACH_SEG_SIZE.
 * VMMACH_NUM_PAGES_PER_SEG	The number of virtual pages per segment.
 */

#define	VMMACH_CLUSTER_SIZE	1
#define	VMMACH_CLUSTER_SHIFT	0
#define	VMMACH_PAGE_SIZE	(VMMACH_CLUSTER_SIZE * VMMACH_PAGE_SIZE_INT)
#define	VMMACH_PAGE_SIZE_INT	4096
#define VMMACH_PAGE_SHIFT	(VMMACH_CLUSTER_SHIFT + VMMACH_PAGE_SHIFT_INT)
#define VMMACH_PAGE_SHIFT_INT	12	
#define VMMACH_OFFSET_MASK	0xfff
#define VMMACH_OFFSET_MASK_INT	0xfff

/*
 * The size that page tables are to be allocated in.  This grows software
 * segments in 256K chunks.
 */
#define	VMMACH_PAGE_TABLE_INCREMENT	64

/*
 * How the VAS is partitioned.
 */
#define VMMACH_KUSEG_START		0
#define VMMACH_PHYS_CACHED_START	0x80000000
#define VMMACH_PHYS_CACHED_START_PAGE	0x80000
#define VMMACH_PHYS_UNCACHED_START	0xa0000000
#define VMMACH_VIRT_CACHED_START	0xc0000000
#define VMMACH_VIRT_CACHED_START_PAGE	0xc0000

/*
 * Where we map kernel pages into the user's address space.
 */
#define VMMACH_USER_MAPPING_PAGES	512
#define VMMACH_USER_MAPPING_BASE_ADDR	(VMMACH_PHYS_CACHED_START - (VMMACH_USER_MAPPING_PAGES * VMMACH_PAGE_SIZE))
#define VMMACH_USER_MAPPING_BASE_PAGE	(VMMACH_PHYS_CACHED_START_PAGE - VMMACH_USER_MAPPING_PAGES)

/*
 * Where the mapped page is at for cross address space copies.
 */
#define VMMACH_MAPPED_PAGE_ADDR		0xfffff000
#define VMMACH_MAPPED_PAGE_NUM		0xfffff

/*
 * The number of TLB entries and the first one that write random hits.
 */
#define VMMACH_NUM_TLB_ENTRIES		64
#define VMMACH_FIRST_RAND_ENTRY 	8

/*
 * The number of process id entries.
 */
#define	VMMACH_NUM_PIDS			64

/*
 * Invalid pid.
 */
#define	VMMACH_INV_PID		0

/*
 * The kernel's pid.
 */
#define VMMACH_KERN_PID		1

/*
 * TLB registers
 */
#define	VMMACH_TLB_INDEX	$0
#define VMMACH_TLB_RANDOM	$1
#define VMMACH_TLB_LOW		$2
#define VMMACH_TLB_CONTEXT	$4
#define VMMACH_BAD_VADDR	$8
#define VMMACH_TLB_HI		$10

/*
 * The maximum number of kernel stacks.
 */
#define	VMMACH_MAX_KERN_STACKS	64

/*
 * TLB probe return codes.
 */
#define VMMACH_TLB_NOT_FOUND		0
#define VMMACH_TLB_FOUND		1
#define VMMACH_TLB_FOUND_WITH_PATCH	2
#define VMMACH_TLB_PROBE_ERROR		3

/*
 * TLB hash table values.
 */
#define VMMACH_LOW_REG_OFFSET		8
#define VMMACH_HASH_KEY_OFFSET		12
#define VMMACH_NUM_TLB_HASH_ENTRIES	8192
#define VMMACH_PAGE_HASH_SHIFT		(VMMACH_PAGE_SHIFT - VMMACH_BUCKET_SIZE_SHIFT)
#define VMMACH_PID_HASH_SHIFT		5
#define VMMACH_BUCKET_SIZE_SHIFT	4
#define VMMACH_HASH_MASK		0x1fff0
#define VMMACH_HASH_MASK_2		0x1fff

/*
 * TLB Hash table performance counters and place to save a temporary.  These
 * values live in low memory just after the exception handlers.  They are
 * in low memory so that they can be accessed quickly.
 */
#define VMMACH_STAT_BASE_OFFSET		0x100
#define VMMACH_SAVED_AT_OFFSET		VMMACH_STAT_BASE_OFFSET + 0
#define VMMACH_UTLB_COUNT_OFFSET	VMMACH_STAT_BASE_OFFSET + 4
#define VMMACH_UTLB_HIT_OFFSET		VMMACH_STAT_BASE_OFFSET + 8
#define VMMACH_MOD_COUNT_OFFSET		VMMACH_STAT_BASE_OFFSET + 12
#define VMMACH_MOD_HIT_OFFSET		VMMACH_STAT_BASE_OFFSET + 16

#endif _VMPMAXCONST
