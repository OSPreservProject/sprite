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

#include "sprite.h"
#include "mach.h"

#define DBG_MAX_REPLY_SIZE	1024
#define DBG_MAX_REPLY_DATA_SIZE 900
#define DBG_MAGIC	0x01020304
#define DBG_MAX_CONTENT 	48
/*
 * These mappings of special registers to register numbers has to correspond
 * to that in the kgdb code.
 */
#define DBG_PS_REGNUM 33		/* Contains processor status */
#define DBG_PC_REGNUM 34		/* Contains program counter */
#define DBG_NEXT_PC_REGNUM 35		/* Contains program counter */
#define DBG_SWP_REGNUM 37   		/* Contains window stack ptr. */
#define DBG_CWP_REGNUM 38		/* Contains current window ptr. */


/*
 * The different opcodes that kdbx can send us.
 */

typedef int Dbg_Opcode;

#define    DBG_READ_ALL_REGS		0	/* Read all the  registers */
#define    DBG_WRITE_REG		1	/* Write one of the registers */
#define    DBG_CONTINUE			2	/* Continue execution */
#define    DBG_SINGLESTEP		3	/* Single step execution */
#define    DBG_DETACH			4	/* The debugger has finished 
						 * with the kernel */
#define    DBG_INST_READ		5	/* Read an instruction */
#define    DBG_INST_WRITE		6	/* Write an instruction */
#define    DBG_DATA_READ		7	/* Read data */
#define    DBG_DATA_WRITE		8	/* Write data */
#define    DBG_SET_PID			9	/* Set the process for which 
						 * the stack back trace is 
						 * to be done. */
#define    DBG_GET_STOP_INFO		10	/* Get all info needed by dbx 
						 * after it stops. */
#define    DBG_GET_VERSION_STRING	11	/* Return the version string. */
#define    DBG_DIVERT_SYSLOG		12	/* Divert syslog output to 
						 * the console. */
#define    DBG_REBOOT			13	/* Call the reboot routine. */
#define   DBG_BEGIN_CALL		14	/* Start a call. */
#define    DBG_END_CALL			15	/* Clean up after a call completes. */
#define    DBG_CALL_FUNCTION		16	/* Call a function. */
#define    DBG_SET_PROCESSOR		17	/* Set the processor number 
						 * for which the trap state 
						 * (pid 0) is shown */
#define    DBG_UNKNOWN			18	/* Used for error checking */

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
	"Processor for which to display trap state",		\
	"UNKNOWN OPCODE"					\
}							

#define DBG_EXCEPTION_NAMES {					\
	"Breakpoint",						\
	"Single step",						\
	"Call debugger",					\
	"Refresh",						\
	"System call",						\
	"Sig return",						\
	"Get window mem",					\
	"User save state",					\
	"User restore state",					\
	"User test and set",					\
	"User cs",						\
	"User bad swp",						\
	"Cmp trap error",					\
	"Fpu error",						\
	"Illegal",						\
	"Fixnum error",						\
	"Overflow error",					\
	"UNUSED EXCEPTION",					\
	"UNUSED EXCEPTION",					\
	"UNUSED EXCEPTION",					\
	"User return trap",					\
	"User fpu exception",					\
	"User illegal",						\
	"User fixnum",						\
	"User overflow",					\
	"UNKNOWN EXCEPTION",					\
}

/* 
 * Macro to call the debugger from kernel code.  Note that trap type
 * 0 is MACH_CALL_DEBUGGER_TRAP and is defined in machConst.h.
 */
#define DBG_CALL asm("cmp_trap always, r0, r0, $2");

/*
 * Number of bytes between acknowledgements when the the kernel is writing
 * to kdbx.
 */
#define DBG_ACK_SIZE	256

#define DBG_REQUEST_ACK ((char) 0xc)

#define DBG_REPLY_ACK ((char) 0xa)

#define DBG_STRINGIFY(string) DBG_STRINGIFY_2(string)
#define DBG_STRINGIFY_2(string) #string
/*
 * The UDP port number that the kernel and kdbx use to identify a packet as
 * a debugging packet.  (composed from "uc": 0x75 = u, 0x63 = c)
 */

#define DBG_UDP_PORT 	0x7563


typedef struct {
    Mach_RegState	regs;
    int			regStateAddr;
    int			codeStart;
    int			trapType;
} Dbg_StopInfo;

#define DBG_STOPINFO_CNTS "{" MACH_REGSTATE_CNTS "w3}"

typedef struct {
    int		regNum;
    int		regVal;
} Dbg_WriteReg;

#define DBG_WR_CNTS "{w2}"

typedef struct {
    int		address;
    int		numBytes;
    char	buffer[100];
} Dbg_WriteMem;

