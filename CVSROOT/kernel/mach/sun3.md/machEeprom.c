/* 
 * machEeprom.c --
 *
 *	Interface to the EEPROM o the Sun 3.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "machConst.h"
#include "machMon.h"
#include "machEeprom.h"
#include "machInt.h"
#include "mach.h"


/*
 *----------------------------------------------------------------------
 *
 * Mach_EepromPrintConfig --
 *
 *	Print system configuration information from the EEPROM.	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Mach_EepromPrintConfig()
{
    struct eed_conf *confPtr;
    int slot;

    confPtr = machEepromPtr->ee_diag.ee_conf[0];
    for (slot=0 ; slot<MACH_MAX_SLOTS ; slot++, confPtr++) {
	if (confPtr->eec_un.eec_gen.eec_type == MACH_SLOT_TYPE_NONE) {
	    continue;
	}
	printf ("Slot %d (%x): ", slot, confPtr->eec_un.eec_gen.eec_type);
	switch (confPtr->eec_un.eec_gen.eec_type) {
	    case MACH_SLOT_TYPE_CPU:
		printf(" CPU with %d Meg %s %s cache %d Kbytes\n",
		    confPtr->eec_un.eec_cpu.eec_cpu_mem_size,
		    confPtr->eec_un.eec_cpu.eec_cpu_dcp ? "DCP" : "",
		    confPtr->eec_un.eec_cpu.eec_cpu_68881 ? "68881" : "",
		    confPtr->eec_un.eec_cpu.eec_cpu_cachesize);
		break;
	    case MACH_SLOT_TYPE_MEM:
		printf(" Memory %8d bytes",
			confPtr->eec_un.eec_mem.eec_mem_size);
		break;
	    case MACH_SLOT_TYPE_COLOR:
		printf(" Color type %d",
			confPtr->eec_un.eec_color.eec_color_type);
		break;
	    case MACH_SLOT_TYPE_BW:
		printf(" Black & White");
		break;
	    case MACH_SLOT_TYPE_FPA:
		printf(" Floating Point Accelerator");
		break;
	    case MACH_SLOT_TYPE_DISK:
		printf(" %s #%d disks %d",
		    (confPtr->eec_un.eec_disk.eec_disk_type ==
			MACH_SLOT_DISK_TYPE_X450) ? "Xylogics 450"
						  : "Xylogics 451",
		    confPtr->eec_un.eec_disk.eec_disk_ctlr,
		    confPtr->eec_un.eec_disk.eec_disk_disks);
		break;
	    case MACH_SLOT_TYPE_TAPE:
		printf(" 1/2\" Tape #%d tapes %d",
		    (confPtr->eec_un.eec_tape.eec_tape_type ==
			MACH_SLOT_TAPE_TYPE_XT) ? "Xylogics 472"
						  : "TapeMaster",
		    confPtr->eec_un.eec_tape.eec_tape_ctlr,
		    confPtr->eec_un.eec_tape.eec_tape_tapes);
		break;
	    case MACH_SLOT_TYPE_TTY:
		printf(" TTY");
		break;
	    case MACH_SLOT_TYPE_GP:
		printf(" GP");
		break;
	    case MACH_SLOT_TYPE_DCP:
		printf(" DCP");
		break;
	    case MACH_SLOT_TYPE_SCSI:
		printf(" SCSI-%d, %d tapes type #%d,  %d disks type #%d",
		    confPtr->eec_un.eec_scsi.eec_scsi_type,
		    confPtr->eec_un.eec_scsi.eec_scsi_tapes,
		    confPtr->eec_un.eec_scsi.eec_scsi_tape_type,
		    confPtr->eec_un.eec_scsi.eec_scsi_disks,
		    confPtr->eec_un.eec_scsi.eec_scsi_disk_type);
		break;
	    case MACH_SLOT_TYPE_IPC:
		printf(" IPC");
		break;
	    case MACH_SLOT_TYPE_GB:
		printf(" GB");
		break;
	    case MACH_SLOT_TYPE_SCSI375:
		printf(" SCSI 3/75");
		break;
	    case MACH_SLOT_TYPE_MAPKIT:
		printf(" MapKit");
		break;
	    case MACH_SLOT_TYPE_CHANNEL:
		printf(" Channel");
		break;
	    case MACH_SLOT_TYPE_END:
		printf(" End of Card Cage");
		break;
	}
	printf("\n");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_EepromGetConfig --
 *
 *	Return system configuration information from the EEPROM.	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Mach_EepromGetConfig(cpuPtr, memSizePtr, colorPtr, fpuPtr)
    Mach_CpuConfig *cpuPtr;		/* Not implemented */
    unsigned int *memSizePtr;		/* Number of bytes */
    Mach_ColorConfig *colorPtr;		/* Color frame buffer info */
    Mach_FpuConfig *fpuPtr;		/* FPU info */
{
    struct eed_conf *confPtr;
    int slot;
    unsigned int memSize = 0;

    *fpuPtr->valid = 0;
    *colorPtr->valid = 0;

    confPtr = machEepromPtr->ee_diag.ee_conf[0];
    for (slot=0 ; slot<MACH_MAX_SLOTS ; slot++, confPtr++) {
	if (confPtr->eec_un.eec_gen.eec_type == MACH_SLOT_TYPE_NONE) {
	    continue;
	}
	switch (confPtr->eec_un.eec_gen.eec_type) {
	    case MACH_SLOT_TYPE_CPU:
		break;
	    case MACH_SLOT_TYPE_COLOR:
		colorPtr->valid = 1;
		colorPtr->type = confPtr->eec_un.eec_color_type;
		break;
	    case MACH_SLOT_TYPE_MEM:
		memSize += confPtr->eec_un.eec_mem.eec_mem * 1024 * 1024;
		break;
	    case MACH_SLOT_TYPE_FPA:
		fpuPtr->valid = 1;
		break;
	}
    }
    *memSizePtr = memSize;
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_ColorBoardInfo --
 *
 *	Return configuration information about the color board, if any.	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Mach_ColorBoardPresent()
    Mach_ColorConfig *colorPtr;		/* Color frame buffer info */
{
    unsigned int memSize;
    Mach_CpuConfig cpu;
    Mach_FpuConfig fpu;
    Mach_ColorConfig color

    Mach_EepromGetConfig(&cpu, &memSize, &color, &fpu);
    return(color.valid);
}
