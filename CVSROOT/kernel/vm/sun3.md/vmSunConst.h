/*
 * vmSunConst.h --
 *
 *     	Virtual memory constants for the Sun.  See the "Sun 2.0 Architecture 
 *	Manual" for a definition of these constants.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMSUN3CONST
#define _VMSUN3CONST

/*
 * Sun3 page table entry masks.
 *
 *    VMMACH_RESIDENT_BIT	The page is physical resident.
 *    VMMACH_PROTECTION_FIELD	Access to the protection bits.
 *    VMMACH_KR_PROT		Kernel read only, user no access.
 *    VMMACH_KRW_PROT		Kernel read/write, user no access.
 *    VMMACH_UR_PROT		Kernel read only, user read only
 *    VMMACH_URW_PROT		Kernel read/write, user read/write.
 *    VMMACH_DONT_CACHE_BIT	Don't cache bit.
 *    VMMACH_TYPE_FIELD		The memory type field.
 *    VMMACH_REFERENCED_BIT	Referenced bit.
 *    VMMACH_MODIFIED_BIT	Modified bit.
 *    VMMACH_PAGE_FRAME_FIELD	The hardware page frame.
 */
#define	VMMACH_RESIDENT_BIT	0x80000000
#define	VMMACH_PROTECTION_FIELD	0x60000000
#define	VMMACH_UR_PROT		0x00000000
#define	VMMACH_KR_PROT		0x20000000
#define VMMACH_URW_PROT		0x40000000
#define	VMMACH_KRW_PROT		0x60000000
#define	VMMACH_DONT_CACHE_BIT	0x10000000
#define	VMMACH_TYPE_FIELD	0x0c000000
#define VMMACH_REFERENCED_BIT	0x02000000
#define	VMMACH_MODIFIED_BIT	0x01000000
#define VMMACH_PAGE_FRAME_FIELD	0x0007ffff	

/*
 * Shift to allow the type field to get set.
 */
#define	VmMachGetPageType(pte) (((pte) >> 26) & 3)
#define	VmMachSetPageType(pte, type) (pte |= (type << 26))

/*
 * Sun memory management unit constants:
 *
 * VMMACH_KERN_CONTEXT		The Kernel's context.
 * VMMACH_NUM_CONTEXTS		The number of contexts.
 * VMMACH_NUM_SEGS_PER_CONTEXT	The number of hardware segments per context.
 * VMMACH_NUM_PAGES_PER_SEG_INT	The number of pages per hardware segment.
 * VMMACH_NUM_PAGE_MAP_ENTRIES	The number of hardware page map entries.
 * VMMACH_NUM_PMEGS		The number of page clusters that there are 
 *				in the system.
 * VMMACH_DEV_START_ADDR        The first virtual address that is used for
 *                              mapping devices.
 * VMMACH_DEV_END_ADDR		The last virtual address that could be used for 
 *				mapping devices.
 * VMMACH_DMA_START_ADDR	The first virtual address that is used for 
 *				DMA buffers by devices.
 * VMMACH_DMA_SIZE		The amount of space devoted to DMA buffers
 * VMMACH_NET_MAP_START		The beginning of space for mapping the Intel
 *				and AMD drivers.
 * VMMACH_NET_MEM_START		The beginning of space for memory for the Intel
 *				and AMD drivers.
 * VMMACH_FIRST_SPECIAL_SEG	The first hardware segment that is used for 
 *				mapping devices.
 * VMMACH_INV_PMEG   		The page cluster number that is used to 
 *				represent an invalid Pmeg.
 * VMMACH_INV_SEG   		The hardware segment number that is used to
 *				represent an invalid hardware segment.
 * VMMACH_INV_CONTEXT   	The context number that is used to represent
 *				an invalid context.
 */

#define VMMACH_KERN_CONTEXT		0
#define VMMACH_NUM_CONTEXTS		8
#define VMMACH_NUM_SEGS_PER_CONTEXT	2048
#define VMMACH_NUM_PAGES_PER_SEG_INT	16
#define VMMACH_NUM_PAGE_MAP_ENTRIES	4096
#define	VMMACH_NUM_PMEGS		(VMMACH_NUM_PAGE_MAP_ENTRIES / VMMACH_NUM_PAGES_PER_SEG_INT)

