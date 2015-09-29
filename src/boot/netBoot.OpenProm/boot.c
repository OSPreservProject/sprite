/*-
 * boot.c --
 *	Some PROM version specific functions which were too
 *	complicated to be macros.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /sprite/src/boot/netBoot.OpenProm/RCS/boot.c,v 1.2 91/01/13 02:26:28 dlong Exp $ SPRITE (Berkeley)";
#endif lint

#include "boot.h"
#include <string.h>
#include <ctype.h>

#define min(x,y)	((x) < (y) ? (x) : (y))


/*
 *----------------------------------------------------------------------
 *
 * PrintBootCommand --
 *
 *	Print out the boot command, as expanded by PROM defaults.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
PrintBootCommand()
{
    int i;

    if (RomVersion >= 2) {
	printf("%s %s\n", *romp->bootpath, *romp->bootargs);
    } else if (romp->v_bootparam) {
	i = 0;
	while ((*romp->v_bootparam)->bp_argv[i]) {
	    printf("%s ", (*romp->v_bootparam)->bp_argv[i]);
	    ++i;
	}
	printf("\n");
    } else {
	printf("(NULL)\n");
    }

}


/*
 *----------------------------------------------------------------------
 *
 * BootDevName --
 *
 *	Get boot device name from PROM data structures.
 *
 * Results:
 *	Pointer to static character buffer, or string in PROM.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
BootDevName()
{
    static char buf[64];
    char *p, *q;
    int n;

    if (RomVersion >= 2) {
	/*
	 * With ROM version 2, bootpath contains the path of the
	 * boot device.  The dev(ctrl,unit,part) style is no
	 * longer fully supported.  v_bootparam is not filled
	 * in.
	 */
	return *romp->bootpath;
    } else {
	/*
	 * With older ROM versions, v_bootparam is filled in,
	 * and bp_argv[0] contains the name of the boot device.
	 * It looks like: dev(ctrl,unit,part)filename.
	 * The filename may not be stripped off the end, so
	 * do it here, even though v_open doesn't care.
	 */
	p = (*romp->v_bootparam)->bp_argv[0];
	q = strchr(p, ')');
	if (q) {
	    n = min(q - p + 1, sizeof buf - 1);
	    strncpy(buf, p, n);
	    return buf;
	} else {
	    return p;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * BootFileName --
 *
 *	Get boot file name from PROM data structures.
 *
 * Results:
 *	Pointer to static character buffer, or string in PROM.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
BootFileName()
{
    static char buf[64];
    char *p, *q;

    if (RomVersion >= 2) {
	/*
	 * bootargs contains the filename and arguments.
	 * Just copy the filename here; ignore arguments.
	 */
	p = buf;
	q = *romp->bootargs;
	while (*q && !isspace(*q)) {
	    *p++ = *q++;
	}
	*p = '\0';
	return buf;
    } else {
	/*
	 * bp_name contains the filename without arguments.
	 */
	return ((*romp->v_bootparam)->bp_name);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Routine --
 *
 *	Print out panic strings, and return to monitor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Should not return.  Aborts to the monitor.
 *
 *----------------------------------------------------------------------
 */

void panic(string)
    char *string;
{
    printf("Panic: %s\n", string);
    /*
     * This could be (*romp->v_exit_to_mon)() I guess.
     * What is the difference?  This one is what L1-a does,
     * I think.
     */
    (*romp->v_abortent)();
}
