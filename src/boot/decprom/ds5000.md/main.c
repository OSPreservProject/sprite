/* 
 * main.c --
 *
 *	The main program for booting.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef  notdef
static char rcsid[] = "$Header: /sprite/src/boot/decprom/ds3100.md/RCS/main.c,v 1.2 90/06/27 14:57:11 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "kernel/machMon.h"
#include "fsBoot.h"
#include "boot.h"
#define NO_PRINTF

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

main(argc,argv,argenv)
int argc;
char **argv;
char **argenv;
{
    ReturnStatus status;
    register int index;			/* Loop index */
    register int entry;			/* Entry point of boot program */
    Fsio_FileIOHandle *handlePtr;	/* Handle for boot program file */
    char *fileName = "sprite";
    register char *i;
    char *boot;

    /*
     * Set up state about the disk.
     */
    Mach_MonPrintf("Performing dec disk boot\n");
    bzero((char *) &fsDevice, sizeof(fsDevice));
    status = FsAttachDisk(&fsDevice);
    if (status != SUCCESS) {
	Mach_MonPrintf("Can't attach disk, status <0x%x>\n",  status);
	goto exit;
    }
    boot = Mach_MonGetenv("boot");
    for (i=boot;*i != '\0'; i++) {
	if (i[0]=='/' && i[1] != '\0') {
	    fileName = i+1;
	}
    }
#ifndef NO_PRINTF
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
#ifndef NO_PRINTF
	Mach_MonPrintf("Transferring to location 0x%x\n", entry);
#endif
	Boot_Transfer(entry,argc,argv,argenv);
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
