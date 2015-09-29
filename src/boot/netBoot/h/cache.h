/*	@(#)cache.h 0.1 86/01/19 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Header file for the Sun-3 Sirius cache. 
 */ 
#define CX_OFF          (0x30000000)    /* context registers */
#define POP_MASK        0x7cfc          /* register pop "moveml" mask */
#define PUSH_MASK       0x3f3e          /* register push "moveml" mask */
#define MEMORY_LOW      0x10000         /* base memory address to test */

#define TAG_MASK        0xffff0000      /* cache tags address fld mask */
#define PAGE_MASK       0xffffe000      /* page mask */
#define CACHEPAGE       0xc0000000      /* w/r enabled, cache pg enabled */
#define SGSHIFT         17              /* binary bits per segment */
#define PAGEADDRBITS    0xffffe000      /* page address bits mask */

#define	ENABLE_REG	0x40000000	/* system enable reg */
#define ENA_CACHE_BIT   4               /* r/w - enable external cache */
#define	EN_NORMAL	0x80		/* turns cache off */
#define	EN_CACHE	0x10		/* turns cache on */
#define	FC_MMU		0x03		/* to talk to mmu  */

#define	CACHE_SIZE	0x10000		 /* cache size */
#define CACHETOFF       0x80000000       /* cache data */
#define CACHEDOFF       0x90000000       /* cache tags */
#define FLUSH_OFF       0xa0000000       /* flush cache */
#define NCACHEWDS       16384            /* number of cache data words */
#define CACHEDINCR      4                /* cache data address increment */
#define NCACHETGS       4096             /* number of cache tag words */
#define	CACHETMASK	0xcfff3700       /* cache tags wr/rd mask */
#define CACHETINCR      0x10             /* cache tag adr incr */
#define BYTES_PER_BLOCK	16		 /* bytes per cache block */	
#define FLUSH_CX        1                /* flush context */
#define FLUSH_PG        2                /* flush page */
#define FLUSH_SG        3                /* flush segment */
/* new def's for flushing setting etc */
/*
 *Virtual Address Cache (VAC) control space addresses
 */

#define VAC_TAGS_BASE   0x80000000      /* cache tags control space base */
#define VAC_DATA_BASE   0x90000000      /* cache data control space base */
#define VAC_FLUSH_BASE  0xa0000000      /* flush cache control space base */

/*
 * Physical Parameters of Virtual Address Cache (VAC)
 */

#define VAC_BYTES_PER_BLOCK 16               /* bytes per block */

/*
 * The Virtual Address Cache (VAC) has a counter which cycles bits 4 thru 8.
 * The software must cycle bits beyond that.
 */

#define VAC_FLUSH_LOWBIT        9
#define VAC_FLUSH_HIGHBIT       15              /* 64 KB VAC */
#define VAC_FLUSHALL            0
#define VAC_FLUSH_ALL_COUNT     0x7f
#define VAC_FLUSH_INCRMNT       512             /* bytes per flush */
#define VAC_SIZE                0x10000         /* cache size */
 
/*
 * Number of cycles to flush a context, segment and page.
 * For a context flush or segment flush, we loop
 * 2^(VAC_HIGH_BIT - VAC_LOW_BIT +1) times, and for a page flush,
 * we loop 2^(PHSHIFT - VAC_LOW_BIT) times.  Because we use a dbra
 * loop, the constants passed are one less.
 */
#define VAC_CTXFLUSH_COUNT      127
#define VAC_SEGFLUSH_COUNT      127
#define VAC_PAGEFLUSH_COUNT     15

/* Constants to do a flush */

#define VAC_CTXFLUSH            1
#define VAC_PAGEFLUSH           2
#define VAC_SEGFLUSH            3

/* VAC Read/Write Cache Tags. */

#define VAC_RWTAG_LOWBIT        4
#define VAC_RWTAG_HIGHBIT       15              /* 64 KB VAC */
#define VAC_RWTAG_INCRMNT       16
#define VAC_RWTAG_COUNT         4095            /* count - 1 for dbra */

/* CPU Cache Defines */

#define CACR_CLEAR 0x8
#define CACR_ENABLE 0x1
#define CACR_CLEAR_ENT 0x4
