/*
 * vmSunConst.h --
 *
 *     	Virtual memory constants for the Sun4.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _VMSUNCONST
#define _VMSUNCONST

/*
 * Sun3 page table entry masks.  The same for the sun4.
 *
 *    VMMACH_RESIDENT_BIT	The page is physical resident.		31
 *    VMMACH_PROTECTION_FIELD	Access to the protection bits.		30:29
 *    VMMACH_KR_PROT		Kernel read only, user no access.
 *    VMMACH_KRW_PROT		Kernel read/write, user no access.
 *    VMMACH_UR_PROT		Kernel read only, user read only
 *    VMMACH_URW_PROT		Kernel read/write, user read/write.
 *    VMMACH_DONT_CACHE_BIT	Don't cache bit.			28
 *    VMMACH_TYPE_FIELD		The memory type field.			27:26
 *    VMMACH_REFERENCED_BIT	Referenced bit.				25
 *    VMMACH_MODIFIED_BIT	Modified bit.				24
 *    VMMACH_PAGE_FRAME_FIELD	The hardware page frame.		18:0
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
#ifdef sun4c
#define VMMACH_PAGE_FRAME_FIELD	0x0000ffff
#else
#define VMMACH_PAGE_FRAME_FIELD	0x0007ffff
#endif
/*
 * Shift to allow the type field to get set.
 */
#define	VmMachGetPageType(pte) (((pte) >> 26) & 3)
#define	VmMachSetPageType(pte, type) (pte |= (type << 26))

/*
 * Shift pte right by this much to isolate page protection and residence bits.
 */
#define	VMMACH_PAGE_PROT_SHIFT	29
/*
 * Compare shifted pte (above) with this to see if user residence and protection
 * are okay for the user to write to this address.
 */
#define	VMMACH_PTE_OKAY_VALUE	6

/*
 * In the sun4 200 series machines, the 2 high bits may be 00 or 11.  However,
 * 00 and 11 actually go to the same entry in the segment table.  So for
 * address comparisons, one may have to strip the high two bits.  The hole in
 * the sun4c space is the same.
 *	VMMACH_BOTTOM_OF_HOLE	first unusable addr in hole
 *	VMMACH_TOP_OF_HOLE	last unusable addr in hole
 *	VMMACH_ADDR_MASK	mask off usable bits of address
 */
#define	VMMACH_BOTTOM_OF_HOLE	0x20000000
#define	VMMACH_TOP_OF_HOLE	(0xe0000000 - 1)
#define	VMMACH_ADDR_MASK	0x3FFFFFFF

/*
 * Check to see if a virtual address falls inside the hole in the middle
 * of the sun4 address space.
 */
#define	VMMACH_ADDR_CHECK(virtAddr)	\
    (	((unsigned int) (virtAddr)) >=	\
		    ((unsigned int) VMMACH_BOTTOM_OF_HOLE) &&	\
	((unsigned int) (virtAddr)) <=	\
		    ((unsigned int) VMMACH_TOP_OF_HOLE) ? FALSE : TRUE)

/*
 * Check, in assembly, whether a virtual address falls inside the hole in
 * the middle of the sun4 address space.
 */
#define	VMMACH_ADDR_OK_ASM(virtAddr, CheckMore, DoneCheck, answerReg, useReg) \
	clr	answerReg;				\
	set	VMMACH_BOTTOM_OF_HOLE, useReg;		\
	cmp     useReg, virtAddr;			\
	bleu	CheckMore;				\
	nop;						\
	ba	DoneCheck;				\
	nop;						\
CheckMore:						\
	set	VMMACH_TOP_OF_HOLE, useReg;		\
	cmp	virtAddr, useReg;			\
	bgu	DoneCheck;				\
	nop;						\
	set	0x1, answerReg;				\
DoneCheck:


