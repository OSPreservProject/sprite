|* excIntr.s -
|*
|*     Contains the interrupt handlers.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

#include "excAsm.h"
#include "asmDefs.h"

.data
.asciz "$Header$ SPRITE (Berkeley)"
	.even
	.globl	_excSpuriousCnt
_excSpuriousCnt:	.long	0
	.globl	_excLevel1Cnt
_excLevel1Cnt:	.long	0
	.globl	_excLevel2Cnt
_excLevel2Cnt:	.long	0
	.globl	_excLevel3Cnt
_excLevel3Cnt:	.long	0
	.globl	_excLevel4Cnt
_excLevel4Cnt:	.long	0
	.globl	_excLevel5Cnt
_excLevel5Cnt:	.long	0
	.globl	_excLevel6Cnt
_excLevel6Cnt:	.long	0

.even
.text

|*
|* ----------------------------------------------------------------------------
|*
|* Interrupt handling --
|*     Handle exceptions.  Enter the debugger for unsuported interrupts.
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*

	.globl	Exc_Spurious
Exc_Spurious:
	addl	#1,_excSpuriousCnt
	CallTrapHandler(EXC_SPURIOUS_INT)

	.globl	Exc_Level1Int
Exc_Level1Int:
	addl	#1,_excLevel1Cnt
    	CallTrapHandler(EXC_LEVEL1_INT)

	.globl	Exc_Level2Int
Exc_Level2Int:
	addl	#1,_excLevel2Cnt
    	CallInterruptHandler(_Dev_SCSIIntr)

	.globl	Exc_Level3Int
Exc_Level3Int:
	addl	#1,_excLevel3Cnt
	CallInterruptHandler(_Net_Intr)

	.globl	Exc_Level4Int
Exc_Level4Int:
	addl	#1,_excLevel4Cnt
    	CallTrapHandler(EXC_LEVEL4_INT)

	.globl	Exc_Level5Int
Exc_Level5Int:
	addl	#1,_excLevel5Cnt
	CallInterruptHandler(_Timer_ServiceInterrupt)

	.globl	Exc_Level6Int
Exc_Level6Int:
	addl	#1,_excLevel6Cnt
    	CallInterruptHandler(_Dev_KbdServiceInterrupt)
