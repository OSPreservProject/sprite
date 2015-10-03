/*
 * reg.h --
 *
 *	Definitions of registers.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _REG
#define _REG

/*
 * The different type of registers.  See loMem.s for an actual explanation
 * for the reasoning behind the temporary register conventions.
 *
 *	r1-r3,r5-r8:	Globals used by the Lisp system and are left untouched.
 *	r4:		The spill stack.
 *	r9:		Global that we can use as a temporary.
 *	r10:		Where the PC to return to when a trap occurs is stored
 *	r11-r15:	Off limits because these are the previous windows
 *			output registers and it might be in the middle
 *			of using them.  If one of these registers is used
 *			then it must be saved and restored.
 *	r16:		Where the 2nd PC is stored when a trap occurs.
 *	r17-r25:	Temporaries.  r17-r19 can be changed by the macros
 *			in machAsmDefs.h and r23-r25 are off limits to the
 *			interrupt handler for reasons described under
 *			"SWITCHING TO KERNEL MODE" below.
 *	r26:		Callers return PC.
 *	r27-r31:	Output registers.  Generally unused in here except
 *			by the instruction parsing routines and to pass args
 *			to C routines.
 *
 *	SPILL_SP	(r4)	Register that is the spill stack pointer.
 *	RETURN_ADDR_REG	(r10)	Where the return address from a function call
 *				is stored.
 *	CUR_PC_REG	(r10)	Register where the first PC is stored on a trap.
 *	NEXT_PC_REG	(r16)	Register where the 2nd PC is stored on a trap.
 *	KPSW_REG	(r17)	The type of trap that occured.
 *	VOL_TEMP[1-3]	(r18-r20)
 *				Volatile temporary register.  Means that 
 *				macros in machAsmDefs.h can modify this 
 *				register.
 *	SAFE_TEMP[1-3]	(r21-r23)
 *				Registers that cannot be modified by macros or
 *				subroutines within the same window.
 *	NON_INTR_TEMP[1-2] (r24-r25)
 *				Registers that cannot be modified by interrupt
 *				handlers.
 *	INPUT_REG[1-5]	(r11-r15)
 *				Arguments to current routine.
 *	OUTPUT_REG[1-5]	(r27-r31)
 *				Arguments to pass to routine that are calling.
 *	RETURN_VAL_REG	(r27)	Where a value is returned from a C routine.
 *	RETURN_VAL_REG_CHILD (11)
 *				Where to return a value to our caller.
 *	
 */
#define	SPILL_SP		r4
#define	RETURN_ADDR_REG		r10
#define	CUR_PC_REG		r10
#define	NEXT_PC_REG		r16
#define	KPSW_REG		r17
#define	INPUT_REG1		r11
#define	INPUT_REG2		r12
#define	INPUT_REG3		r13
#define	INPUT_REG4		r14
#define	INPUT_REG5		r15
#define	VOL_TEMP1		r18
#define	VOL_TEMP2		r19
#define	VOL_TEMP3		r20
#define	SAFE_TEMP1		r21
#define	SAFE_TEMP2		r22
#define	SAFE_TEMP3		r23
#define	NON_INTR_TEMP1		r24
#define	NON_INTR_TEMP2		r25
#define	OUTPUT_REG1		r27
#define	OUTPUT_REG2		r28
#define	OUTPUT_REG3		r29
#define	OUTPUT_REG4		r30
#define	OUTPUT_REG5		r31
#define	RETURN_VAL_REG		r27
#define	RETURN_VAL_REG_CHILD	r11

#endif _REG