/*
 * Sun memory management unit constants:
 *
 * VMMACH_KERN_CONTEXT		The Kernel's context.
 * VMMACH_NUM_CONTEXTS		The number of contexts. Impl. dependent.
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
#ifdef sun4c
#define VMMACH_NUM_CONTEXTS		8
#else
#define VMMACH_NUM_CONTEXTS		16
#endif

/*
 *	VMMACH_NUM_SEGS_PER_CONTEXT	Number of segments per context:
 * There is a hole in the middle of the address space, so really there's only
 * 2**12 segs per context, but having discontinuous maps is a pain, so we
 * pretend it's all mappable, which means there's 2**14 segs per context, on
 * the sun4.  This is calculated from 256K bytes per segment == 0x40000 bytes
 * per segment == 2**18 bytes per segment into 2**32 bytes per context gives
 * (32-18) == 14 segments per context.  The sparc station has the same number
 * of segments per context since segments are the same size.
 *	VMMACH_NUM_PAGES_PER_SEG_INT	Number of pages per segment
 *					(Implementation dependent)
 *	VMMACH_NUM_PAGE_MAP_ENTRIES	Number of total page map entries
 *	VMMACH_NUM_PMEGS		Number of pmegs
 *	VMMACH_NUM_NET_SEGS		The number of segments used for mapping
 *					net scatter gather arrays aligned to
 *					avoid cache flushing.  This number is
 *					picked by rule of thumb.  Usually there
 *					are no more than 5 elements in a
 *					scatter gather array, so 5 segments
 *					avoids cache flushing.  The 6th segment
 *					is for mapping any elements beyond this
 *					consecutively (cache flushing used).
 *	
 */
#define VMMACH_NUM_SEGS_PER_CONTEXT	0x4000	/* 2**14 */

#ifdef sun4c
#define VMMACH_NUM_PAGES_PER_SEG_INT	64
#define	VMMACH_NUM_PMEGS		128
#define VMMACH_NUM_PAGE_MAP_ENTRIES	(VMMACH_NUM_PMEGS * VMMACH_NUM_PAGES_PER_SEG_INT)
#else
#define VMMACH_NUM_PAGES_PER_SEG_INT	32
#define VMMACH_NUM_PAGE_MAP_ENTRIES	16384
#define	VMMACH_NUM_PMEGS		(VMMACH_NUM_PAGE_MAP_ENTRIES / VMMACH_NUM_PAGES_PER_SEG_INT)
#endif

/* The values of MONSTART and MONEND */
#define VMMACH_DEV_START_ADDR       	0xFFD00000
#define	VMMACH_DEV_END_ADDR		0xFFEFFFFF
#define	VMMACH_DMA_START_ADDR		0xFFF00000
#define	VMMACH_DMA_SIZE			0xC0000		/* still correct? */

#define	VMMACH_NUM_NET_SEGS		6
#define VMMACH_NET_MAP_START		(VMMACH_DEV_START_ADDR -	\
				(VMMACH_NUM_NET_SEGS * VMMACH_SEG_SIZE))
#define	VMMACH_NET_MAP_SIZE		(VMMACH_NUM_NET_SEGS * VMMACH_SEG_SIZE)
#define VMMACH_NET_MEM_START		(VMMACH_DMA_START_ADDR +	\
						VMMACH_DMA_SIZE)
#define	VMMACH_NET_MEM_SIZE		(0x20000-VMMACH_PAGE_SIZE)
#define	VMMACH_FIRST_SPECIAL_SEG	(((unsigned int) VMMACH_DEV_START_ADDR) >> VMMACH_SEG_SHIFT)
						/* why is this one - 1??? */
#define	VMMACH_INV_PMEG			(VMMACH_NUM_PMEGS - 1)
#define	VMMACH_INV_SEG			VMMACH_NUM_SEGS_PER_CONTEXT
#define	VMMACH_INV_CONTEXT		VMMACH_NUM_CONTEXTS

/*
 * Function code constants for access to the different address spaces as
 * defined by the Sun 4 hardware.  These constants determine which
 * address space is accessed when using a load alternate or store alternate
 * or swap alternate instruction.
 *
 * VMMACH_USER_DATA_SPACE		User data space.
 * VMMACH_USER_PROGRAM_SPACE		User program space
 * VMMACH_KERN_DATA_SPACE		Kernel data space.
 * VMMACH_KERN_PROGRAM_SPACE		Kernel program space.
 * VMMACH_FLUSH_SEG_SPACE		Flush segment (in control space).
 * VMMACH_FLUSH_PAGE_SPACE		Flush page (in control space).
 * VMMACH_FLUSH_CONTEXT_SPACE		Flush context (in control space).
 * VMMACH_CACHE_DATA_SPACE		Cache data instruction (in cntrl space).
 * VMMACH_CONTROL_SPACE			Other parts of control space, including
 *						the context register, etc.
 * VMMACH_SEG_MAP_SPACE			Segment map (in control space).
 * VMMACH_PAGE_MAP_SPACE		Page map (in control space).
 */

