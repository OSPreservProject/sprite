/*
 * machConfig.h --
 *
 *	Declarations of routines and data structures specifying the 
 *	configuration of a SPUR processor.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHCONFIG
#define _MACHCONFIG

/*
 * Entry specifing each board in system. 
 */

typedef struct {
    int		slotId;		/* Slot ID of board. */
    int		boardType;	/* Board type, see below. */
    int		number; 	/* Logical number for the board type. */
    int		flags;		/* Flags, see blow. */
} Mach_Board;

/*
 * Board type values.
 */
#define	MACH_CONFIG_NO_BOARD		0x0	/* No board in slot. */
#define	MACH_CONFIG_CPU_BOARD		0x1	/* SPUR processor board. */
#define	MACH_CONFIG_ETHER_BOARD		0x2	/* NuBus ether net board. */
#define	MACH_CONFIG_MEMORY_MASK		0xf0	/* Mask for memory boards. */
#define	MACH_CONFIG_HALF_MEG_BOARD	0x10	/* 1/2 meg memory board. */
#define	MACH_CONFIG_2_MEG_BOARD		0x20	/* 2 meg memory board. */
#define	MACH_CONFIG_8_MEG_BOARD		0x30	/* 8 meg memory board. */
#define	MACH_CONFIG_16_MEG_BOARD	0x40	/* 16 meg memory board. */
#define	MACH_CONFIG_32_MEG_BOARD	0x50	/* 32 meg memory board. */

/*
 * Board flag values. Valid for all types
 */
#define	MACH_CONFIG_NO_ACCESS_FLAG	0x1	/* No access to Sprite. */
 /*
  * Flags for CPU boards.
  */
#define	MACH_CONFIG_MASTER_FLAG		0x100	/* Master CPU board. */
 /*
  * Flags for memory boards.
  */
#define	MACH_CONFIG_INITIALIZED_FLAG	0x100	/* Memory board has been
						 * initialized. 
						 */
#define	MACH_CONFIG_KERNEL_MEM_FLAG	0x200	/* Board containing kernel. */
extern ReturnStatus Mach_FindBoardDescription();
extern int	    Mach_ConfigMemSize();
extern unsigned int Mach_ConfigInitMem();

#endif

