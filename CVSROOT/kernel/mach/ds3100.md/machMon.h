/*
 * machMon.h --
 *
 *	Structures, constants and defines for access to the pmax prom.
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

#ifndef _MACHMON
#define _MACHMON

/*
 * The prom has a jump table at the beginning of it to get to its
 * functions.
 */
#define MACH_MON_JUMP_TABLE_ADDR	0xBFC00000

/*
 * The jump table.
 */
typedef struct Mach_MonFuncs {
    int		(*reset)();
    int		(*exec)();
    int		(*restart)();
    int		(*reinit)();
    int		(*reboot)();
    int		(*autoboot)();
    int		(*open)();
    int		(*read)();
    int		(*write)();
    int		(*ioctl)();
    int		(*close)();
    int		(*lseek)();
    int		(*getchar)();
    int		(*putchar)();
    int		(*showchar)();
    int		(*gets)();
    int		(*puts)();
    int		(*printf)();
} Mach_MonFuncs;
extern	Mach_MonFuncs	mach_MonFuncs;

/*
 * Each entry in the jump table is 8 bytes - 4 for the jump and 4 for a nop.
 */
#define MACH_MON_FUNC_ADDR(funcNum)	(MACH_MON_JUMP_TABLE_ADDR+((funcNum)*8))

/*
 * The functions:
 *
 *	MACH_MON_RESET		Run diags, check bootmode, reinit.
 *	MACH_MON_EXEC		Load new program image.
 *	MACH_MON_RESTART	Re-enter monitor command loop.
 *	MACH_MON_REINIT		Re-init monitor, then cmd loop.
 *	MACH_MON_REBOOT		Check bootmode, no config.
 *	MACH_MON_AUTOBOOT	Autoboot the system.
 *
 * The following routines access PROM saio routines and may be used by
 * standalone programs that would like to use PROM I/O:
 *
 *	MACH_MON_OPEN		Open a file.
 *	MACH_MON_READ		Read from a file.
 *	MACH_MON_WRITE		Write to a file.
 *	MACH_MON_IOCTL		Iocontrol on a file.
 *	MACH_MON_CLOSE		Close a file.
 *	MACH_MON_LSEEK		Seek on a file.
 *	MACH_MON_GETCHAR	Get character from console.
 *	MACH_MON_PUTCHAR	Put character on console.
 *	MACH_MON_SHOWCHAR	Show a char visibly.
 *	MACH_MON_GETS		gets with editing.
 *	MACH_MON_PUTS		Put string to console.
 *	MACH_MON_PRINTF		Kernel style printf to console.
 */
#define MACH_MON_RESET		MACH_MON_FUNC_ADDR(0)
#define MACH_MON_EXEC		MACH_MON_FUNC_ADDR(1)
#define MACH_MON_RESTART	MACH_MON_FUNC_ADDR(2)
#define MACH_MON_REINIT		MACH_MON_FUNC_ADDR(3)
#define MACH_MON_REBOOT		MACH_MON_FUNC_ADDR(4)
#define MACH_MON_AUTOBOOT	MACH_MON_FUNC_ADDR(5)
#define MACH_MON_OPEN		MACH_MON_FUNC_ADDR(6)
#define MACH_MON_READ		MACH_MON_FUNC_ADDR(7)
#define MACH_MON_WRITE		MACH_MON_FUNC_ADDR(8)
#define MACH_MON_IOCTL		MACH_MON_FUNC_ADDR(9)
#define MACH_MON_CLOSE		MACH_MON_FUNC_ADDR(10)
#define MACH_MON_LSEEK		MACH_MON_FUNC_ADDR(11)
#define MACH_MON_GETCHAR	MACH_MON_FUNC_ADDR(12)
#define MACH_MON_PUTCHAR	MACH_MON_FUNC_ADDR(13)
#define MACH_MON_SHOWCHAR	MACH_MON_FUNC_ADDR(14)
#define MACH_MON_GETS		MACH_MON_FUNC_ADDR(15)
#define MACH_MON_PUTS		MACH_MON_FUNC_ADDR(16)
#define MACH_MON_PRINTF		MACH_MON_FUNC_ADDR(17)

/*
 * Functions and defines to access the monitor.
 */

#define Mach_MonPrintf (mach_MonFuncs.printf)
extern	int 	Mach_MonPutChar ();
#define Mach_MonMayPut Mach_MonPutChar
extern	void	Mach_MonAbort();
extern	void	Mach_MonReboot();

#define Mach_MonGetChar (mach_MonFuncs.getchar)
#define Mach_MonGetNextChar (mach_MonFuncs.getchar)
#define Mach_MonGetLine (mach_MonFuncs.gets)

#endif /* _MACHPROM */
