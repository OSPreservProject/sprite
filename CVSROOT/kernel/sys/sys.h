/*
 * sys.h --
 *
 *     Routines and types for the sys module.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _SYS
#define _SYS

#include "user/sys.h"
#include "sprite.h"
#include "sunSR.h"
#include "status.h"

/*
 *  Flags for Sys_SetIntrLevel.
 */
#define SYS_INTR_DISABLE	SUN_SR_PRIO_7
#define SYS_INTR_ENABLE		SUN_SR_PRIO_0

/*
 * Stuff for system calls.
 *
 * SYS_ARG_OFFSET	Where the system calls arguments begin.
 * SYS_MAX_ARGS		The maximum number of arguments possible to be passed
 *			    to a system call.
 * SYS_ARG_SIZE		The size in bytes of each argument.
 */

#define	SYS_ARG_OFFSET	8
#define	SYS_MAX_ARGS	10
#define	SYS_ARG_SIZE	4

typedef enum {
    SYS_GATHER_PROCESS_INFO
} Sys_InterruptCodes;

typedef enum {
    SYS_USER,
    SYS_KERNEL
} Sys_ProcessorStates;

/*
 * Structure for a Sys_SetJump and Sys_LongJump.
 */

typedef struct {
    int		pc;
    int		regs[12];
} Sys_SetJumpState;

/*
 * Macros
 */

#define	Sys_GetProcessorNumber() 	0

extern	Boolean	sys_KernelMode;
extern	int	sys_NumProcessors;
extern	Boolean	sys_AtInterruptLevel;
extern	int	*sys_NumDisableIntrsPtr;
extern	Boolean	sys_ShuttingDown;
extern	Boolean	sys_ErrorShutdown;
extern	int	sys_NumCalls[];

/*
 * Procedure headers.
 */

extern	ReturnStatus	Sys_SetJump();
extern	void		Sys_UnsetJump();
extern	ReturnStatus	Sys_LongJump();

extern	void	Sys_Init();
extern	void	Sys_Printf();
extern	void	Sys_SafePrintf();
extern	void	Sys_UnSafePrintf();
extern	void	Sys_Panic();
extern	void	Sys_InterruptProcessor();
extern	Sys_ProcessorStates	Sys_ProcessorState();

/*
 *  Declarations of system calls.
 */

extern	ReturnStatus	Sys_GetTimeOfDay();
extern	ReturnStatus	Sys_SetTimeOfDay();
extern	ReturnStatus	Sys_DoNothing();
extern	ReturnStatus	Sys_Shutdown();
extern	ReturnStatus	Sys_GetMachineInfo();

extern  ReturnStatus	Sys_OutputNumCalls();

/*
 *  The following routines are machine-dependent. They
 *  can be found in sunSubr.s.
 */
extern int  Sys_SetIntrLevel();

#define DISABLE_INTR() \
    if (!sys_AtInterruptLevel) { \
	asm("movw #0x2700,sr"); \
	if (sys_NumDisableIntrsPtr[0] < 0) { \
	    Sys_Panic(SYS_FATAL, "Negative interrupt count.\n"); \
	} \
	sys_NumDisableIntrsPtr[0]++; \
    }

#define ENABLE_INTR() \
    if (!sys_AtInterruptLevel) { \
	sys_NumDisableIntrsPtr[0]--; \
	if (sys_NumDisableIntrsPtr[0] < 0) { \
	    Sys_Panic(SYS_FATAL, "Negative interrupt count.\n"); \
	} \
	if (sys_NumDisableIntrsPtr[0] == 0) { \
	    asm("movw #0x2000,sr"); \
	} \
    }

#define	Sys_DisableIntr() \
	asm("movw #0x2700,sr")
    
#define	Sys_EnableIntr() \
	asm("movw #0x2000,sr")

#define	Sys_SetJump(setJumpPtr) \
    SysSetJump((Proc_GetCurrentProc(Sys_GetProcessorNumber()))->setJumpStatePtr = setJumpPtr)

#endif _SYS
