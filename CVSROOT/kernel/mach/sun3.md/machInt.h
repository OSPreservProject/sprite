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

#include "machMon.h"

/*
 * The bounds of the code that copies arguments from the user stack to
 * the kernel stack.
 */
extern int MachFetchArgs();
extern int MachFetchArgsEnd();

/*
 * The number of different exceptions.
 */
#define	MACH_NUM_EXCEPTIONS	64

/*
 * Structure which contains the different functions called for exceptions.
 */

typedef struct {
	int	initSupStackPtr;	/* 00 - initial SSP on reset*/
	int	(*initPC)();		/* 04 - initial PC on reset*/
	int	(*busError)();		/* 08 - bus error */
	int	(*addressError)();	/* 0c - address error */
	int	(*illegalInst)();	/* 10 - illegal instruction */
	int	(*zeroDiv)();		/* 14 - zero divide */
	int	(*chk)();		/* 18 - CHK instruction */
	int	(*trapv)();		/* 1c - TRAPV instruction */
	int	(*privViol)();		/* 20 - privilege violation */
	int	(*traceTrap)();		/* 24 - trace trap */
	int	(*emu1010)();		/* 28 - line 1010 emulator */
	int	(*emu1111)();		/* 2c - line 1111 emulator */
	int	(*res30)();		/* 30 - reserved */
	int	(*res34)();		/* 34 - reserved */
	int	(*stackFmtError)();	/* 38 - RTE format error */
	int	(*unInitIntr)();	/* 3c - unitialized interrupt */
	int	(*res1[8])();		/* 40-5c - reserved */
	int	(*spuriousInt)();	/* 60 - spurious interrupt */
	int	(*autoVec[7])();	/* 64-7c - level 1-7 autovectors */
	int	(*trap[16])();		/* 80-bc - trap instruction vectors */
	int	(*res2[16])();		/* c0-fc - reserved */
} MachVectorTable;

/*
 * The actual table of vectors.
 */
extern	MachVectorTable		*machVectorTablePtr;

/*
 * The prototype table which contains the vectors which are to be copied into
 * the vector table.
 */
extern	MachVectorTable 	machProtoVectorTable;

/*
 * Copy of the boot parameter structure.
 */
extern 	MachMonBootParam	machMonBootParam;

/*
 * Internal functions.
 */
extern	void	MachUserReturn();

#endif _MACHINT
