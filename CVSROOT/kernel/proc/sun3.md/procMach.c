/* procMach.c --
 *
 *	Routine to interpret file header.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif /* not lint */

#include "sprite.h"
#include "procMach.h"
#include "proc.h"
#include "procInt.h"
#include "status.h"


/*
 *----------------------------------------------------------------------
 *
 * ProcGetObjInfo --
 *
 *	Translate the object file information into the machine independent
 *	form.
 *
 * Results:
 *	SUCCESS if could translate.
 *	PROC_BAD_AOUT_FORMAT if could not.
 *
 * Side effects:
 *	*objInfoPtr is filled in.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
ProcGetObjInfo(execPtr, objInfoPtr)
    ProcExecHeader	*execPtr;
    ProcObjInfo		*objInfoPtr;
{

#ifdef sun2
    if (execPtr->machineType != PROC_MC68010) {
	return(PROC_BAD_AOUT_FORMAT);
    }
#endif
#ifdef sun3
    if (execPtr->machineType != PROC_MC68010 &&
        execPtr->machineType != PROC_MC68020) {
	return(PROC_BAD_AOUT_FORMAT);
    }
#endif
#ifdef sun4
    /*
     * Sun's compiler includes a tool version number or something in the
     * top 16 bits of the machineType field, so we can only look at the
     * low 16 bits.
     */
    if ((execPtr->machineType & 0xff) != PROC_SPARC) {
	return(PROC_BAD_AOUT_FORMAT);
    }
#endif
    switch (execPtr->magic) {

    case PROC_ZMAGIC:
	objInfoPtr->codeLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->codeFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	objInfoPtr->codeSize = execPtr->code;
	objInfoPtr->heapLoadAddr = (Address)PROC_DATA_LOAD_ADDR(*execPtr);
	objInfoPtr->heapFileOffset = PROC_DATA_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data;
	objInfoPtr->bssLoadAddr = (Address)PROC_BSS_LOAD_ADDR(*execPtr);
	objInfoPtr->bssSize = execPtr->bss;
	objInfoPtr->entry = (Address)execPtr->entry;
	break;

    case PROC_OMAGIC:
	objInfoPtr->codeLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->codeFileOffset = 0;
	objInfoPtr->codeSize = 0;
	objInfoPtr->heapLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->heapFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data + execPtr->code;
	objInfoPtr->bssLoadAddr = (Address)PROC_BSS_LOAD_ADDR(*execPtr);
	objInfoPtr->bssSize = execPtr->bss;
	objInfoPtr->entry = (Address)execPtr->entry;
	break;

    default:
	return(PROC_BAD_AOUT_FORMAT);
    }
    return(SUCCESS);
}
