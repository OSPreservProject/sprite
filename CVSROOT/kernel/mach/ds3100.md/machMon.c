/* 
 * machMon.c --
 *
 *	Routines to access the PMAX prom monitor.
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
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "machMon.h"
#include "machConst.h"
#include "machInt.h"
#include "ctype.h"
#include "mach.h"

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


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonPutChar --
 *
 *     Call the monitor put character routine
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
int
Mach_MonPutChar(ch)
    int		ch;
{
    return(Dev_GraphicsPutc(ch));
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonAbort --
 *
 *     	Abort to the monitor.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_MonAbort()
{
    mach_MonFuncs.restart();
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonReboot --
 *
 *     	Reboot the system.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     System rebooted.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_MonReboot(rebootString)
    char	*rebootString;
{
    char *bootpath;
    if (*rebootString != '\0') {
	mach_MonFuncs.setenv2("bootpath",rebootString);
    } else {
	bootpath = mach_MonFuncs.getenv2("bootpath");
	if (bootpath == (char *)NULL || *bootpath == '\0') {
	    /*
	     * Hardware doesn't have a bootpath.
	     */
	    mach_MonFuncs.setenv2("bootpath",DEFAULT_REBOOT);
	    printf("Using default %s\n",DEFAULT_REBOOT);
	}
	/*
	 * Otherwise use hardware's bootpath.
	 */
    }
    *MACH_USE_NON_VOLATILE |= MACH_NON_VOLATILE_FLAG;
    mach_MonFuncs.autoboot();
    panic("Mach_MonReboot: Reboot failed (I'm still alive aren't I?)\n");
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_ArgParse --
 *
 *	Parse a string into ds3100 string table form.
 *
 * Results:
 *     Returns argc.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
int
Mach_ArgParse(string,table)
char *string;
MachStringTable *table;
{
    mach_MonFuncs.argparse(string,table);
}
