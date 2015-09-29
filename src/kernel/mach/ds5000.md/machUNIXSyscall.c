/* 
 * machUNIXSyscall.c --
 *
 * This file calls the Unix Compatibility stubs.  Normally, the assembly
 * version of this file will get used, not this file.
 *	
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/mach/ds5000.md/machUNIXSyscall.c,v 9.15 92/06/16 17:08:45 shirriff Exp $ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "status.h"
#include "machConst.h"
#include "machInt.h"
#include "mach.h"
#include "proc.h"
#include "vm.h"
#include "user/sys/types.h"
#include "user/sys/file.h"
#include "user/fs.h"
#include "user/sys/wait.h"
#include "user/sys/time.h"
#include "user/sys/resource.h"
#include "user/sys/uio.h"
#include "stat.h"
#include "ultrixSignal.h"
#include "sys.h"
#include "fs.h"
#include "fsdm.h"
#include "fslcl.h"	/* Directory format */
#include "sys/types.h"

/*
 * System call entry structure.  Note that this is different than the one
 * in the sys directory.  This should be converted over to that format
 * with the addition of the name field.
 */
typedef struct {
    char		*name;
    int			numArgs;
    ReturnStatus	(*func)();
} SyscallInfo;

/*
 * Values for system call tracing:
 *
 *	0: 	Use the fast assembly language interface and don't trace 
 *		anything.
 *	1: 	Use the slower C interface but no trace printing.
 *	> 1: 	Use the slower C interface and trace system calls.
 */

/*
 * The code to disable migration for processes using the compatibility
 * library is in the slower C interface...
 */
#ifdef CANT_MIGRATE_COMPAT
int machUNIXSyscallTrace = 0;
#else /* CANT_MIGRATE_COMPAT */
int machUNIXSyscallTrace = 0;
#endif /* CANT_MIGRATE_COMPAT */

extern Mach_State	*machCurStatePtr;

#define UNIX_SIG_RETURN		103
#define UNIX_LONG_JUMP_RETURN	139

extern int sysCallNum;


/*
 * ----------------------------------------------------------------------------
 *
 * MachUNIXSyscallNew --
 *
 *	Try to handle the new given UNIX system call.
 *	Note that this routine is called only if (machUNIXSyscallTrace > 0)
 *	or under special conditions.
 *
 * Results:
 *	TRUE if this was really a UNIX system call.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
Boolean
MachUNIXSyscallNew()
{
    unsigned		*regs;
    int			args[6];
    ReturnStatus	status;
    int			type;
    int			numArgs;
#ifdef CANT_MIGRATE_COMPAT
    Proc_ControlBlock	*procPtr;
#endif /* CANT_MIGRATE_COMPAT */

    /*
     * See if we got a UNIX system call.  Unix passes the system call type
     * in v0.
     */
    type = machCurStatePtr->userState.regState.regs[V0];
    sysCallNum = type;
    machCurStatePtr->userState.savedV0 = type;
    machCurStatePtr->userState.savedA3 =
	    machCurStatePtr->userState.regState.regs[A3];
    if (type < 0 || type >= MACH_MAX_UNIX_SYSCALL) {
	printf("MachUNIXSyscallNew failed with type %d\n", type);
	return(FALSE);
    }

#ifdef CANT_MIGRATE_COMPAT
    /*
     * We don't want to migrate processes that are making unix-compatible calls.
     */
    procPtr = Proc_GetCurrentProc();
    if (!(procPtr->genFlags & PROC_DONT_MIGRATE)) {
	Proc_NeverMigrate(procPtr);
    }
#endif /* CANT_MIGRATE_COMPAT */
    
    numArgs = sysUnixSysCallTable[type].numArgs;
    regs = machCurStatePtr->userState.regState.regs;
    if (numArgs > 4) {
	if (Vm_CopyIn(4 * (numArgs - 4), regs[SP] + 16, args) != SUCCESS) {
	    regs[V0] = Compat_MapCode(SYS_ARG_NOACCESS);
	    regs[A3] = 1;
	    return(TRUE);
	}
    }
    switch (numArgs) {
	case 0:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d() => ", type);
	    }
	    status = sysUnixSysCallTable[type].func();
	    break;
	case 1:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x) => ", 
			type,
			regs[A0]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0]);
	    break;
	case 2:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x) => ", 
			type,
			regs[A0], regs[A1]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1]);
	    break;
	case 3:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x, %x) => ", 
			type,
			regs[A0], regs[A1], regs[A2]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2]);
	    break;
	case 4:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x, %x, %x) => ", 
			type,
			regs[A0], regs[A1], regs[A2], regs[A3]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3]);
	    break;
	case 5: 
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x, %x, %x, %x) => ", 
			type,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0]);
	    break; 
	case 6:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x, %x, %x, %x, %x) => ", 
			type,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0],
					     args[1]);
	    break; 
	case 7:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x, %x, %x, %x, %x, %x) => ", 
			type,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1], args[2]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1],
					     regs[A2], regs[A3], args[0],
					     args[1], args[2]);
	    break;
	case 8:
	    if (machUNIXSyscallTrace > 1) {
		printf("MachUNIXSyscallNew: syscall%d(%x, %x, %x, %x, %x, %x, %x, %x) => ", 
			type,
			regs[A0], regs[A1], regs[A2], regs[A3], args[0],
			args[1], args[2], args[3]);
	    }
	    status = sysUnixSysCallTable[type].func(regs[A0], regs[A1], 
					     regs[A2], regs[A3], args[0],
					     args[1], args[2], args[3]);
	default:
	    panic("Too many args to UNIX system call\n");
	    break;
    }
    /*
     * The UNIX stubs looks at a3 to decide what to do.  If a3 == 1 then
     * v0 has the UNIX error code and the stub will stuff the error
     * into errno.  If a3 == 0 the V0 has the return value.
     */
    if (type != MACH_UNIX_LONG_JUMP_RETURN && type != MACH_UNIX_SIG_RETURN) {
	if (status >= 0) {
	    regs[V0] = status;
	    regs[A3] = 0;
	} else {
	    regs[V0] = Proc_GetActualProc()->unixErrno;
	    regs[A3] = 1;
	}
    }
    if (machUNIXSyscallTrace > 1) {
	printf("V0 = %x A3 = %x\n", regs[V0], regs[A3]);
    }

    machCurStatePtr->userState.regState.pc += 4;

    return(TRUE);
}