#define VMMACH_DEV_START_ADDR       	0xFE00000
#define	VMMACH_DEV_END_ADDR		0xFEFFFFF
#define	VMMACH_DMA_START_ADDR		0xFF00000
#define	VMMACH_DMA_SIZE			0xC0000
#define VMMACH_NET_MAP_START		0xFFC0000
#define VMMACH_NET_MEM_START		0xFFE0000
#define	VMMACH_FIRST_SPECIAL_SEG	(VMMACH_DEV_START_ADDR >> VMMACH_SEG_SHIFT)
#define	VMMACH_INV_PMEG			(VMMACH_NUM_PMEGS - 1)
#define	VMMACH_INV_SEG			VMMACH_NUM_SEGS_PER_CONTEXT
#define	VMMACH_INV_CONTEXT		VMMACH_NUM_CONTEXTS

/*
 * Function code constants for access to the different address spaces as
 * defined by the Sun and 68010 hardware.  These constants determine which
 * address space is accessed when using a "movs" instruction.
 *
 * VMMACH_USER_DATA_SPACE		User data space.
 * VMMACH_USER_PROGRAM_SPACE	User program space
 * VMMACH_MMU_SPACE			Sun-2 memory map space.
 * VMMACH_KERN_DATA_SPACE		Kernel data space.
 * VMMACH_KERN_PROGRAM_SPACE	Kernel program space.
 * VMMACH_CPU_SPACE			Cpu space.
 */

#define	VMMACH_USER_DATA_SPACE		1
#define	VMMACH_USER_PROGRAM_SPACE	2
#define	VMMACH_MMU_SPACE		3
#define	VMMACH_KERN_DATA_SPACE		5
#define	VMMACH_KERN_PROGRAM_SPACE	6
#define	VMMACH_CPU_SPACE		7

/*
 * Masks for access to different parts of mmu space.
 *
 * VMMACH_PAGE_MAP_OFF	 	Offset to hardware page map entries
 * VMMACH_SEG_MAP_OFF	 	Offset to hardware segment map entries
 * VMMACH_CONTEXT_OFF	 	Offset to context register
 * VMMACH_DIAGNOSTIC_REG	The address of the diagnostic register.
 * VMMACH_BUS_ERROR_REG		The address of the bus error register.
 * VMMACH_SYSTEM_ENABLE_REG	The address of the system enable register.
 * VMMACH_ETHER_ADDR		Address of ethernet address in the id prom.
 * VMMACH_MACH_TYPE_ADDR	Address of machine type in the id prom.
 * VMMACH_IDPROM_INC		Amount to increment an address when stepping
 *				through the id prom.
 */

#define	VMMACH_PAGE_MAP_OFF		0x10000000
#define	VMMACH_SEG_MAP_OFF		0x20000000
#define	VMMACH_CONTEXT_OFF		0x30000000
#define VMMACH_SYSTEM_ENABLE_REG	0x40000000
#define VMMACH_BUS_ERROR_REG		0x60000000
#define VMMACH_DIAGNOSTIC_REG		0x70000000
#define VMMACH_ETHER_ADDR		0x02
#define VMMACH_MACH_TYPE_ADDR		0x01
#define VMMACH_IDPROM_INC		0x01

/*
 * The highest virtual address useable by the kernel for both machine type
 * 1 and machine type 2.
 */

#define	VMMACH_MACH_TYPE1_MEM_END	0xF40000
#define	VMMACH_MACH_TYPE2_MEM_END	0x1000000
#define	VMMACH_MACH_TYPE3_MEM_END	0x10000000

/*
 * Masks to extract good bits from the virtual addresses when accessing
 * the page and segment maps.
 */

#define	VMMACH_PAGE_MAP_MASK	0xFFFFe000
#define	VMMACH_SEG_MAP_MASK		0xFFFe0000

/*
 * Mask to get only the low order three bits of a context register.
 */

#define	VMMACH_CONTEXT_MASK		0x7

