/*
 * dbg.h --
 *
 *	Exported types and procedure headers for the debugger module.
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

#ifndef _DBG
#define _DBG

#include <sprite.h>
#ifdef KERNEL
#include <machTypes.h>
#else
#include <kernel/machTypes.h>
#endif

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
 * Variable that indicates that we are in the debugger command loop.
 */
extern	Boolean	dbg_InDebugger;

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

#define    DBG_READ_ALL_REGS	1	/* Read all the  registers */
#define    DBG_WRITE_REG	2	/* Write one a register */
#define    DBG_CONTINUE		3	/* Continue execution */
#define    DBG_SINGLESTEP	4	/* Single step execution */
#define    DBG_DETACH		5	/* Detach from the debugger */
#define    DBG_INST_READ	6	/* Read an instruction */
#define    DBG_INST_WRITE	7	/* Write an instruction */
#define    DBG_DATA_READ	8	/* Read data */
#define    DBG_DATA_WRITE	9	/* Write data */
#define    DBG_SET_PID		10	/* Set the process for which the stack
					 * back trace is to be done. */
#define    DBG_GET_STOP_INFO	11	/* Get all info needed by dbx after 
					 *it stops. */
#define    DBG_GET_VERSION_STRING 12	/* Return the version string. */
#define    DBG_DIVERT_SYSLOG	13	/* Divert syslog to the console. */
#define    DBG_REBOOT		14	/* Call the reboot routine. */
#define    DBG_BEGIN_CALL	15	/* Start a call. */
#define    DBG_END_CALL		16	/* Clean up after a call completes. */
#define    DBG_CALL_FUNCTION	17	/* Call a function. */
#define    DBG_GET_DUMP_BOUNDS	18	/* Get bounds for the dump program. */
#define    DBG_UNKNOWN		19	/* Used for error checking */

typedef int Dbg_Opcode;

#define	DBG_OPCODE_NAMES {					\
	"UNUSED",						\
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
}								

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

#define	DBG_EXCEPTION_NAMES {		\
    "Interrupt",			\
    "TLB Mod",				\
    "TLB LD miss",			\
    "TLB ST miss",			\
    "TLB load address error",		\
    "TLB store address error",		\
    "TLB ifetch bus error",		\
    "TLB load or store bus error",	\
    "System call",			\
    "Breakpoint trap",			\
    "Reserved instruction",		\
    "Coprocessor unusable",		\
    "Overflow"				\
}					

/*
 * Variable that is set to true when we are called through the DBG_CALL macro.
 */
extern	Boolean	dbgPanic;

/*
 * Macro to call the debugger from kernel code.
 */
extern	void Dbg_Call _ARGS_((void));
#define DBG_CALL	dbgPanic = TRUE; Dbg_Call();

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
extern	void	Dbg_InputPacket _ARGS_((Address packetPtr, int packetLength));
extern	Boolean	Dbg_InRange _ARGS_((unsigned int addr, int numBytes,
				    Boolean writeable));
extern	unsigned	Dbg_Main _ARGS_((void));

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
