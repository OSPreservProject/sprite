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

#include <sprite.h>
#include <stdio.h>
#include <procMach.h>
#include <proc.h>
#include <procInt.h>
#include <status.h>


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
ProcGetObjInfo(filePtr, execPtr, objInfoPtr)
    Fs_Stream		*filePtr;
    ProcExecHeader	*execPtr;
    ProcObjInfo		*objInfoPtr;
{

    int data[4];
    int sizeRead;
    ReturnStatus status;
    int excess;

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
    if (execPtr->dynamic) {
	return(PROC_BAD_AOUT_FORMAT);
    }
#endif
    switch (execPtr->magic) {

    case PROC_ZMAGIC:
	/*
	 * The following few lines are total hack.  The idea is to look at
	 * the startup code to see if it was a Sprite-compiled file, or
	 * a Unix-compiled file.
	 */
	sizeRead = 4*sizeof(int);
	status = Fs_Read(filePtr, (char *)data,
	    execPtr->entry-PROC_CODE_LOAD_ADDR(*execPtr), &sizeRead);
	if (status != SUCCESS) {
	    printf("READ failed\n");
	    return(PROC_BAD_AOUT_FORMAT);
	}
#ifdef sun3
	if (data[0]==0x241747ef && data[1]==0x42002 &&
		(data[2]==0x52807204 || data[2]==0x5280223c) &&
		((data[3]&0xffff0000)==0x4eb90000 || data[3]==4)) {
#else
	/* Normal sun4 startup code */
	if ((data[0]==0xac10000e && data[1]==0xac05a060 &&
		data[2]==0xd0058000 && data[3]==0x9205a004) ||
	/* Profiled sun4 startup code */
		(data[0]==0xbc100000 && data[1]==0x11000008 &&
		    data[2]==0x13000208 && data[3]==0x400038df)) {
#endif
	    goto spriteMagic;
	} else {
#ifdef sun3
	    /*
	     * Special check for emacs, which has weird startup code.
	     */
	    if (data[0]==0x4e560000 && data[1]==0x61064e5e &&
		    data[2]==0x4e750000) goto spriteMagic;
#endif

	    printf("Executing UNIX file in compatibility mode.\n");
	    goto unixMagic;
	}
spriteMagic:
	objInfoPtr->codeLoadAddr = (Address)PROC_CODE_LOAD_ADDR(*execPtr);
	objInfoPtr->codeFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	objInfoPtr->codeSize = execPtr->code;
	objInfoPtr->heapLoadAddr = (Address)PROC_DATA_LOAD_ADDR(*execPtr);
	objInfoPtr->heapFileOffset = PROC_DATA_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data;
	objInfoPtr->bssLoadAddr = (Address)PROC_BSS_LOAD_ADDR(*execPtr);
	objInfoPtr->bssSize = execPtr->bss;
	objInfoPtr->entry = (Address)execPtr->entry;
	objInfoPtr->unixCompat = 0;

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
	objInfoPtr->unixCompat = 0;
	break;

    case UNIX_ZMAGIC:
    unixMagic:

	objInfoPtr->codeLoadAddr =
	    (Address) (execPtr->entry < 0x2000 ? 0 : 0x2000);

	objInfoPtr->codeFileOffset = PROC_CODE_FILE_OFFSET(*execPtr);
	objInfoPtr->codeSize = execPtr->code;
#ifdef sun3
	objInfoPtr->heapLoadAddr = (Address) 0x20000
	    + (((int) objInfoPtr->codeLoadAddr +
	        execPtr->code - 1) & ~(0x20000 - 1));
	objInfoPtr->heapFileOffset = PROC_DATA_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data;
#else
	/*
	 * We have to shuffle things around so the heap is on a pmeg
	 * boundary.  This involves loading some of the code as heap.
	 */
	objInfoPtr->heapLoadAddr = (Address)
	    (((int) objInfoPtr->codeLoadAddr +
	        execPtr->code) & ~(0x20000 - 1));
	if (objInfoPtr->heapLoadAddr < objInfoPtr->codeLoadAddr) {
	    objInfoPtr->heapLoadAddr = objInfoPtr->codeLoadAddr;
	}
	objInfoPtr->heapFileOffset = PROC_DATA_FILE_OFFSET(*execPtr);
	objInfoPtr->heapSize = execPtr->data;
	excess = (objInfoPtr->codeLoadAddr+execPtr->code) -
		objInfoPtr->heapLoadAddr;
	objInfoPtr->heapFileOffset -= excess;
	objInfoPtr->heapSize += excess;
	objInfoPtr->codeSize -= excess;
#endif

	objInfoPtr->bssLoadAddr = objInfoPtr->heapLoadAddr + execPtr->data;
	objInfoPtr->bssSize = execPtr->bss;
	objInfoPtr->entry = (Address)execPtr->entry;
	objInfoPtr->unixCompat = 1;
	break;

    default:
	return(PROC_BAD_AOUT_FORMAT);
    }
    return(SUCCESS);
}
