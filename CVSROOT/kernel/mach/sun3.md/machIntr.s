|* machIntr.s -
|*
|*     Contains the interrupt handlers.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

#include "machConst.h"
#include "machAsmDefs.h"

.data
.asciz "$Header$ SPRITE (Berkeley)"
	.even
	.globl	_machSpuriousCnt
_machSpuriousCnt:	.long	0
	.globl	_machLevel1Cnt
_machLevel1Cnt:	.long	0
	.globl	_machLevel2Cnt
_machLevel2Cnt:	.long	0
	.globl	_machLevel3Cnt
_machLevel3Cnt:	.long	0
	.globl	_machLevel4Cnt
_machLevel4Cnt:	.long	0
	.globl	_machLevel5Cnt
_machLevel5Cnt:	.long	0
	.globl	_machLevel6Cnt
_machLevel6Cnt:	.long	0

.even
.text

|*
|* ----------------------------------------------------------------------------
|*
|* Interrupt handling --
|*
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

	.globl	MachSpurious
MachSpurious:
	addl	#1,_machSpuriousCnt
	CallTrapHandler(MACH_SPURIOUS_INT)

	.globl	MachLevel1Int
MachLevel1Int:
	addl	#1,_machLevel1Cnt
	CallTrapHandler(MACH_LEVEL1_INT)

	.globl	MachLevel2Int
MachLevel2Int:
	addl	#1,_machLevel2Cnt
    	CallInterruptHandler(_Dev_SCSIIntr)

	.globl	MachLevel3Int
MachLevel3Int:
	addl	#1,_machLevel3Cnt
	CallInterruptHandler(_Net_Intr)

	.globl	MachLevel4Int
MachLevel4Int:
	addl	#1,_machLevel4Cnt
	CallTrapHandler(MACH_LEVEL4_INT)

	.globl	MachLevel5Int
MachLevel5Int:
	addl	#1,_machLevel5Cnt
	CallInterruptHandler(_Timer_TimerServiceInterrupt)

	.globl	MachLevel6Int
MachLevel6Int:
	addl	#1,_machLevel6Cnt
    	CallInterruptHandler(_Dev_KbdServiceInterrupt)
