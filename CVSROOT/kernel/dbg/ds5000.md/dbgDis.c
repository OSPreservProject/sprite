/* dbgDis.c -
 *
 *     	This contains the routine which disassembles an instruction to find
 *	the target.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "mach.h"
#include "dbgInt.h"

extern Mach_DebugState	mach_DebugState;

/*
 * Define the three basic types of instruction formats.
 */
typedef struct {
  unsigned imm: 16;
  unsigned f2: 5;
  unsigned f1: 5;
  unsigned op: 6;
} ITypeFmt;

typedef struct {
  unsigned target: 26;
  unsigned op: 6;
} JTypeFmt;

typedef struct {
  unsigned funct: 6;
  unsigned f4: 5;
  unsigned f3: 5;
  unsigned f2: 5;
  unsigned f1: 5;
  unsigned op: 6;
} RTypeFmt;

/*
 * Opcodes of the branch instructions.
 */
#define OP_SPECIAL	0x00
#define OP_BCOND	0x01
#define OP_J		0x02
#define	OP_JAL		0x03
#define OP_BEQ		0x04
#define OP_BNE		0x05
#define OP_BLEZ		0x06
#define OP_BGTZ		0x07

/*
 * Branch subops of the special opcode.
 */
#define OP_JR		0x08
#define OP_JALR		0x09

/*
 * Sub-ops for OP_BCOND code.
 */
#define OP_BLTZ		0x00
#define OP_BGEZ		0x01
#define OP_BLTZAL	0x10
#define OP_BGEZAL	0x11

unsigned *GetBranchDest();


/*
 * ----------------------------------------------------------------------------
 *
 * DbgGetDestPC --
 *
 *	Return the destination program counter of the instruction at the
 *	given address.
 *
 * Results:
 *	Destination PC of the given instruction.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
unsigned *
DbgGetDestPC(instPC)
    Address	instPC;
{
    JTypeFmt	*jInstPtr;
    RTypeFmt	*rInstPtr;
    ITypeFmt	*iInstPtr;
    unsigned	*retAddr;
    int		dummy;

    if (dbgTraceLevel >= 1) {
	printf("Inst=%x\n", *(unsigned *)instPC);
    }

    jInstPtr = (JTypeFmt *)instPC;
    rInstPtr = (RTypeFmt *)instPC;
    iInstPtr = (ITypeFmt *)instPC;

    dummy = jInstPtr->op;
    switch (dummy) {
        case OP_SPECIAL:
	    dummy = rInstPtr->funct;
	    switch (dummy) {
		case OP_JR:
		case OP_JALR:
		    retAddr = (unsigned *)mach_DebugState.regs[rInstPtr->f1];
		    break;
		default:
		    retAddr = (unsigned *)(instPC + 4);
		    break;
	    }
	    break;
        case OP_BCOND:
	    dummy = iInstPtr->f2;
	    switch (dummy) {
		case OP_BLTZ:
		case OP_BLTZAL:
		    if ((int)(mach_DebugState.regs[rInstPtr->f1]) < 0) { 
			retAddr = GetBranchDest(iInstPtr);
		    } else {
			retAddr = (unsigned *) (instPC + 8);
		    }
		    break;
		case OP_BGEZAL:
		case OP_BGEZ:
		    if ((int)(mach_DebugState.regs[rInstPtr->f1]) >= 0) { 
			retAddr = GetBranchDest(iInstPtr);
		    } else {
			retAddr = (unsigned *) (instPC + 8);
		    }
		    break;
		default:
		    printf("DbgGetDestPC: Bad branch cond\n");
		    retAddr = (unsigned *)(instPC + 4);
		    break;
	    }
	    break;
        case OP_J:
        case OP_JAL:
	    retAddr = (unsigned *) ((jInstPtr->target << 2) | 
				   ((unsigned)instPC & 0xF0000000));
	    break;
	    break;
        case OP_BEQ:
	    if (mach_DebugState.regs[rInstPtr->f1] == 
	        mach_DebugState.regs[rInstPtr->f2]) {
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
        case OP_BNE:
	    if (mach_DebugState.regs[rInstPtr->f1] != 
	        mach_DebugState.regs[rInstPtr->f2]) {
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
        case OP_BLEZ:
	    if ((int)(mach_DebugState.regs[rInstPtr->f1]) <= 0) { 
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
        case OP_BGTZ:
	    if ((int)(mach_DebugState.regs[rInstPtr->f1]) > 0) { 
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
	default:
	    retAddr = (unsigned *)(instPC + 4);
	    break;
    }
    if (dbgTraceLevel >= 1) {
	printf("Target addr=%x\n", retAddr);
    }
    return(retAddr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * GetBranchDest --
 *
 *	Return the destination of the given branch instruction.
 *
 * Results:
 *	The destination of the given branch instruction.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static unsigned *
GetBranchDest(iInstPtr)
    ITypeFmt	*iInstPtr;
{
    return((unsigned *)((Address)iInstPtr + 4 + ((short)iInstPtr->imm << 2)));
}
