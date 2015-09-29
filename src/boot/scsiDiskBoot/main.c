/* 
 * main.c --
 *
 *	The main program for booting.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef  notdef
static char rcsid[] = "$Header: /sprite/src/boot/scsiDiskBoot.new/RCS/main.c,v 1.6 89/01/06 08:12:02 brent Exp Locker: mendel $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "machMon.h"
#include "fs.h"
#include "fsInt.h"
#include "fsLocalDomain.h"
#include "boot.h"
#undef NO_PRINTF
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
    FsLocalFileIOHandle *handlePtr;	/* Handle for boot program file */
    char *fileName = "vmunix";

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
#ifndef SCSI3_BOOT
    /*
     * Set up for Block I/O to the boot device
     */
    Mach_MonPrintf("Sprite Boot %s(%d,%d,%d)%s\n", paramPtr->devName,
		     paramPtr->ctlrNum, paramPtr->unitNum,
		     paramPtr->partNum, paramPtr->fileName);
#endif
    /*
     * Probe the boot device.
     */
    Dev_Config();
    if (paramPtr->fileName[0]) {
	fileName = paramPtr->fileName;
    }

    status = FsAttachDisk(paramPtr->ctlrNum, paramPtr->unitNum, 
			  paramPtr->partNum);
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
    status = Fs_Open(fileName, FS_READ, 0, &handlePtr);
    if (status != SUCCESS) {
	Mach_MonPrintf("Can't open \"%s\", <%x>\n", fileName, status);
	goto exit;
    }
    entry = FileLoad(handlePtr);
    if (entry != -1) {
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
