/* 
 * main.c --
 *
 *	The main program for booting.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef  notdef
static char rcsid[] = "$Header: /sprite/src/boot/diskBoot.OpenProm/RCS/main.c,v 1.10 91/09/01 16:41:03 dlong Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "machMon.h"
#include "fsBoot.h"
#include "boot.h"

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
    register int index;			/* Loop index */
    register int entry;			/* Entry point of boot program */
    Fsio_FileIOHandle *handlePtr;	/* Handle for boot program file */
    char devBuf[64], *devName;
    char *fileName = "vmsprite";

    /*
     * The Sun prom collects the boot command line arguments and
     * puts makes them available throught the rom vector.
     */
    if (romVectorPtr->v_romvec_version >= 2) {
	devName = *romVectorPtr->bootpath;
    } else {
	MachMonBootParam *paramPtr;		/* Ref to boot params supplied
						 * by the PROM montor */
	paramPtr = *romVectorPtr->bootParam;
	devName = paramPtr->argPtr[0];
	Mach_MonPrintf("Sprite Boot %s %s\n", devName, paramPtr->fileName);

	for (index = 0 ; index < 8; ++index) {
	    if (paramPtr->argPtr[index] != (char *)0) {
		if (strcmp(paramPtr->argPtr[index], "-dev") == 0 && index < 7) {
		    devName = paramPtr->argPtr[++index];
		}
	    } else {
		break;
	    }
	}
	if (paramPtr->fileName[0]) {
	    fileName = paramPtr->fileName;
	}
    }

    /*
     * Set up state about the boot device.
     */
    dev_config(devName, &fsDevice);
    Mach_MonPrintf("Sprite Boot %s %s\n", devName, fileName);
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
	SunPromDevClose();
	goto exit;
    }
    entry = FileLoad(handlePtr);
    SunPromDevClose();
    if (entry != -1) {
#ifndef NO_PRINTF
	Mach_MonPrintf("Transferring to location 0x%x\n", entry);
#endif
	Boot_Transfer(entry, romVectorPtr);
    }
exit:
    (*romVectorPtr->exitToMon)();
}
