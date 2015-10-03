/*
 * machTypes.h --
 *
 *     Exported types for the mach module.
 *
 * Copyright 1989 Regents of the University of California
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

#ifndef _MACHTYPES
#define _MACHTYPES

#ifdef KERNEL
#include "devAddrs.h"
#include "machConst.h"
#include "user/fmt.h"
#else
#include <kernel/devAddrs.h>
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
 * The format for the vector offset register.
 */
typedef struct {
    unsigned	int	stackFormat:4;		/* Format of the stack */
    unsigned	int	   	   :2;		/* Filler */
    unsigned	int	vectorOffset:10;	/* Vector offset */
} Mach_VOR;
    
/*
 * The format for the special status word.
 */
typedef	struct {
    unsigned 	int 	rerun: 1,	 /* Rerun bus cycle (0 = processor
					    rerun, 1 = software rerun) */
    			fill1: 1,	 /* Reserved */
    			ifetch: 1,	 /* Instruction fetch to instruction
					    buffer (1 = true) */
			dfetch: 1,	 /* Data fetch to the data input
					    buffer (1 = true) */
			readModWrite: 1, /* Read-Modify-Write cycle */
    			highByte: 1,	 /* High byte transfer */
			byteTrans: 1,	 /* Byte transfer flag */
    			readWrite: 1,	 /* Read/Write flag (0 = write, 
					    1 = read) */
    			fill2: 4,	 /* Reserved */
    			funcCode : 4;	 /* The function code */
} Mach_SpecStatWord;

/*
 * 68020 stack formats are defined in "MC68020 32-Bit Microprocessor User's 
 * Manual" on pages 6-19 to 6-24.  68010 stack formats are defined in 
 * "MC68000  16/32-Bit Microprocessor Programmers Reference Manual" on pages
 * 33 to 48.
 */

#ifdef sun3
/*
 * Address and bus error info that is stored on the stack for a  68020 short
 * bus or address fault.
 */
typedef struct {
    unsigned	int	:16;
    Mach_SpecStatWord	specStatWord;	
    short		pipeStageC;
    short		pipeStageB;
    int			faultAddr;
    unsigned 	int	:16;
    unsigned 	int	:16;
    int			dataOutBuf;
    unsigned 	int	:16;
    unsigned 	int	:16;
} Mach_ShortAddrBusErr;

/*
 * Address and bus error info that is stored on the stack for a 68020 long
 * bus or address fault.
 */
typedef struct {
    unsigned	int	:16;
    Mach_SpecStatWord	specStatWord;	
    short		pipeStageC;
    short		pipeStageB;
    int			faultAddr;
    unsigned 	int	int1;
    int			dataOutBuf;
    unsigned	int	int2[2];
    int			stageBAddr;
    unsigned 	int	int3;
    int			dataInBuf;
    unsigned	int	int4[11];
} Mach_LongAddrBusErr;

/*
 * The default stack is the long one.  Anyone who actually plays with these
 * stacks will have to look at the exception type and figure things out
 * for themselves.
 */
typedef	Mach_LongAddrBusErr	Mach_AddrBusErr;

/*
 * The structure of the exception stack
 */
typedef struct {
    short		statusReg;		/* Status register */
    int			pc;			/* Program counter */
    Mach_VOR		vor;			/* The vector offset register */
    union {
	int		instrAddr;		/* Instruction that caused
						 * the fault. */
	Mach_AddrBusErr	addrBusErr;		/* Address or bus error info */
    } tail;
} Mach_ExcStack;

#else
/*
 * Address and bus error info that is stored on the stack.
 */
typedef struct {
    Mach_SpecStatWord	specStatWord;	
    int			faultAddr;
    unsigned 	int	:16;
    short		dataOutBuf;
    unsigned 	int	:16;
    short		dataInBuf;
    unsigned 	int	:16;
    short		instInBuf;
    short		internal[16];
} Mach_AddrBusErr;

/*
 * The structure of the exception stack
 */
typedef struct {
    short		statusReg;		/* Status register */
    int			pc;			/* Program counter */
    Mach_VOR		vor;			/* The vector offset register */
    union {
	Mach_AddrBusErr	addrBusErr;		/* Address or bus error info */
    } tail;
} Mach_ExcStack;

#endif

/*
 * Bus error register 
 */
