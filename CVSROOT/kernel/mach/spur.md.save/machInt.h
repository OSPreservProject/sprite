/*
 * machInt.h --
 *
 *	This file defines things that are shared between the "mach" modules
 *	but aren't used by the rest of Sprite.
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

#ifndef _MACHINT
#define _MACHINT

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern int MachFetchArgs();
extern int MachFetchArgsEnd();


/*
 * The interrupt mask registers.
 */
extern	unsigned int	machNonmaskableIntrMask;
extern	unsigned int	machIntrMask;
extern  unsigned int	machDbgInterruptMask;

/*
 * Slotid <-> processor number mappings. 
 */
extern int	machMapSlotIdToPnum[];
extern int	machMapPnumToSlotId[];

/*
 * Mach_MapPnumToSlotId(pnum) - Map a processor number to the slot Id of that's
 * processor's CPU board.
 */

#define Mach_MapPnumToSlotId(pnum)      (machMapPnumToSlotId[pnum])

/*
 * Refresh the CPU's dynamic registers.
 */

#define	REFRESH_CPU()	asm("cmp_trap	always,r0,r0,$3");
/*
 * Internal functions.
 */
extern	void	MachUserReturn();
extern 	void	Mach_CPC_Init();
extern	void	Mach_RefreshStart();
extern	void	Mach_RefreshInterrupt();

#endif _MACHINT