#define	VMMACH_CONTROL_SPACE		0x2	/* also called system space */
#define	VMMACH_SEG_MAP_SPACE		0x3
#define	VMMACH_PAGE_MAP_SPACE		0x4
#define	VMMACH_USER_PROGRAM_SPACE	0x8
#define	VMMACH_KERN_PROGRAM_SPACE	0x9
#define	VMMACH_USER_DATA_SPACE		0xA
#define	VMMACH_KERN_DATA_SPACE		0xB
#define	VMMACH_FLUSH_SEG_SPACE		0xC
#define	VMMACH_FLUSH_PAGE_SPACE		0xD
#define	VMMACH_FLUSH_CONTEXT_SPACE	0xE
#ifndef sun4c
#define	VMMACH_CACHE_DATA_SPACE		0xF	/* not on sun4c */
#endif

/*
 * Masks for access to different parts of control and device space.
 *
 * VMMACH_CONTEXT_OFF	 	Offset to context register in control space.
 * VMMACH_DIAGNOSTIC_REG	The address of the diagnostic register.
 * VMMACH_USER_DVMA_ENABLE_REG  Bits to enable user DVMA for different VME 
 *				context.
 * VMMACH_USER_DVMA_MAP		Map of VME context to sun user contexts.
 * VMMACH_BUS_ERROR_REG		The address of the bus error register.
 * VMMACH_ADDR_ERROR_REG	Addr of register storing addr of mem error.
 * VMMACH_ADDR_CONTROL_REG	Addr of control register for memory errors.
 * VMMACH_SYSTEM_ENABLE_REG	The address of the system enable register.
 * VMMACH_ETHER_ADDR		Address of ethernet address in the id prom.
 *				On the sun4c, this is in NVRAM in device space
 *				instead.
 * VMMACH_MACH_TYPE_ADDR	Address of machine type in the id prom.
 *				On sun4c, this is in NVRAM.
 * VMMACH_IDPROM_INC		Amount to increment an address when stepping
 *				through the id prom.
 * VMMACH_CACHE_TAGS_ADDR	Address of cache tags in control space.
 * VMMACH_SYNC_ERROR_REG	Address of sync error reg on sun4c.
 * VMMACH_SYNC_ERROR_ADDR_REG	Address of sync error addr reg on sun4c.
 * VMMACH_ASYNC_ERROR_REG	Address of async error reg on sun4c.
 * VMMACH_ASYNC_ERROR_ADDR_REG	Address of async error addr reg on sun4c.
 */

#define	VMMACH_CONTEXT_OFF		0x30000000	/* control space */
#define VMMACH_SYSTEM_ENABLE_REG	0x40000000	/* control space */
#define	VMMACH_CACHE_TAGS_ADDR		0x80000000	/* control space */
#define	VMMACH_SERIAL_PORT_ADDR		0xF0000000	/* control space */

#ifdef sun4c
#define VMMACH_IDPROM_INC		0x01
#define VMMACH_MACH_TYPE_ADDR		0xffd047d9	/* device space */
#define VMMACH_ETHER_ADDR		0xffd047da	/* device space */
/* 4 bus error registers */
#define	VMMACH_CACHE_DATA_ADDR		0x90000000	/* control space */
#define VMMACH_SYNC_ERROR_REG		0x60000000	/* control space */
#define VMMACH_SYNC_ERROR_ADDR_REG	0x60000004	/* control space */
#define VMMACH_ASYNC_ERROR_REG		0x60000008	/* control space */
#define VMMACH_ASYNC_ERROR_ADDR_REG	0x6000000C	/* control space */
#else
#define VMMACH_IDPROM_INC		0x01
#define VMMACH_MACH_TYPE_ADDR		0x01
#define VMMACH_ETHER_ADDR		0x02
#define	VMMACH_USER_DVMA_ENABLE_REG	0x50000000      /* control space */
#define VMMACH_BUS_ERROR_REG		0x60000000	/* control space */
#define VMMACH_ADDR_ERROR_REG		0xffd08004	/* device space */
#define VMMACH_ADDR_CONTROL_REG		0xffd08000	/* device space */
#define VMMACH_DIAGNOSTIC_REG		0x70000000	/* control space */
#define	VMMACH_USER_DVMA_MAP		0xD0000000	/* control space */
/*
 * Bit in address error control register to enable reporting of asynchronous
 * memory errors.
 */
