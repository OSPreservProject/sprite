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
 * The prom routines use the following structure to hold strings.
 */
typedef struct {
	char	*argPtr[16];	/* Pointers to the strings. */
	char	strings[256];		/* Buffer for the strings. */
	char	*end;		/* Pointer to end of used buf. */
	int 	num;		/* Number of strings used. */
} MachStringTable;

MachStringTable	MachMonBootParam;	/* Boot command line. */

/*
 * The prom has a jump table at the beginning of it to get to its
 * functions.
 */
#define MACH_MON_JUMP_TABLE_ADDR	0xBFC00000

/*
 * Default reboot string.
 */
#define DEFAULT_REBOOT	"tftp()ds3100"

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
    int		(*mgetchar)();
    int		(*mputchar)();
    int		(*showchar)();
    int		(*gets)();
    int		(*puts)();
    int		(*printf)();
    int		(*mem1)();
    int		(*mem2)();
    int		(*save_regs)();
    int		(*load_regs)();
    int		(*jump_s8)();
    char *	(*getenv2)();
    int		(*setenv2)();
    int		(*atonum)();
    int		(*strcmp)();
    int		(*strlen)();
    char *	(*strcpy)();
    char *	(*strcat)();
    int		(*get_cmd)();
    int		(*get_nums)();
    int 	(*argparse)();
    int		(*help)();
    int		(*dump)();
    int		(*setenv)();
    int		(*unsetenv)();
    int		(*printenv)();
    int		(*jump2_s8)();
    int		(*enable)();
    int		(*disable)();
    int		(*zero_buf)();
} Mach_MonFuncs;

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
 *
 * The following are other prom routines:
 *	MACH_MON_MEM1		Do something in memory.
 *	MACH_MON_MEM2		Do something else in memory.
 *	MACH_MON_SAVEREGS	Save registers in a buffer.
 *	MACH_MON_LOADREGS	Get register back from buffer.
 *	MACH_MON_JUMPS8		Jump to address in s8.
 *	MACH_MON_GETENV2	Gets a string from system environment.
 *	MACH_MON_SETENV2	Sets a string in system environment.
 *	MACH_MON_ATONUM		Converts ascii string to number.
 *	MACH_MON_STRCMP		Compares strings (strcmp).
 *	MACH_MON_STRLEN		Length of string (strlen).
 *	MACH_MON_STRCPY		Copies string (strcpy).
 *	MACH_MON_STRCAT		Appends string (strcat).
 *	MACH_MON_GETCMD		Gets a command.
 *	MACH_MON_GETNUMS	Gets numbers.
 *	MACH_MON_ARGPARSE	Parses string to argc,argv.
 *	MACH_MON_HELP		Help on prom commands.
 *	MACH_MON_DUMP		Dumps memory.
 *	MACH_MON_SETENV		Sets a string in system environment.
 *	MACH_MON_UNSETENV	Unsets a string in system environment
 *	MACH_MON_PRINTENV	Prints system environment
 *	MACH_MON_JUMP2S8	Jumps to s8
 *	MACH_MON_ENABLE		Performs prom enable command.
 *	MACH_MON_DISABLE	Performs prom disable command.
 *	MACH_MON_ZEROB		Zeros a system buffer.
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
#define MACH_MON_MEM1		MACH_MON_FUNC_ADDR(28)
#define MACH_MON_MEM2		MACH_MON_FUNC_ADDR(29)
#define MACH_MON_SAVEREGS	MACH_MON_FUNC_ADDR(30)
#define MACH_MON_LOADREGS	MACH_MON_FUNC_ADDR(31)
#define MACH_MON_JUMPS8		MACH_MON_FUNC_ADDR(32)
#define MACH_MON_GETENV2	MACH_MON_FUNC_ADDR(33)
#define MACH_MON_SETENV2	MACH_MON_FUNC_ADDR(34)
#define MACH_MON_ATONUM		MACH_MON_FUNC_ADDR(35)
#define MACH_MON_STRCMP		MACH_MON_FUNC_ADDR(36)
#define MACH_MON_STRLEN		MACH_MON_FUNC_ADDR(37)
#define MACH_MON_STRCPY		MACH_MON_FUNC_ADDR(38)
#define MACH_MON_STRCAT		MACH_MON_FUNC_ADDR(39)
#define MACH_MON_GETCMD		MACH_MON_FUNC_ADDR(40)
#define MACH_MON_GETNUMS	MACH_MON_FUNC_ADDR(41)
#define MACH_MON_ARGPARSE	MACH_MON_FUNC_ADDR(42)
#define MACH_MON_HELP		MACH_MON_FUNC_ADDR(43)
#define MACH_MON_DUMP		MACH_MON_FUNC_ADDR(44)
#define MACH_MON_SETENV		MACH_MON_FUNC_ADDR(45)
#define MACH_MON_UNSETENV	MACH_MON_FUNC_ADDR(46)
#define MACH_MON_PRINTENV	MACH_MON_FUNC_ADDR(47)
#define MACH_MON_JUMP2S8	MACH_MON_FUNC_ADDR(48)
#define MACH_MON_ENABLE		MACH_MON_FUNC_ADDR(49)
#define MACH_MON_DISABLE	MACH_MON_FUNC_ADDR(50)
#define MACH_MON_ZEROB		MACH_MON_FUNC_ADDR(51)

