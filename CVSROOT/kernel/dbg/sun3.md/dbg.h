/*
 * dbg.h --
 *
 *     Exported types and procedure headers for the debugger module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBG
#define _DBG

#ifndef _SPRITE
#include "sprite.h"
#endif
#include "dbgRs232.h"
/*
 * Variable to indicate if are using the rs232 debugger or the network debugger.
 */
extern	Boolean	dbg_Rs232Debug;

/*
 * Variable to indicate that dbg wants a packet.
 */
extern	Boolean	dbg_UsingNetwork;

/*
 * Variable that indicates that we are under control of the debugger.
 */
extern	Boolean	dbg_BeingDebugged;

/*
 * The maximum stack address.
 */
extern	int	dbgMaxStackAddr;

/*
 * Debugger using syslog to dump output of call command or not.
 */
extern	Boolean	dbg_UsingSyslog;

/*
 * The different opcodes that kdbx can send us.
 */

typedef enum {
    DBG_READ_ALL_GPRS,		/* Read all 16 of the general purpose 
				   registers */
    DBG_WRITE_GPR,		/* Write one of the general purpose registers
				   d0-d7 or a0-a7 */
    DBG_CONTINUE, 		/* Continue execution */
    DBG_SINGLESTEP,		/* Single step execution */
    DBG_DETACH,			/* The debugger has finished with the kernel */
    DBG_INST_READ,		/* Read an instruction */
    DBG_INST_WRITE,		/* Write an instruction */
    DBG_DATA_READ,		/* Read data */
    DBG_DATA_WRITE,		/* Write data */
    DBG_SET_PID,		/* Set the process for which the stack 
				 * back trace is to be done. */
    DBG_GET_STOP_INFO,		/* Get all info needed by dbx after it stops. */
    DBG_GET_VERSION_STRING,	/* Return the version string. */
    DBG_DIVERT_SYSLOG,		/* Divert syslog output to the console. */
    DBG_REBOOT,			/* Call the reboot routine. */
    DBG_BEGIN_CALL,		/* Start a call. */
    DBG_END_CALL, 		/* Clean up after a call completes. */
    DBG_UNKNOWN			/* Used for error checking */
} Dbg_Opcode;

typedef struct {
    int	regNum;
    int	regVal;
} Dbg_WriteGPR;

typedef struct {
    int		address;
    int		numBytes;
    char	buffer[100];
} Dbg_WriteMem;

typedef struct {
    int		address;
    int		numBytes;
} Dbg_ReadMem;

typedef struct {
    int		stringLength;
    char	string[100];
} Dbg_Reboot;

typedef enum {
    DBG_SYSLOG_TO_ORIG,
    DBG_SYSLOG_TO_CONSOLE,
} Dbg_SyslogCmd;

/*
 * Message format.
 */
typedef struct {
    short	opcode;
    union {
	int		pid;
	Dbg_WriteGPR	writeGPR;
	Dbg_WriteMem	writeMem;
	Dbg_ReadMem	readMem;
	int		pc;
	Dbg_SyslogCmd	syslogCmd;
	Dbg_Reboot	reboot;
    } data;
} Dbg_Msg;

#define	DBG_MAX_REPLY_SIZE	1024
#define	DBG_MAX_REQUEST_SIZE	1024

/*
 * The UDP port number that the kernel and kdbx use to identify a packet as
 * a debugging packet.  (composed from "uc": 0x75 = u, 0x63 = c)
 */

#define DBG_UDP_PORT 	0x7563

/*
 * The different statuses that we send kdbx after we stop.  There is one
 * status for each exception.  These numbers matter because kdbx uses them to
 * index an array.  Therefore don't change any of them without also changing
 * kdbx's array in ../kdbx/machine.c.
 *
 *     DBG_INTERRUPT		No error just an interrupt from the console.
 *     DBG_RESET		System reset.
 *     DBG_BUS_ERROR		Bus error.
 *     DBG_ADDRESS_ERROR	Address error.
 *     DBG_ILLEGAL_INST		Illegal instruction.
 *     DBG_ZERO_DIV		Division by zero.
 *     DBG_CHK_INST		A CHK instruction failed.
 *     DBG_TRAPV		Overflow trap.
 *     DBG_PRIV_VIOLATION	Privledge violation.
 *     DBG_TRACE_TRAP		Trace trap.
 *     DBG_EMU1010		Emulator 1010 trap.
 *     DBG_EMU1111		Emulator 1111 trap.
 *     DBG_STACK_FMT_ERROR	Stack format error.
 *     DBG_UNINIT_VECTOR	Unitiailized vector error.
 *     DBG_SPURIOUS_INT		Spurious interrupt.
 *     DBG_LEVEL1_INT		Level 1 interrupt.
 *     DBG_LEVEL2_INT		Level 2 interrupt.
 *     DBG_LEVEL3_INT		Level 3 interrupt.
 *     DBG_LEVEL4_INT		Level 4 interrupt.
 *     DBG_LEVEL5_INT		Level 5 interrupt.
 *     DBG_LEVEL6_INT		Level 6 interrupt.
 *     DBG_LEVEL7_INT		Level 7 interrupt.
 *     DBG_SYSCALL_TRAP		A system call trap.
 *     DBG_SIG_RET_TRAP		A return from signal trap.
 *     DBG_BAD_TRAP		Bad trap.
 *     DBG_BRKPT_TRAP		Breakpoint trap.
 *     DBG_UKNOWN_EXC		Unknown exception.
 */

#define DBG_INTERRUPT		0
#define	DBG_RESET		1
#define	DBG_BUS_ERROR		2
#define	DBG_ADDRESS_ERROR	3
#define	DBG_ILLEGAL_INST	4
#define	DBG_ZERO_DIV		5
#define	DBG_CHK_INST		6
#define	DBG_TRAPV		7
#define	DBG_PRIV_VIOLATION	8
#define	DBG_TRACE_TRAP		9
#define	DBG_EMU1010		10
#define	DBG_EMU1111		11
#define	DBG_STACK_FMT_ERROR	12
#define	DBG_UNINIT_VECTOR	13
#define	DBG_SPURIOUS_INT	14
#define	DBG_LEVEL1_INT		15
#define	DBG_LEVEL2_INT		16
#define	DBG_LEVEL3_INT		17
#define	DBG_LEVEL4_INT		18
#define	DBG_LEVEL5_INT		19
#define	DBG_LEVEL6_INT		20
#define	DBG_LEVEL7_INT		21
#define	DBG_SYSCALL_TRAP	22
#define	DBG_SIG_RET_TRAP	23
#define	DBG_BAD_TRAP		24
#define	DBG_BRKPT_TRAP		25
#define	DBG_UNKNOWN_EXC		26

/*
 * Variable that is set to true when we are called through the DBG_CALL macro.
 */
extern	Boolean	dbgPanic;

/* 
 * Macro to call the debugger from kernel code.
 */
#define DBG_CALL	dbgPanic = TRUE; asm("trap #15");

/*
 * Number of bytes between acknowledgements when the the kernel is writing
 * to kdbx.
 */
#define DBG_ACK_SIZE	256

/*
 * Info returned when GETSTOPINFO command is submitted.
 */
typedef struct {
    int			codeStart;
    int			maxStackAddr;
    int			termReason;
    int			trapCode;
    unsigned	int	statusReg;
    int			genRegs[16];
    int			pc;
} StopInfo;

extern	void	Dbg_Init();
extern	void	Dbg_InputPacket();

#endif _DBG