#define	VMMACH_ENABLE_MEM_ERROR_BIT	0x40
#endif

/*
 * Other cache constants:
 */
/*
 * Mask to set enable cache bit in the system enable register.
 * Or it with register to enable the cache.
 *	VMMACH_ENABLE_CACHE_BIT		Bit in system enable register
 *					to turn on the cache.
 *	VMMACH_NUM_CACHE_TAGS		Number of tags in the cache
 *	VMMACH_CACHE_TAG_INCR		Byte size for incrementing thru tags
 *	VMMACH_CACHE_LINE_SIZE		Line size in bytes
 *	VMMACH_NUM_CACHE_LINES		Number of lines in cache
 *	VMMACH_CACHE_SIZE		Total size of cache in bytes of data
 */
#ifdef sun4c
#define	VMMACH_NUM_CACHE_LINES		0x1000	/* 4K * 16 = 64K cache size */
#else
#define	VMMACH_NUM_CACHE_LINES		0x2000	/* 8K * 16 = 128K cache size */
#endif
#define	VMMACH_ENABLE_CACHE_BIT		0x10
#define	VMMACH_CACHE_TAG_INCR		0x10
#define	VMMACH_CACHE_LINE_SIZE		0x10
#define	VMMACH_NUM_CACHE_TAGS		VMMACH_NUM_CACHE_LINES
#define VMMACH_CACHE_SIZE  (VMMACH_CACHE_LINE_SIZE * VMMACH_NUM_CACHE_LINES)

/*
 * Other bits in the system enable register.
 */
#define	VMMACH_ENABLE_DVMA_BIT		0x20

/*
 * The highest virtual address useable by the kernel for both machine type
 * 1 and machine type 2 and 3 and 4...  This seems to be the largest
 * virtual address plus 1...
 */
#define	VMMACH_MACH_TYPE1_MEM_END	0xF40000
#define	VMMACH_MACH_TYPE2_MEM_END	0x1000000
#define	VMMACH_MACH_TYPE3_MEM_END	0x10000000
#define	VMMACH_MACH_TYPE4_MEM_END	0x100000000

/*
 * Masks to extract good bits from the virtual addresses when accessing
 * the page and segment maps.
 */
#ifdef sun4c
#define	VMMACH_PAGE_MAP_MASK	0xFFFFF000
#else
#define	VMMACH_PAGE_MAP_MASK	0xFFFFe000
#endif
#define	VMMACH_SEG_MAP_MASK	0xFFFc0000

/*
 * Mask to get only the low order four bits of a context register.
 */
#define	VMMACH_CONTEXT_MASK		0xF

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
#ifdef sun4c
#define	VMMACH_PAGE_SIZE_INT	4096
#define VMMACH_PAGE_SHIFT_INT	12
#define VMMACH_OFFSET_MASK	0x0fff
#define VMMACH_OFFSET_MASK_INT	0x0fff
#define VMMACH_PAGE_MASK	0x3F000	
#else
#define	VMMACH_PAGE_SIZE_INT	8192
#define VMMACH_PAGE_SHIFT_INT	13
#define VMMACH_OFFSET_MASK	0x1fff
#define VMMACH_OFFSET_MASK_INT	0x1fff
#define VMMACH_PAGE_MASK	0x3E000	
#endif

#define VMMACH_PAGE_SHIFT	(VMMACH_CLUSTER_SHIFT + VMMACH_PAGE_SHIFT_INT)
#define	VMMACH_SEG_SIZE		0x40000		/* twice as large as sun3? */
#define VMMACH_SEG_SHIFT	18		/* twice as large as sun3? */
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
#define	VMMACH_MAP_SEG_ADDR		(MACH_KERN_START - VMMACH_SEG_SIZE)

#endif /* _VMSUNCONST */
