/*
 * machTypes.h --
 *
 *     	Exported structures for the mach module.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHTYPES
#define _MACHTYPES

#ifdef KERNEL
#include "machConst.h"
#include "user/fmt.h"
#else
#include <kernel/machConst.h>
#include <fmt.h>
#endif

/*
 * The state of each processor: user mode or kernel mode.
 */
typedef enum {
    MACH_USER,
    MACH_KERNEL
} Mach_ProcessorStates;

/*
 * State for each process.
 *
 * IMPORTANT NOTE: If the order or size of fields in these structures change
 *		   then the constants which give the offsets must be
 *		   changed in "machConst.h".
 */

/*
 * The register state of a user process which is passed to debuggers.
 */
typedef struct {
    Address		pc;			/* The program counter. */
    unsigned		regs[MACH_NUM_GPRS];	/* General purpose registers.*/
    unsigned		fpRegs[MACH_NUM_FPRS];	/* The floating point
						 * registers. */
    unsigned		fpStatusReg;		/* The floating point status
						 * register. */
    unsigned		mflo, mfhi;		/* Multiply lo and hi
						 * registers. */
} Mach_RegState;

/*
 * The user state for a process.
 */
typedef struct {
    Mach_RegState	regState;		/* State of a process after
						 * a trap. */
    int			unixRetVal;		/* Return value from a
						 * UNIX system call. */
    int                 savedV0, savedA3;       /* For restarting calls. */
} Mach_UserState;

/*
 * The kernel and user state for a process.
 */
typedef struct Mach_State {
    Mach_UserState	userState;		/* User state for a process. */
    Mach_RegState	switchRegState;		/* The state to save on a
						 * context switch. */
    Address		kernStackStart;		/* Address of the beginning of
						 * the kernel stack. */
    Address		kernStackEnd;		/* Address of the end of the
						 * the kernel stack. */
    unsigned		sstepInst;		/* The instruction that we
						 * replaced to do a single
						 * step. */
    unsigned		tlbHighEntry;		/* The TLB high entry value
						 * for the first stack page. */
    unsigned		tlbLowEntries[MACH_KERN_STACK_PAGES - 1];
    						/* The TLB low entry values
						 * for the mapped stack
						 * pages. */
} Mach_State;

/*
 * The machine dependent signal structure.
 */
typedef struct {
    int		  	break1Inst;	/* The instruction that is
					 * executed upon return. */
    Mach_UserState	userState;	/* The user process machine state
					 * info. */
    unsigned		fpRegs[MACH_NUM_FPRS];	/* The floating point
						 * registers. */
    unsigned		fpStatusReg;		/* The floating point status
						 * register. */
} Mach_SigContext;

/*
 * The structure used by the debugger to hold the machine state.
 */
typedef struct {
    int		regs[MACH_NUM_GPRS];
    int		fpRegs[MACH_NUM_FPRS];
    unsigned	sig[32];
    unsigned	excPC;
    unsigned	causeReg;
    unsigned	multHi;
    unsigned	multLo;
    unsigned	fpCSR;
    unsigned	fpEIR;
    unsigned	trapCause;
    unsigned	trapInfo;
    unsigned	tlbIndex;
    unsigned	tlbRandom;
    unsigned	tlbLow;
    unsigned	tlbContext;
    unsigned	badVaddr;
    unsigned	tlbHi;
    unsigned	statusReg;
} Mach_DebugState;

/*
 *  Information from reading a TURBOchannel ROM.
 */

typedef struct {
    char	revision[9];	/* Firmware revision. */
    char	vendor[9];	/* Vendor name. */
    char	module[9];	/* Module name. */
    char	type[5];	/* Host firmware type. */
} Mach_SlotInfo;

#endif /* _MACHTYPES */
