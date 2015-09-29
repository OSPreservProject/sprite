/*
 * compatMapCode.c --
 *
 * 	Returns the Unix error code corresponding to a Sprite ReturnStatus.
 *
 * Copyright 1986, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/compatMapCode.c,v 1.12 92/03/27 12:27:06 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#ifdef KERNEL
#include "sprite.h"
#include "status.h"
#include "compatInt.h"
#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#include "mem.h"
#else
#include <sprite.h>
#include <status.h>
#include <compatInt.h>
#include <stdio.h>
#include <errno.h>
#endif

#ifdef KERNEL
#define fprintf(fp, fmt, status) printf(fmt, status)
#endif

typedef struct {
    char *name;		/* ignored, but there for reference */
    int  *array;	/* array of integers, one per ReturnStatus */
    int   size;		/* size of array */
} StatusMappings;

/*
 * The tables below map the Sprite ReturnStatus values for each class
 * (gen, proc, etc.) to UNIX errno's.  These tables used to be
 * automatically generated but as part of the transition to the new C
 * library (in 1988), the auto-generator was dropped, leaving this final
 * version.  Eventually, Sprite error codes should go away entirely, leaving
 * only errno's.
 */

/*	/sprite/src/lib/libc/Status/gen.stat	*/
static int genStatusMappings[] = {
0, 0, 4, 13, 0, 22, 60, 1, 2, 4, 7, 11, 13, 14, 17, 22, 27, 28, 34, 77, };

/*	/sprite/src/lib/libc/Status/proc.stat	*/
static int procStatusMappings[] = {
22, 8, 0, 0, 35, 3, 1, 10, 3, 0, 0, 0, 0, 0, 0, 0, 0, };

/*	/sprite/src/lib/libc/Status/sys.stat	*/
static int sysStatusMappings[] = {
22, 22, 0, };

/*	/sprite/src/lib/libc/Status/rpc.stat	*/
static int rpcStatusMappings[] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

/*	/sprite/src/lib/libc/Status/fs.stat	*/
static int fsStatusMappings[] = {
13, 22, 22, 22, 22, 9, 0, 32, 28, 0, 0, 0, 2, 35, 40, 21, 20, 1, 70, 17, 66, 62, 18, 73, 22, 22, 13, 0, 26, 29, 19, 70, 70, };

/*	/sprite/src/lib/libc/Status/vm.stat	*/
static int vmStatusMappings[] = {
0, 0, 0, 0, 0, 12, };

/*	/sprite/src/lib/libc/Status/sig.stat	*/
static int sigStatusMappings[] = {
22, 22, };

/*	/sprite/src/lib/libc/Status/dev.stat	*/
static int devStatusMappings[] = {
5, 6, 60, 5, 5, 5, 19, 22, 5, 5, 19, 5, 5, 5, 16};

/*	/sprite/src/lib/libc/Status/net.stat	*/
static int netStatusMappings[] = {
51, 65, 61, 54, 0, 56, 57, 48, 49, 0, 45, 42, };

static StatusMappings statusMappings[] = {
	{"Gen", 	 genStatusMappings   , 	 20},
	{"Proc", 	 procStatusMappings  , 	 17},
	{"Sys", 	 sysStatusMappings   , 	 3},
	{"Rpc", 	 rpcStatusMappings   , 	 11},
	{"Fs", 	 fsStatusMappings    , 	 33},
	{"Vm", 	 vmStatusMappings    , 	 6},
	{"Sig", 	 sigStatusMappings   , 	 2},
	{"Dev", 	 devStatusMappings   , 	 15},
	{"Net", 	 netStatusMappings   , 	 12},
};
static int maxNumModules = 9;


/*
 *----------------------------------------------------------------------
 *
 * Compat_MapCode --
 *
 *	Given a Sprite ReturnStatus, return the corresponding UNIX
 *	errno value.
 *
 * Results:
 *	The errno value corresponding to status is returned.  If the
 *	mapping didn't work, an error message will be output and
 *	EINVAL will be returned as a default.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Compat_MapCode(status)
    ReturnStatus  status;
{
	int module = STAT_MODULE(status);
	int msg    = STAT_MSGNUM(status);
	int code   = 0;

	if (status < 0) {
	    fprintf(stderr,
		"*** compat: Cannot decode user status value 0x%x\n", status);
	} else if (module >= maxNumModules) {
	    fprintf(stderr,
		"*** compat: Invalid module # in status value 0x%x\n", status);
	} else if (msg >= statusMappings[module].size) {
#ifdef KERNEL
	    printf(
		"*** compat: Invalid message # for %s module: status = 0x%x\n", 
		statusMappings[module].name, status);
#else
	    fprintf(stderr,
		"*** compat: Invalid message # for %s module: status = 0x%x\n", 
		statusMappings[module].name, status);
#endif
	} else {
	    code = statusMappings[module].array[msg];
	}

	/*
	 * No mapping was found. At least return some type of error value.
	 */
	if (code == 0 &&  status != GEN_SUCCESS) {
	    code = 22;  /* EINVAL */
	}
	return(code);
}

/*
 *----------------------------------------------------------------------
 *
 * Compat_MapToSprite --
 *
 *	Given a UNIX errno value, return the corresponding Sprite
 *	ReturnStatus.
 *
 * Results:
 *	The result is the Sprite ReturnStatus corresponding to
 *	unixErrno.  This mapping isn't exact, in that there may
 *	be several Sprite values that map to the same UNIX value,
 *	but this procedure will always return a value that will
 *	map back to unixErrno when passed to Compat_MapCode.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Compat_MapToSprite(unixErrno)
    int unixErrno;		/* A UNIX error number (e.g. EINVAL). */
{
#define NO_STATUS ((ReturnStatus) -1)
    static int first, last;
    static ReturnStatus *table;
    static int initialized = 0;
    int i, j, k;
    ReturnStatus result;

    /*
     * On the first call to this procedure, build a reverse mapping
     * table from the information in statusMappings.  Do it in three
     * steps:  1) scan statusMappings to find out the range of errnos
     * we have to handle;  2) create and initialize the table;  3)
     * scan statusMappings again to generate reverse mappings.
     */

    if (!initialized) {
	initialized = 1;
	first = 100000;
	last = -100000;
	for (i = 0; i < maxNumModules; i++) {
	    for (j = 0; j < statusMappings[i].size; j++) {
		k = statusMappings[i].array[j];
		if (k < first) {
		    first = k;
		}
		if (k > last) {
		    last = k;
		}
	    }
	}

	table = (ReturnStatus *) malloc((unsigned)
		((last + 1 - first) * sizeof(int)));
	for (i = 0; i <= last-first; i++) {
	    table[i] = NO_STATUS;
	}

	for (i = 0; i < maxNumModules; i++) {
	    for (j = 0; j < statusMappings[i].size; j++) {
		k = statusMappings[i].array[j] - first;
		if (table[k] == NO_STATUS) {
		    table[k] = (i << 16) + j;
		}
	    }
	}

	/*
	 * Some UNIX errno's map to multiple Sprite ReturnStatus'es;  in
	 * some cases, the wrong Sprite status was chosen above.  Touch
	 * up these special cases.
	 */

	table[EWOULDBLOCK-first] = FS_WOULD_BLOCK;
    }

    /*
     * The table is set up.  Do the mapping.
     */

    result = NO_STATUS;
    if ((unixErrno >= first) && (unixErrno <= last)) {
	result = table[unixErrno-first];
    }
    if (result == NO_STATUS) {
	fprintf(stderr, "*** compat: unknown errno value %d\n", unixErrno);
	return GEN_FAILURE;
    }
    return result;
}
