/* 
 * main.c --
 *
 *	The main program for booting.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef  notdef
static char rcsid[] = "$Header: /sprite/src/boot/sunprom/RCS/main.c,v 1.9 90/11/27 11:17:24 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "machMon.h"
#include "fsBoot.h"
#include "boot.h"
#undef NO_PRINTF

extern Fs_Device fsDevice;		/* Global FS device descriptor */

void Exit();

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      This gets arguments from the PROM, calls other routines to open
 *      and load the program to boot, and then transfers execution to that
 *      new program.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

main()
{
    ReturnStatus status;
    register MachMonBootParam *paramPtr;/* Ref to boot params supplied
					 * by the PROM montor */
    register int index;			/* Loop index */
    register int entry;			/* Entry point of boot program */
    Fsio_FileIOHandle *handlePtr;	/* Handle for boot program file */
    char *fileName = "vmsprite";

    /*
     * The Sun prom collects the boot command line arguments and
     * puts makes them available throught the rom vector.
     */
    paramPtr = *romVectorPtr->bootParam;
#ifdef notdef
    for (index=0 ; index < 8; index ++) {
	if (paramPtr->argPtr[index] != (char *)0) {
	    Mach_MonPrintf("Arg %d: %s\n", index, paramPtr->argPtr[index]);
	} else {
	    break;
	}
    }
    Mach_MonPrintf("Device %s\n", paramPtr->devName);
    Mach_MonPrintf("File \"%s\"\n", paramPtr->fileName);


#endif
    /*
     * Set up state about the boot device.
     */
    dev_config(paramPtr, &fsDevice);
    if (paramPtr->fileName[0]) {
	fileName = paramPtr->fileName;
    }
    /*
     * Set up state about the disk.
     */
    status = FsAttachDisk(&fsDevice);
#ifndef SCSI3_BOOT
    if (status != SUCCESS) {
	Mach_MonPrintf("Can't attach disk, status <0x%x>\n",  status);
	goto exit;
    }
    Mach_MonPrintf("Open File \"%s\"\n", fileName);
#endif
    /*
     * Open the file to bootstrap.
     */
    status = Open(fileName, FS_READ, 0, &handlePtr);
    if (status != SUCCESS) {
	Mach_MonPrintf("Can't open \"%s\", <%x>\n", fileName, status);
	goto exit;
    }
    entry = FileLoad(handlePtr);
    if (entry != -1) {
	Mach_MonPrintf("Transferring to location 0x%x\n", entry);
	Boot_Transfer(entry);
    }
exit:
    return;
}

/*
 * Exit is called by start.s
 */
void
Exit()
{
    /*
     * Return to start.s and then the PROM monitor.
     */
    return;
}