#ifdef sun3
typedef struct {
    unsigned	int	fill:24,	/* Filler to make it a long. */
			pageInvalid: 1,	/* The page accessed did not have the
					   valid bit set. */
			protError: 1,	/* Protection error. */
			timeOut: 1,	/* Timeout error. */
			vmeBusErr: 1,	/* VME bus error. */
			fpaBusErr: 1,	/* FPA bus error. */
			fpaEnErr: 1,	/* FPA Enable error. */
			res1:1,		/* Reserved. */
			watchdog:1;	/* Watchdog or user reset. */
} Mach_BusErrorReg;
#else
typedef struct {
    unsigned	int	fill:16,	/* Filler to make it a long. */
			res1:8,		/* Reserved */
			resident:1,	/* Valid bit set */
			busErr:1,	/* System bus error */
			res2:2,		/* Reserved */
			protErr:1,	/* Protection error */
			timeOut:1,	/* Timeout error */
			parErrU:1,	/* Parity error upper byte */
			parErrL:1;	/* Parity error lower byte */
} Mach_BusErrorReg;

#endif

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
    int			regs[MACH_NUM_GPRS];	/* General purpose registers.*/
    int			pc;			/* The program counter. */
    int			statusReg;		/* The status register. */
} Mach_RegState;

#ifdef sun3
/*
 * The structure of the fpu state frames is described in
 * section 6.4.2 of the Motorola 68881/2 User's Manual.
 */
struct fpuState {
    unsigned char version;
    unsigned char state;
    unsigned short xxx1;
    unsigned long  xxx2[(MACH_FP_STATE_SIZE/4) - 1];
};

#define     MACH_68881_IDLE_STATE     0x18
#define     MACH_68882_IDLE_STATE     0x38
#define     MACH_68881_BUSY_STATE     0xb4
#define     MACH_68882_BUSY_STATE     0xd4

extern const unsigned long      mach68881Present;
extern const unsigned long      mach68881NullState;
extern const unsigned char      mach68881Version;
extern const struct fpuState    mach68881IdleState;
#endif

/*
 * The user state for a process.
 */
typedef struct {
    Address		userStackPtr;		/* The user stack pointer */
    int			trapRegs[MACH_NUM_GPRS];/* General purpose registers.*/
    Mach_ExcStack	*excStackPtr;		/* The exception stack */
    int			lastSysCall;		/* Last system call. */
#ifdef sun3
    long    trapFpRegs[MACH_NUM_FPRS][3];       /* Floating point registers */
    long    trapFpCtrlRegs[3];                  /* fpu control registers */
    struct fpuState trapFpuState;               /* internal state of the fpu*/
#endif
} Mach_UserState;

/*
 * The kernel and user state for a process.
 */
typedef struct Mach_State {
    Mach_UserState  userState;		        /* User state for a process. */
    long	 switchRegs[MACH_NUM_GPRS];     /* Where registers are saved
						 * and restored to/from during
						 * context switches. */
    Address      kernStackStart;		/* Address of the beginning of
						 * the kernel stack. */
    int	         sigExcStackSize;	        /* Amount of valid data in the
						 * signal exception stack. */
    Mach_ExcStack   sigExcStack;		/* Place to store sig exception 
						 * stack on return from signal
						 * handler.*/
#if 0
    long  switchFpRegs[MACH_NUM_FPRS][3];       /* Where fpu registers are
                                                 * saved and restored to/from
						 * during context switches. */
    long  switchFpCtrlRegs[3];                  /* fpu control registers */
    struct fpuState switchFpuState;             /* internal state of the fpu*/
#endif
} Mach_State;

/*
 * The stack that is created when a trap occurs.   If you change the size
 * of this structure then you must also change the MACH_TRAP_INFO_SIZE constant
 * in machConst.h.
 */
typedef struct {
    int			trapType;	/* Type of trap. */
    Mach_BusErrorReg	busErrorReg;	/* Bus error register at time of trap.*/
    int			tmpRegs[4];	/* The tmp registers d0, d1, a0 and 
					 * a1.*/
    Mach_ExcStack	excStack;	/* The exception stack. */
} Mach_TrapStack;

/*
 * The stack that is created when an interrupt occurs.
 */
typedef struct {
    int			tmpRegs[4];	/* The tmp registers d0, d1, a0 and 
					 * a1.*/
    Mach_ExcStack	excStack;	/* The exception stack. */
} Mach_IntrStack;

/*
 * The machine dependent signal structure.
 */
typedef struct {
    int		  	trapInst;	/* The trap instruction that is
					 * executed upon return. */
    Mach_UserState	userState;	/* The user process machine state
					 * info. */
    Mach_ExcStack	excStack;	/* The exception stack that would
					 * have been restored if this signal
					 * were not taken. */
} Mach_SigContext;

#endif /* _MACHTYPES */