/*
 * Hardware dependent constants for pages and segments:
 *
 * VMMACH_CLUSTER_SIZE      The number of hardware pages per virtual page.
 * VMMACH_CLUSTER_SHIFT     The log base 2 of VMMACH_CLUSTER_SIZE.
 * VMMACH_PAGE_SIZE		The size of each virtual page.
 * VMMACH_PAGE_SIZE_INT		The size of each hardware page.
 * VMMACH_PAGE_SHIFT		The log base 2 of VMMACH_PAGE_SIZE.
 * VMMACH_PAGE_SHIFT_INT	The log base 2 of VMMACH_PAGE_SIZE_INT
 * VMMACH_OFFSET_MASK		Mask to get to the offset of a virtual address.
 * VMMACH_OFFSET_MASK_INT	Mask to get to the offset of a virtual address.
 * VMMACH_PAGE_MASK		Mask to get to the Hardware page number within a
 *				hardware segment.
 * VMMACH_SEG_SIZE		The size of each hardware segment.
 * VMMACH_SEG_SHIFT		The log base 2 of VMMACH_SEG_SIZE.
 * VMMACH_NUM_PAGES_PER_SEG	The number of virtual pages per segment.
 */

#define VMMACH_CLUSTER_SIZE     1
#define VMMACH_CLUSTER_SHIFT    0
#define VMMACH_PAGE_SIZE        (VMMACH_CLUSTER_SIZE * VMMACH_PAGE_SIZE_INT)
#define	VMMACH_PAGE_SIZE_INT	8192
#define VMMACH_PAGE_SHIFT	(VMMACH_CLUSTER_SHIFT + VMMACH_PAGE_SHIFT_INT)
#define VMMACH_PAGE_SHIFT_INT	13
#define VMMACH_OFFSET_MASK	0x1fff
#define VMMACH_OFFSET_MASK_INT	0x1fff
#define VMMACH_PAGE_MASK	0x1E000	
#define	VMMACH_SEG_SIZE		0x20000
#define VMMACH_SEG_SHIFT	17	
#define	VMMACH_NUM_PAGES_PER_SEG (VMMACH_NUM_PAGES_PER_SEG_INT / VMMACH_CLUSTER_SIZE)

/*
 * The size that page tables are to be allocated in.  This grows software
 * segments in 256K chunks.  The page tables must grow in chunks that are 
 * multiples of the hardware segment size.  This is because the heap and 
 * stack segments grow towards each other in VMMACH_PAGE_TABLE_INCREMENT 
 * sized chunks.  Thus if the increment wasn't a multiple of the hardware 
 * segment size then the heap and stack segments could overlap on a 
 * hardware segment.
 */

#define	VMMACH_PAGE_TABLE_INCREMENT	(((256 * 1024 - 1) / VMMACH_SEG_SIZE + 1) * VMMACH_NUM_PAGES_PER_SEG)

/*
 * The maximum number of kernel stacks.  There is a limit because these
 * use part of the kernel's virtual address space.
 */
#define VMMACH_MAX_KERN_STACKS	128

/*
 * The following sets of constants define the kernel's virtual address space. 
 * Some of the constants are defined in machineConst.h.  The address space 
 * looks like the following:
 *
 *	|-------------------------------|	MACH_KERNEL_START
 *	| Trap vectors.			|
 *	|-------------------------------|	MACH_STACK_BOTTOM
 *	| Invalid page.			|
 *	|-------------------------------|	MACH_STACK_BOTTOM + VMMACH_PAGE_SIZE
 *     	|				|
 *     	| Stack for the initial kernel 	|
 *     	| process.			|
 *     	|				|
 *	---------------------------------	MACH_CODE_START
 *     	|				|
 *     	| Kernel code and data		|
 *     	| 				|
 *	---------------------------------	VMMACH_PMEG_SEG
 *	| 				|
 *	| Stacks, mapped pages, etc.	|
 *	| 				|
 *      ---------------------------------	VMMACH_DEV_START_ADDR
 *	|				|
 *	| Mapped in devices		|
 *	|				|	VMMACH_DEV_END_ADDR
 *	|-------------------------------|	VMMACH_DMA_START_ADDR
 *	|				|
 *	| Space for mapping in DMA bufs |
 *	|				|
 *	|-------------------------------|
 */

/*
 * There is a special area of a users VAS to allow copies between two user
 * address spaces.  This area sits between the beginning of the kernel and
 * the top of the user stack.  It is one hardware segment wide.
 */
#define	VMMACH_MAP_SEG_ADDR		(0xE000000 - VMMACH_SEG_SIZE)

#endif _VMSUN3CONST
