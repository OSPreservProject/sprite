/*
 * vmSpurConst.h --
 *
 *     	Virtual memory constants for SPUR.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMSPURCONST
#define _VMSPURCONST

/*
 * There are 255 usuable segments (0 through 254) and one invalid segment
 * so we can map this segment into the three unused kernel segments.
 */
#define	VMMACH_NUM_SEGMENTS		256
#define	VMMACH_INVALID_SEGMENT		255

/*
 * SPUR page table entry masks.
 *
 *    VMMACH_RESIDENT_BIT	The page is physically resident.
 *    VMMACH_PROTECTION_FIELD	Access to the protection bits.
 *    VMMACH_KRO_UNA_PROT	Kernel read only, user no access.
 *    VMMACH_KRW_UNA_PROT	Kernel read/write, user no access.
 *    VMMACH_KRW_URO_PROT	Kernel read/write, user read only
 *    VMMACH_KRW_URW_PROT	Kernel read/write, user read/write.
 *    VMMACH_TYPE_FIELD		The memory type field.
 *    VMMACH_REFERENCED_BIT	Referenced bit.
 *    VMMACH_MODIFIED_BIT	Modified bit.
 *    VMMACH_PAGE_FRAME_FIELD	The hardware page frame.
 */
#define	VMMACH_RESIDENT_BIT		0x00000001
#define	VMMACH_MODIFIED_BIT		0x00000002
#define	VMMACH_REFERENCED_BIT		0x00000004
#define	VMMACH_CACHEABLE_BIT		0x00000008
#define	VMMACH_COHERENCY_BIT		0x00000010
#define	VMMACH_PROTECTION_FIELD		0x00000060
#ifdef MAKE_READABLE
/*
 * Make everything user readable so instruction buffer works (maybe).
 */
#define	VMMACH_KRO_UNA_PROT		VMMACH_KRW_URO_PROT
#define	VMMACH_KRW_UNA_PROT		VMMACH_KRW_URO_PROT
#else
#define	VMMACH_KRO_UNA_PROT		0x00000000
#define	VMMACH_KRW_UNA_PROT		0x00000020
#endif
#define	VMMACH_KRW_URO_PROT		0x00000040
#define	VMMACH_KRW_URW_PROT		0x00000060
#define VMMACH_PAGE_FRAME_FIELD		0xfffff000	
#define VMMACH_PAGE_FRAME_SHIFT		12
#define VMMACH_PAGE_FRAME_INC_VALUE	0x00001000

#define IncPageFrame(pte) ((pte) + VMMACH_PAGE_FRAME_INC_VALUE)
#define SetPageFrame(pfNum) ((pfNum) << VMMACH_PAGE_FRAME_SHIFT)
#define GetPageFrame(pte) (((pte) & VMMACH_PAGE_FRAME_FIELD) >> VMMACH_PAGE_FRAME_SHIFT)

/*
 * How the kernel's virtual address space is partitioned.  The lower quarter
 * is for kernel code and data.  The next quarter is for the page tables.
 * Finally the rest is for devices.
 */
#define	VMMACH_KERN_CODE_DATA_QUAD	0
#define	VMMACH_KERN_PT_QUAD		1
#define	VMMACH_KERN_DEVICE_SPACE	(VMMACH_SEG_SIZE / 2)
/*
 * The first page of device space is for the UART driver.
 */
#define	VMMACH_UART_ADDR		VMMACH_KERN_DEVICE_SPACE

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
 * VMMACH_SEG_SIZE		The number of bytes in a segment.
 * VMMACH_SEG_PT_SIZE		The number of bytes in a page table for a 
 *				segment.
 * VMMACH_SEG_PT2_SIZE		The number of bytes in a page table that maps
 *				a page table.
 * VMMACH_SEG_PT2_SHIFT		Amount of shift a virtual page to get to the
 *				index into the 2nd level page table.
 * VMMACH_NUM_PT3_PAGES		The number of 3rd page table pages.
 * VMMACH_PTES_IN_PAGE		The number of page table entries that can
 *				be stored in one page.
 * VMMACH_PTES_PER_SEG		Number of page table entries needed to describe
 *				a segment.
 * VMMACH_PAGES_PER_SEG		The number of pages that are in each hardware
 *				segment.
 * VMMACH_SEG_REG_SHIFT		Number of bits to shift a virtual address to
 *				the right to get the segment register value.
 */
#define	VMMACH_CLUSTER_SIZE	1
#define	VMMACH_CLUSTER_SHIFT	0
#define	VMMACH_PAGE_SIZE	(VMMACH_CLUSTER_SIZE * VMMACH_PAGE_SIZE_INT)
#define	VMMACH_PAGE_SIZE_INT	4096
#define VMMACH_PAGE_SHIFT	(VMMACH_CLUSTER_SHIFT + VMMACH_PAGE_SHIFT_INT)
#define VMMACH_PAGE_SHIFT_INT	12	
#define VMMACH_OFFSET_MASK	0xfff
#define VMMACH_OFFSET_MASK_INT	0xfff
#define VMMACH_SEG_SIZE		(VMMACH_PAGE_SIZE_INT * VMMACH_PAGES_PER_SEG)
#define VMMACH_SEG_PT_SIZE	(4 * VMMACH_PAGES_PER_SEG)
#define	VMMACH_NUM_PT_PAGES	(VMMACH_SEG_PT_SIZE / VMMACH_PAGE_SIZE)
#define VMMACH_SEG_PT2_SIZE	(4 * VMMACH_NUM_PT_PAGES)
#define	VMMACH_SEG_PT2_SHIFT	10
#define	VMMACH_NUM_PT3_PAGES	64
#define	VMMACH_PTES_IN_PAGE	1024
#define VMMACH_PAGES_PER_SEG	(1024 * 256)
#define VMMACH_SEG_REG_SHIFT	30
#define	VMMACH_SEG_REG_MASK	0xC0000000

/*
 * The base of kernel's page table and the portion of the kernel's page table
 * that maps the kernel page table.
 */
#define	VMMACH_KERN_PT_BASE	(VMMACH_SEG_SIZE / 4 * VMMACH_KERN_PT_QUAD)
#define	VMMACH_KERN_PT2_BASE	(VMMACH_KERN_PT_BASE + VMMACH_SEG_PT_SIZE / 4 * VMMACH_KERN_PT_QUAD)
/*
 * The size of a cache block and the size of the cache.
 */
#define	VMMACH_CACHE_BLOCK_SIZE		32
#define	VMMACH_CACHE_SIZE		(128 * 1024)

/*
 * The size that page tables are to be allocated in.  This grows software
 * segments in 256K chunks.
 */
#define	VMMACH_PAGE_TABLE_INCREMENT	64

/*
 * Where the three segments start.
 */
#define	VMMACH_CODE_SEG_START		0x40000000
#define	VMMACH_HEAP_SEG_START		0x80000000
#define	VMMACH_STACK_SEG_START		0xc0000000

#endif _VMSPURCONST
