/* 
 * fileLoad.c --
 *
 *	The routine to load a program into main memory.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/scsiDiskBoot/RCS/fileLoad.c,v 1.7 89/01/06 08:14:27 brent Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "procAOUT.h"
#include "machMon.h"
#include "boot.h"

#define KERNEL_ENTRY 0x4000


/*
 *----------------------------------------------------------------------
 *
 * FileLoad --
 *
 *	Read in the kernel object file.  This is loaded into memory at
 *	a pre-defined location (in spite of what is in the a.out header)
 *	for compatibility with Sun/UNIX boot programs.  The Sprite kernel
 *	expects to be loaded into the wrong place and does some re-mapping
 *	to relocate the kernel into high virtual memory.
 *
 * Results:
 *	The entry point.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


int
FileLoad(handlePtr)
    register FsHandleHeader	*handlePtr;
{
    Proc_AOUT		aout;
    int			bytesRead;
    register int	*addr;
    register ReturnStatus status;
    register int	i;
    register int	numBytes;

    /*
     * Read a.out header.
     */

    status = Fs_Read(handlePtr, 0, sizeof(aout), &aout, &bytesRead);
    if (status != SUCCESS || bytesRead != sizeof(aout)) {
	Mach_MonPrintf("No a.out header");
	goto readError;
    } else if (aout.magic != PROC_OMAGIC) {
	Mach_MonPrintf("A.out? mag %x size %d+%d+%d\n",
	    aout.magic, aout.code, aout.data, aout.bss);
	return(-1);
    }

    /*
     * Read the code.
     */

    numBytes = aout.code;
    Mach_MonPrintf("Size: %d", numBytes);
    status = Fs_Read(handlePtr, PROC_CODE_FILE_OFFSET(aout), numBytes,
		      KERNEL_ENTRY, &bytesRead);
    if (status != SUCCESS) {
	goto readError;
    } else if (bytesRead != numBytes) {
	goto shortRead;
    }

    /*
     * Read the initialized data.
     */

    numBytes = aout.data;
    Mach_MonPrintf("+%d", numBytes);
    status = Fs_Read(handlePtr, PROC_DATA_FILE_OFFSET(aout), numBytes,
		      KERNEL_ENTRY + aout.code, &bytesRead);
    if (status != SUCCESS) {
readError:
	Mach_MonPrintf("\nRead error <%x>\n", status);
	return(-1);
    } else if (bytesRead != numBytes) {
shortRead:
	Mach_MonPrintf("\nShort read (%d)\n", bytesRead);
	return(-1);
    }

    /*
     * Zero out the bss.
     */

    numBytes = aout.bss;
    Mach_MonPrintf("+%d\n", numBytes);
    addr = (int *) (KERNEL_ENTRY + aout.code + aout.data);
    bzero(addr, numBytes);

    return (KERNEL_ENTRY);
}
