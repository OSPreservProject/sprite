/*
 * sysSysCallParam.h --
 *
 *	Declarations of constants and types relevant to system call
 *	parameterization.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $SysSysCallParam: proto.h,v 1.4 86/03/20 14:00:21 andrew Exp $ SPRITE (Berkeley)
 */

#ifndef _SYSSYSCALLPARAM
#define _SYSSYSCALLPARAM

/*
 * For each parameter to a system call, there are several possible
 * dispositions:
 *
 *	SYS_PARAM_IN		- Pass the value of the parameter into the
 *				  system call.
 *	SYS_PARAM_OUT		- Copy the value returned by the system
 *				  call back into the location specified
 *				- By the pointer argument.
 *	SYS_PARAM_ACC		- Indicates that the argument is to
 *				  be made accessible.
 *	SYS_PARAM_COPY		- Indicates that the argument is to
 *				  be copied into and/or out of kernel memory.
 *	SYS_PARAM_NIL		- If set in disposition field during RPC,
 *				  indicates that the corresponding pointer
 *				  was NIL.
 *	SYS_PARAM_ARRAY		- Indicates that the argument is a pointer
 *				  to an array of the specified type.  The
 *				  size of the array must be specified as the
 *				  preceding argument.  If multiple arrays
 *				  are specified as successive args, then they
 *				  each are assumed to have the same number
 *				  of arguments.
 */

#define SYS_PARAM_IN		0x0001
#define SYS_PARAM_OUT		0x0002
#define SYS_PARAM_ACC		0x0010
#define SYS_PARAM_COPY		0x0020
#define SYS_PARAM_NIL		0x0100
#define SYS_PARAM_ARRAY		0x1000

/*
 * Define constants to indicate types of arguments.  The value of the
 * argument type may be used to simplify packaging of the arguments
 * (for example, int's do not need to be made accessible).  It is also
 * used as a subscript into an array of sizes corresponding to each type.
 *
 * Most of the types are self-explanatory, with the exception of Time values.
 * Time structures are two words, and they are sometimes passed by value
 * (on the stack) and sometimes by reference.  TIMEPTR is used to indicate
 * a Time that is passed by reference, and TIME1/TIME2 are used for a Time
 * that is passed by value.  TIME1 and TIME2 are just like ints and are
 * treated like two separate arguments in order to reduce the number of special
 * cases in the parameter parsing routines.
 *
 * SYS_PARAM_INT	-	integer
 * SYS_PARAM_CHAR	-	character (for input, will cause problems due
 *				to the number of bytes!)
 * SYS_PARAM_PROC_PID	- 	Proc_PID
 * SYS_PARAM_PROC_RES   -	Proc_ResUsage
 * SYS_PARAM_SYNC_LOCK	-	Sync_Lock
 * SYS_PARAM_FS_ATT	-	Fs_Attributes
 * SYS_PARAM_FS_NAME	-	string containing a path name: call
 *				Fs_MakeNameAccessible.
 * SYS_PARAM_TIMEPTR	-	Time: only used as OUT or INOUT (ie, pointer)
 * SYS_PARAM_TIME1	-	first word of a Time being passed IN
 * SYS_PARAM_TIME2	-	second word of a Time structure.
 * SYS_PARAM_VM_CMD	-	Vm_Command
 * SYS_PARAM_DUMMY	-	A placeholder for cases in which the
 *				number of arguments is different from the
 *				number of words.
 * SYS_PARAM_RANGE1	-	Start of range of values for array subscripts
 * SYS_PARAM_RANGE2	-	End of range for array.
 * SYS_PARAM_PCB	-	Process control block.
 * SYS_PARAM_FS_DEVICE	-	Device specification.
 */

#define SYS_PARAM_INT			0
#define SYS_PARAM_CHAR			1
#define SYS_PARAM_PROC_PID              2
#define SYS_PARAM_PROC_RES              3
#define SYS_PARAM_SYNC_LOCK             4
#define SYS_PARAM_FS_ATT                5
#define SYS_PARAM_FS_NAME               6
#define SYS_PARAM_TIMEPTR               7
#define SYS_PARAM_TIME1                 8
#define SYS_PARAM_TIME2                 9
#define SYS_PARAM_VM_CMD                10
#define SYS_PARAM_DUMMY			11
#define SYS_PARAM_RANGE1		12
#define SYS_PARAM_RANGE2		13
#define SYS_PARAM_PCB			14
#define SYS_PARAM_FS_DEVICE		15
#define SYS_PARAM_PCBARG		16

typedef struct {
    int type;
    int disposition;
} Sys_CallParam;

/*
 * Define a structure for passing system call parameters to a routine
 * regardless of the number of arguments.
 */

typedef struct {
    int argArray[SYS_MAX_ARGS];
} Sys_ArgArray;

/*
 * Array of sizes of system call parameters, declared in sysSysCall.c.
 */

extern int *sys_ParamSizes;

#endif /* _SYSSYSCALLPARAM */