#ifdef _MONFUNCS
Mach_MonFuncs mach_MonFuncs = {
    (int (*)()) MACH_MON_RESET,
    (int (*)()) MACH_MON_EXEC,
    (int (*)()) MACH_MON_RESTART,
    (int (*)()) MACH_MON_REINIT,
    (int (*)()) MACH_MON_REBOOT,
    (int (*)()) MACH_MON_AUTOBOOT,
    (int (*)()) MACH_MON_OPEN,
    (int (*)()) MACH_MON_READ,
    (int (*)()) MACH_MON_WRITE,
    (int (*)()) MACH_MON_IOCTL,
    (int (*)()) MACH_MON_CLOSE,
    (int (*)()) MACH_MON_LSEEK,
    (int (*)()) MACH_MON_GETCHAR,
    (int (*)()) MACH_MON_PUTCHAR,
    (int (*)()) MACH_MON_SHOWCHAR,
    (int (*)()) MACH_MON_GETS,
    (int (*)()) MACH_MON_PUTS,
    (int (*)()) MACH_MON_PRINTF,
    (int (*)()) MACH_MON_MEM1,
    (int (*)()) MACH_MON_MEM2,
    (int (*)()) MACH_MON_SAVEREGS,
    (int (*)()) MACH_MON_LOADREGS,
    (int (*)()) MACH_MON_JUMPS8,
    (char *(*)()) MACH_MON_GETENV2,
    (int (*)()) MACH_MON_SETENV2,
    (int (*)()) MACH_MON_ATONUM,
    (int (*)()) MACH_MON_STRCMP,
    (int (*)()) MACH_MON_STRLEN,
    (char *(*)()) MACH_MON_STRCPY,
    (char *(*)()) MACH_MON_STRCAT,
    (int (*)()) MACH_MON_GETCMD,
    (int (*)()) MACH_MON_GETNUMS,
    (int (*)()) MACH_MON_ARGPARSE,
    (int (*)()) MACH_MON_HELP,
    (int (*)()) MACH_MON_DUMP,
    (int (*)()) MACH_MON_SETENV,
    (int (*)()) MACH_MON_UNSETENV,
    (int (*)()) MACH_MON_PRINTENV,
    (int (*)()) MACH_MON_JUMP2S8,
    (int (*)()) MACH_MON_ENABLE,
    (int (*)()) MACH_MON_DISABLE,
    (int (*)()) MACH_MON_ZEROB,
};
#else
extern	Mach_MonFuncs	mach_MonFuncs;
#endif

/*
 * Functions and defines to access the monitor.
 */

extern void Mach_MonAbort _ARGS_((void));
extern int Mach_MonPutChar _ARGS_((int ch));
extern void Mach_MonReboot _ARGS_((char *rebootString));
#define Mach_MonMayPut Mach_MonPutChar


#define Mach_MonGetChar			(mach_MonFuncs.getchar)
#define Mach_MonGetNextChar		(mach_MonFuncs.getchar)
#define Mach_MonGetLine			(mach_MonFuncs.gets)
#define Mach_ArgParse(string,table)	(mach_MonFuncs.argparse)(string,table)
#define Mach_MonPrintf			(mach_MonFuncs.printf)
#define Mach_MonOpen(name,flags)	(mach_MonFuncs.open)(name,flags)
#define Mach_MonRead(fd,buf,len)	(mach_MonFuncs.read)(fd,buf,len)
#define Mach_MonClose(fd)		(mach_MonFuncs.close)(fd)
#define Mach_MonLseek(fd,offset,mode)	(mach_MonFuncs.lseek)(fd,offset,mode)

/*
 * The nonvolatile ram has a flag to indicate it is usable.
 */
#define MACH_USE_NON_VOLATILE 	((char *)0xbd0000c0)
#define MACH_NON_VOLATILE_FLAG	0x02

#endif /* _MACHPROM */
