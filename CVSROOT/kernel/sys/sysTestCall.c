/* 
 * sysTestCall.c --
 *
 *	Routines and structs for system calls.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "exc.h"
#include "sysSysCall.h"
#include "sys.h"
#include "dbg.h"
#include "machine.h"
#include "vm.h"
#include "sunMon.h"

int 
Test_PrintOut(args)
    struct {
	int argArray[SYS_MAX_ARGS];
    } args;
{
    struct {
	int 	arg;
	int 	mapped;
	int	numBytes;
    } nargs[SYS_MAX_ARGS];
    int		len;
    int		i;
    char	*string;

    Vm_MakeAccessible(VM_READONLY_ACCESS, 1024, args.argArray[0], 
					  &len, &(nargs[0].arg));
    if (len == 0) {
	return(FAILURE);
    }
    nargs[0].numBytes = len;

    nargs[0].mapped = TRUE;
    for (i = 1; i < SYS_MAX_ARGS; i++) {
	nargs[i].mapped = FALSE;
    }
    string = (char *) nargs[0].arg;
    i = 1;
    while (*string != '\0') {
	if (*string != '%') {
	    string++;
	    continue;
	}
	string++;
	if (*string == 's') {
	    Vm_MakeAccessible(VM_READONLY_ACCESS, 1024,
	      		      args.argArray[i], &len, &(nargs[i].arg));
	    if (len == 0) {
		for (i = 0; i < SYS_MAX_ARGS; i++) {
		    if (nargs[i].mapped) {
			Vm_MakeUnaccessible(nargs[i].arg, nargs[i].numBytes);
		    }
		}
		return(FAILURE);
	    }

	    nargs[i].mapped = TRUE;
	    nargs[i].numBytes = len;
	} else {
	    nargs[i].arg = args.argArray[i];
	}
	string++;
	i++;
    }
    Sys_SafePrintf(nargs[0].arg, nargs[1].arg, nargs[2].arg, nargs[3].arg, 
		   nargs[4].arg, nargs[5].arg, nargs[6].arg, nargs[7].arg, 
		   nargs[8].arg, nargs[9].arg);
    for (i = 0; i < SYS_MAX_ARGS; i++) {
	if (nargs[i].mapped) {
	    Vm_MakeUnaccessible(nargs[i].arg, nargs[i].numBytes);
	}
    }
    return(0);
}

int 
Test_GetLine(string, length)
    char	*string;
    int		length;
{
    int		i, numBytes;
    char 	*realString;
    int		loop = 1;

    Vm_MakeAccessible(VM_OVERWRITE_ACCESS, length, (Address) string,
		      &numBytes, (Address *) &realString);

    Mon_GetLine(1);
    i = 0;
    realString[i] = Mon_GetNextChar();
    while (i < length - 1 && realString[i] != '\0') {
	i++;
	realString[i] = Mon_GetNextChar();
    }
    realString[i] = '\0';

    Vm_MakeUnaccessible((Address *) realString, numBytes);

    return(SUCCESS);
}

int 
Test_GetChar(charPtr)
    char	*charPtr;
{
    char 	*realCharPtr;
    int		numBytes;

    Sys_Printf("Obsolete Test_GetChar() called\n");

    Vm_MakeAccessible(VM_OVERWRITE_ACCESS, 1, (Address) charPtr,
		      &numBytes, (Address *) &realCharPtr);
    if (numBytes == 0) {
	return(SYS_ARG_NOACCESS);
    }
    *realCharPtr = Mon_GetNextChar();

    Sys_Printf("%c", *realCharPtr);

    Vm_MakeUnaccessible((Address) realCharPtr, numBytes);

    return(SUCCESS);
}
