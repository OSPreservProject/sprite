/*
 * dbg.h --
 *
 *     Exported types and procedure headers for the debugger module.
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
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBG
#define _DBG

#ifndef _SPRITE
#include <sprite.h>
#endif

#ifdef KERNEL
#include <mach.h>
#include <user/netInet.h>
#include <netTypes.h>
#else
#include <kernel/mach.h>
#include <netInet.h>
#endif

/*
 * Variable to indicate that dbg wants a packet.
 */
extern	Boolean	dbg_UsingNetwork;

/*
 * Variable to indicate if are using the rs232 debugger or the network debugger.
 * On the sun4, we have only used the network.
 */
extern  Boolean dbg_Rs232Debug;

/*
 * Variable that indicates that we are under control of the debugger.
 */
extern	Boolean	dbg_BeingDebugged;

/*
 * Debugger using syslog to dump output of call command or not.
 */
extern	Boolean	dbg_UsingSyslog;

/*
 * The different opcodes that kdbx can send us.
 */

typedef enum {
    DBG_READ_ALL_REGS,		/* Read all the  registers */
    DBG_WRITE_REG,		/* Write one of the registers d0-d7 or a0-a7 */
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
    DBG_CALL_FUNCTION,		/* Call a function. */
    DBG_GET_DUMP_BOUNDS,	/* Get the bounds for the dump program. */
    DBG_UNKNOWN			/* Used for error checking */
} Dbg_Opcode;

#define	DBG_OPCODE_NAMES {					\
	"Read all regs",					\
	"Write reg",						\
	"Continue",						\
	"Single Step",						\
	"Detach",						\
	"Inst Read",						\
	"Inst Write",						\
	"Data Read",						\
	"Data Write",						\
	"Process to walk stack for",				\
	"Read information after stopped",			\
	"Return version string",				\
	"Divert syslog to the console",				\
	"Reboot the machine",					\
	"Set up things to start a call command",		\
	"Clean up things after a call command has executed",	\
	"Call a function",					\
	"Get bounds for the dump program",			\
	"UNKNOWN OPCODE"					\
}								\


typedef struct {
    int	regNum;
    int	regVal;
} Dbg_WriteReg;

typedef struct {
    int		address;
    int		numBytes;
    char	buffer[100];
} Dbg_WriteMem;

typedef Dbg_WriteMem Dbg_CallFunc;

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

typedef struct {
    unsigned int	pageSize;
    unsigned int	stackSize;
    unsigned int	kernelCodeStart;
    unsigned int	kernelCodeSize;
    unsigned int	kernelDataStart;
    unsigned int	kernelDataSize;
    unsigned int	kernelStacksStart;
    unsigned int	kernelStacksSize;
    unsigned int	fileCacheStart;
    unsigned int	fileCacheSize;
} Dbg_DumpBounds;

/*
 * Message format.
 */
typedef struct {
    int		opcode;
    union {
	int		pid;
	Dbg_WriteReg	writeReg;
	Dbg_WriteMem	writeMem;
	Dbg_CallFunc	callFunc;
	Dbg_ReadMem	readMem;
	int		pc;
	Dbg_SyslogCmd	syslogCmd;
	Dbg_Reboot	reboot;
    } data;
} Dbg_Msg;

#define	DBG_MAX_REPLY_SIZE	1400
#define	DBG_MAX_REQUEST_SIZE	1400

/*
 * The UDP port number that the kernel and kdbx use to identify a packet as
 * a debugging packet.  (composed from "uc": 0x75 = u, 0x63 = c)
 */

#define DBG_UDP_PORT 	0x7563

/*
 * The different statuses that we send kgdb after we stop.
 */

#define	DBG_RESET		0
#define	DBG_INSTR_ACCESS	1
#define	DBG_ILLEGAL_INSTR	2
#define	DBG_PRIV_INSTR		3
#define	DBG_FP_DISABLED		4
#define	DBG_WINDOW_OVERFLOW	5
#define	DBG_WINDOW_UNDERFLOW	6
#define	DBG_MEM_ADDR_ALIGN	7
#define	DBG_FP_EXCEP		8
#define	DBG_DATA_ACCESS		9
#define	DBG_TAG_OVERFLOW	10
#define	DBG_UNKNOWN_TRAP11	11
#define	DBG_UNKNOWN_TRAP12	12
#define	DBG_UNKNOWN_TRAP13	13
#define	DBG_UNKNOWN_TRAP14	14
#define	DBG_UNKNOWN_TRAP15	15

