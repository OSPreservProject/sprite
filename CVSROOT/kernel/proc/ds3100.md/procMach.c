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
    if (execPtr->fileHeader.magic != PROC_OBJ_MAGIC) {
	return(PROC_BAD_AOUT_FORMAT);
    }
    switch (execPtr->aoutHeader.magic) {
	case PROC_ZMAGIC:
	    objInfoPtr->codeLoadAddr = execPtr->aoutHeader.codeStart;
	    objInfoPtr->codeFileOffset = 0;
	    objInfoPtr->codeSize = execPtr->aoutHeader.codeSize;
	    objInfoPtr->heapLoadAddr = execPtr->aoutHeader.heapStart;
	    objInfoPtr->heapFileOffset = execPtr->aoutHeader.codeSize;
	    objInfoPtr->heapSize = execPtr->aoutHeader.heapSize;
	    objInfoPtr->bssLoadAddr = execPtr->aoutHeader.bssStart;
	    objInfoPtr->bssSize = execPtr->aoutHeader.bssSize;
	    objInfoPtr->entry = execPtr->aoutHeader.entry;
	    break;
	case PROC_OMAGIC:
	    if (execPtr->aoutHeader.codeStart+execPtr->aoutHeader.codeSize !=
		    execPtr->aoutHeader.heapStart) {
		printf("OMAGIC output file must have data segment %s\n",
			"immediately following text segment.");
		return(PROC_BAD_AOUT_FORMAT);
	    }
	    if (execPtr->aoutHeader.codeStart <= (Address)DEFAULT_TEXT) {
		printf("OMAGIC text segment is going to collide with %s\n",
			"header segment.");
		return(PROC_BAD_AOUT_FORMAT);
	    }
	    objInfoPtr->codeLoadAddr = (Address)DEFAULT_TEXT;
	    objInfoPtr->codeFileOffset = 0;
	    objInfoPtr->codeSize = 0;
	    objInfoPtr->heapLoadAddr = execPtr->aoutHeader.codeStart;
	    objInfoPtr->heapFileOffset = TextOffset(execPtr);
	    objInfoPtr->heapSize = execPtr->aoutHeader.codeSize +
		execPtr->aoutHeader.heapSize;
	    objInfoPtr->bssLoadAddr = execPtr->aoutHeader.bssStart;
	    objInfoPtr->bssSize = execPtr->aoutHeader.bssSize;
	    objInfoPtr->entry = execPtr->aoutHeader.entry;
	    break;
	default:
	    return(PROC_BAD_AOUT_FORMAT);
    }
    return(SUCCESS);
}