#define DBG_WM_CNTS "{w2b100}"

typedef struct {
    int		address;
    int		numArgs;
    int		args[10];
} Dbg_CallFunc;

#define DBG_CF_CNTS "{w12}"

typedef struct {
    int		address;
    int		numBytes;
} Dbg_ReadMem;

#define DBG_RM_CNTS "{w2}"

typedef struct {
    int		stringLength;
    char	string[100];
} Dbg_Reboot;

#define DBG_RB_CNTS "{wb100}"

typedef enum {
    DBG_SYSLOG_TO_ORIG,
    DBG_SYSLOG_TO_CONSOLE,
} Dbg_SyslogCmd;

#define DBG_SYS_CNTS "{w2}"

typedef struct {
    int		magic;
    int		sequence;
    Dbg_Opcode	opcode;
    int		dataSize;
} Dbg_RequestHeader;

/*
 * Format for a request header.
 */
#define DBG_REQUEST_HDR_CNTS "{w4}"

/*
 * Message format.
 */
typedef struct {
    Dbg_RequestHeader		header;
    union {
	int			pid;
	Dbg_WriteReg		writeReg;
	Dbg_WriteMem		writeMem;
	Dbg_CallFunc		callFunc;
	Dbg_ReadMem		readMem;
	int			pc;
	Dbg_SyslogCmd		syslogCmd;
	Dbg_Reboot		reboot;
	int			processor;
    } data;
} Dbg_Request;

/*
 * Contents of requests.
 */

#define DBG_CREATE_REQ_CNTS(foo) \
    "(" #foo DBG_WR_CNTS DBG_WM_CNTS DBG_CF_CNTS DBG_RM_CNTS DBG_RB_CNTS \
    DBG_SYS_CNTS "w" ")"

#define DBG_REQUEST_CNTS "{" DBG_REQUEST_HDR_CNTS DBG_CREATE_REQ_CNTS(0) "}"

#define DBG_WR_REQ_CNTS 	DBG_CREATE_REQ_CNTS(0)
#define DBG_WM_REQ_CNTS 	DBG_CREATE_REQ_CNTS(1)
#define DBG_CF_REQ_CNTS 	DBG_CREATE_REQ_CNTS(2)
#define DBG_RM_REQ_CNTS 	DBG_CREATE_REQ_CNTS(3)
#define DBG_RB_REQ_CNTS 	DBG_CREATE_REQ_CNTS(4)
#define DBG_SYS_REQ_CNTS 	DBG_CREATE_REQ_CNTS(5)
#define DBG_PID_REQ_CNTS 	DBG_CREATE_REQ_CNTS(6)
#define DBG_DET_REQ_CNTS 	DBG_CREATE_REQ_CNTS(6)
#define DBG_SP_REQ_CNTS		DBG_CREATE_REQ_CNTS(6)

typedef struct {
    int			magic;
    int			sequence;
    int			dataSize;
    ReturnStatus	status;
    char		contents[DBG_MAX_CONTENT];
} Dbg_ReplyHeader;

/*
 * Format for a reply header.
 */
#define DBG_REPLY_HDR_CNTS "{w4b" DBG_STRINGIFY(DBG_MAX_CONTENT) "}"

typedef struct {
    Dbg_ReplyHeader	header;
    union {
	char		bytes[DBG_MAX_REPLY_DATA_SIZE];
	Dbg_Opcode	opcode;
	Dbg_StopInfo	stopInfo;
	Mach_RegState	regs;
	int		returnVal;
    } data;
} Dbg_Reply;

/*
 * Format for a reply. 
 */

#define DBG_CREATE_REP_CNTS(foo) \
    "(" #foo "b%d" "w" DBG_STOPINFO_CNTS MACH_REGSTATE_CNTS "w" ")"

#define DBG_REPLY_CNTS "{" DBG_REPLY_HDR_CNTS DBG_CREATE_REP_CNTS(0) "}"

#define DBG_BYTES_REP_CNTS	DBG_CREATE_REP_CNTS(0)
#define DBG_OPCODE_REP_CNTS	DBG_CREATE_REP_CNTS(1)
#define DBG_STOPINFO_REP_CNTS	DBG_CREATE_REP_CNTS(2)
#define DBG_REGSTATE_REP_CNTS	DBG_CREATE_REP_CNTS(3)
#define DBG_RETVAL_REP_CNTS	DBG_CREATE_REP_CNTS(4)

/*
 * Debugger using syslog to dump output of call command or not.
 */
extern	Boolean	dbg_UsingSyslog;

/*
 * Debugger is using the rs232 to debug.
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

extern	void	Dbg_Init();
extern	void	Dbg_InputPacket();

#endif _DBG
