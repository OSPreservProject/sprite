/* machDis.c -
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

/*
 * Coprocessor branch masks.
 */
#define COPz_BC_MASK	0x1a
#define COPz_BC		0x08
#define COPz_BC_TF_MASK	0x01
#define COPz_BC_TRUE	0x01
#define COPz_BC_FALSE	0x00

/*
 * Coprocessor 1 operation.
 */
#define OP_COP_1	0x11

unsigned *GetBranchDest();

unsigned *
MachEmulateBranch(regsPtr, instPC, fpcCSR, allowNonBranch)
    unsigned	*regsPtr;
    Address	instPC;
    unsigned	fpcCSR;
    Boolean	allowNonBranch;
{
    JTypeFmt	*jInstPtr;
    RTypeFmt	*rInstPtr;
    ITypeFmt	*iInstPtr;
    unsigned	*retAddr;

#ifdef notdef
    printf("regsPtr=%x PC=%x Inst=%x fpcCsr=%x\n", regsPtr, instPC,
					       *(unsigned *)instPC, fpcCSR);
#endif

    jInstPtr = (JTypeFmt *)instPC;
    rInstPtr = (RTypeFmt *)instPC;
    iInstPtr = (ITypeFmt *)instPC;

    switch ((int)jInstPtr->op) {
        case OP_SPECIAL:
	    switch ((int)rInstPtr->funct) {
		case OP_JR:
		case OP_JALR:
		    retAddr = (unsigned *)regsPtr[rInstPtr->f1];
		    break;
		default:
		    if (allowNonBranch) {
			retAddr = (unsigned *)(instPC + 4);
		    } else {
			panic("MachEmulateBranch: Non-branch\n");
		    }
		    break;
	    }
	    break;
        case OP_BCOND:
	    switch ((int)iInstPtr->f2) {
		case OP_BLTZ:
		case OP_BLTZAL:
		    if ((int)(regsPtr[rInstPtr->f1]) < 0) { 
			retAddr = GetBranchDest(iInstPtr);
		    } else {
			retAddr = (unsigned *) (instPC + 8);
		    }
		    break;
		case OP_BGEZAL:
		case OP_BGEZ:
		    if ((int)(regsPtr[rInstPtr->f1]) >= 0) { 
			retAddr = GetBranchDest(iInstPtr);
		    } else {
			retAddr = (unsigned *) (instPC + 8);
		    }
		    break;
		default:
		    panic("MachEmulateBranch: Bad branch cond\n");
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
	    if (regsPtr[rInstPtr->f1] == 
	        regsPtr[rInstPtr->f2]) {
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
        case OP_BNE:
	    if (regsPtr[rInstPtr->f1] != 
	        regsPtr[rInstPtr->f2]) {
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
        case OP_BLEZ:
	    if ((int)(regsPtr[rInstPtr->f1]) <= 0) { 
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
        case OP_BGTZ:
	    if ((int)(regsPtr[rInstPtr->f1]) > 0) { 
		retAddr = GetBranchDest(iInstPtr);
	    } else {
		retAddr = (unsigned *) (instPC + 8);
	    }
	    break;
	case OP_COP_1: {
	    Boolean	condition;

	    if ((rInstPtr->f1 & COPz_BC_MASK) == COPz_BC) {
		if ((rInstPtr->f2 & COPz_BC_TF_MASK) == COPz_BC_TRUE) {
		    condition = fpcCSR & MACH_FPC_COND_BIT;
		} else {
		    condition = !(fpcCSR & MACH_FPC_COND_BIT);
		}
		if (condition) {
		    retAddr = GetBranchDest(iInstPtr);
		} else {
		    retAddr = (unsigned *) (instPC + 8);
		}
	    } else if (allowNonBranch) {
		retAddr = (unsigned *)(instPC + 4);
	    } else {
		panic("MachEmulateBranch: Bad coproc branch instruction\n");
	    }
	}
	default:
	    if (allowNonBranch) {
		retAddr = (unsigned *)(instPC + 4);
	    } else {
		panic("MachEmulateBranch: Non-branch instruction\n");
	    }
	    break;
    }
#ifdef notdef
    printf("Target addr=%x\n", retAddr);
#endif
    return(retAddr);
}

static unsigned *
GetBranchDest(iInstPtr)
    ITypeFmt	*iInstPtr;
{
    return((unsigned *)((Address)iInstPtr + 4 + ((short)iInstPtr->imm << 2)));
}
