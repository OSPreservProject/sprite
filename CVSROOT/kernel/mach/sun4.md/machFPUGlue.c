/* 
 * machFPUGlue.c --
 *
 *	Routines that emulate the SunOS routines called by the
 *	SunOS floating point simulator for SPARC.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

/*
 * Too much of the sun fpu stuff just won't lint.
 */
#ifndef lint

#include "sys/types.h"
#include "fpu_simulator.h"
#include "vm.h"


/*
 *----------------------------------------------------------------------
 *
 * fuword --
 *
 *	Read a word from the user's address space.
 *
 * Results:
 *	The word read or -1 if read fails.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
fuword(address)
    caddr_t address;
{
    ReturnStatus status;
    int		value;
    status = Vm_CopyIn(sizeof(int), (Address) address, (Address) &value);

     return (status == SUCCESS) ? value : -1;

}


/*
 *----------------------------------------------------------------------
 *
 * fubyte --
 *
 *	Read a byte from the user's address space.
 *
 * Results:
 *	The byte read or -1 if read fails.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
fubyte(address)
    caddr_t address;
{
    ReturnStatus status;
    unsigned char	value;
    status = Vm_CopyIn(sizeof(char), (Address) address, (Address) &value);

     return (status == SUCCESS) ? value : -1;

}


/*
 *----------------------------------------------------------------------
 *
 * suword --
 *
 *	Store a word into the user's address space.
 *
 * Results:
 *	The 0 if SUCCESS or -1 if write fails.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
suword(address,value)
    caddr_t address;
    int	value;
{
    ReturnStatus status;
    status = Vm_CopyOut(sizeof(int), (Address) &value, (Address) address);

     return (status == SUCCESS) ? 0 : -1;

}


void
MachFPU_Emulate(processID, instAddr, userRegsPtr, curWinPtr, fpuStatePtr)
    int		processID;
    Address	instAddr;
    Mach_RegState 	*userRegsPtr;
    Mach_RegWindow	*curWinPtr;
    Mach_FPUState	*fpuStatePtr;
{
    enum ftt_type result;

    result = fp_emulator(instAddr, userRegsPtr, curWinPtr, fpuStatePtr);
    switch (result) {
    case ftt_none:
	break;
    case ftt_ieee:
	(void) Sig_Send(SIG_ARITH_FAULT, SIG_ILL_INST_CODE, processID, FALSE);
	break;
    case ftt_unimplemented:
	(void) Sig_Send(SIG_ILL_INST, SIG_ILL_INST_CODE, processID, FALSE);
	break;
    case ftt_alignment:
	(void) Sig_Send(SIG_ADDR_FAULT, SIG_ADDR_ERROR, processID,FALSE);
	break;
    case ftt_fault:
	(void) Sig_Send(SIG_ADDR_FAULT, SIG_ACCESS_VIOL, processID, FALSE);
    case ftt_7:
    default:
	break;
    }
}
_fp_read_pfreg() { } 
_fp_write_pfreg() { }

#endif /* lint */