#define	DBG_INTERRUPT		16
#define	DBG_LEVEL1_INT		17
#define	DBG_LEVEL2_INT		18
#define	DBG_LEVEL3_INT		19
#define	DBG_LEVEL4_INT		20
#define	DBG_LEVEL5_INT		21
#define	DBG_LEVEL6_INT		22
#define	DBG_LEVEL7_INT		23
#define	DBG_LEVEL8_INT		24
#define	DBG_LEVEL9_INT		25
#define	DBG_LEVEL10_INT		26
#define	DBG_LEVEL11_INT		27
#define	DBG_LEVEL12_INT		28
#define	DBG_LEVEL13_INT		29
#define	DBG_LEVEL14_INT		30
#define	DBG_LEVEL15_INT		31

#define	DBG_BREAKPOINT_TRAP	32		/* ta 1 */
#define	DBG_UNKNOWN_TRAP	33		/* Anything other trap. */
#define	DBG_UNKNOWN_EXCEPT	34		/* Anything execption. */

/*
 * Convert a sparc machine trap into a DBG trap.
 */
#define	DBG_CVT_MACH_TRAP(tn)	( ((tn) < 32) ? (tn) : \
	(( (tn) == 129) ? DBG_BREAKPOINT_TRAP : \
		(((tn) > 128 ) ? DBG_UNKNOWN_TRAP : DBG_UNKNOWN_EXCEPT )))




#define	DBG_EXECPTION_NAMES {		\
    "Reset",				\
    "Instruction Fault",		\
    "Illegal Instruction Fault",	\
    "Privilege Instruction Fault",	\
    "FPU Disabled Fault",		\
    "Window Overflow Fault",		\
    "Window Underflow Fault",		\
    "Memory Address Fault",		\
    "FPU Exception Fault",		\
    "Data Fault",			\
    "Tag Overflow Trap",		\
    "Unknown Trap 11",			\
    "Unknown Trap 12",			\
    "Unknown Trap 13",			\
    "Unknown Trap 14",			\
    "Unknown Trap 15",			\
    "Interrupt Trap",			\
    "D[1] Interrupt (level 1)",		\
    "VMEbus 1 Interrupt (level 2)",	\
    "VMEbus 2 Interrupt (level 3)",	\
    "SCSI Interrupt (level 4)",		\
    "VMEbus 3 Interrupt (level 5)",	\
    "Ethernet Interrupt (level 6)",	\
    "VMEbus 4 Interrupt (level 7)",	\
    "Video Interrupt (level 8)",	\
    "VMEbus 5 Interrupt (level 9)",	\
    "Clock Interrupt (level 10)",	\
    "VMEbus 6 Interrupt (level 11)",	\
    "SCCs Interrupt (level 12)",	\
    "VMEbus 7 Interrupt (level 13)",	\
    "Clock Interrupt (level 14)",	\
    "Memory Interrupt (level 15)",	\
    "Breakpoint Trap",			\
    "Unknown Trap",			\
    "UNKNOWN EXCEPTION"			\
}					\

/*
 * Variable that is set to true when we are called through the DBG_CALL macro.
 */
extern	Boolean	dbgPanic;

/*
 * Macro to call the debugger from kernel code.
 */
#define DBG_CALL	dbgPanic = TRUE; asm("ta 1");


/*
 * Info returned when GETSTOPINFO command is submitted.
 */
typedef struct {
    int			codeStart;
    int			trapType;
    Mach_RegState	regs;
} StopInfo;

#ifdef KERNEL
extern	void	Dbg_Init _ARGS_((void));
extern	void	Dbg_InputPacket _ARGS_((Net_Interface *interPtr,
			    Address packetPtr, int packetLength));
extern	Boolean	Dbg_InRange _ARGS_((unsigned int addr, int numBytes,
				    Boolean writeable));
extern	void	Dbg_Main _ARGS_((int trapType, Mach_RegState *trapStatePtr));
extern int Dbg_PacketHdrSize _ARGS_((void));
extern Boolean
    Dbg_ValidatePacket _ARGS_((int size, Net_IPHeader *ipPtr, int *lenPtr,
			       Address *dataPtrPtr,
			       Net_InetAddress *destIPAddrPtr,
			       Net_InetAddress *srcIPAddrPtr,
			       unsigned int *srcPortPtr));
extern void
    Dbg_FormatPacket _ARGS_((Net_InetAddress srcIPAddress,
			     Net_InetAddress destIPAddress,
			     unsigned int destPort, int dataSize,
			     Address dataPtr));
extern int	Dbg_PacketHdrSize _ARGS_((void));
#endif
#endif /* _DBG */
