/*-
 * main.c --
 *	First-level boot program for Sprite. Takes its arguments
 *	and uses tftp to download the appropriate kernel image.
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
"$Header: /sprite/src/boot/netBoot.OpenProm/RCS/main.c,v 1.2 91/01/13 02:39:23 dlong Exp $ SPRITE (Berkeley)";
#endif lint

#include    "boot.h"
#include    "mach.h"
#include    <string.h>

/*-
 *-----------------------------------------------------------------------
 * main --
 *	Main function for downloading stuff.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Begins the booted program.
 *
 *-----------------------------------------------------------------------
 */
main() 
{
    int		unitNum = 0;
    char	*fileName, *devName;
    int		startAddr;
    void	*fileId;
    
    /*
     * Enable interrupts so that L1-a and the milli-second timer work.
     */
    Mach_EnableIntr();
    if (!CheckRomMagic()) {
	printf("Do not know about ROM magic %x\n", RomMagic);
	ExitToMon();
    }
    printf ("\nROM version is %d\n", RomVersion);
    devName = BootDevName();
    printf("Boot Device: %s\n", devName);
    if (RomVersion >= 2) {
	sscanf(devName, "%*[^:]:%x", &unitNum);
    } else {
	sscanf(devName, "%*2c(%*x,%x,%*x)", &unitNum);
    }
    fileName = BootFileName();
    printf("Boot Path: %s\n", fileName);

    if ((strcmp(fileName, "vmunix") == 0) || (*fileName == '\0')) {
	fileName = BOOT_FILE;
    }
#ifdef sun4c
    if (strcmp(fileName, "showprom") == 0) {
	ShowProm();
	ExitToMon();
    }
#endif
    printf ("\nSpriteBoot: ");
    PrintBootCommand();

    fileId = (void *)DevOpen(devName);
    if (fileId == 0) {
	printf("DevOpen(\"%s\") failed.  Aborting.\n", devName);
	ExitToMon();
    }
    etheropen(fileId);
    startAddr = tftpload(fileId, fileName, unitNum);
    (void)DevClose(fileId);
    
    if (startAddr == -1){
	ExitToMon();
    } else {
	/*
	 * Jump to the address returned by tftpload
	 */
	printf("Starting execution at 0x%x\n", startAddr);
	startKernel(startAddr, (char *) romVectorPtr);
	return(startAddr);
    }
}
