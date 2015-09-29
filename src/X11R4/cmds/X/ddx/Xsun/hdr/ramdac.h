/* @(#)ramdac.h	1.1 88/11/15 */
#ifndef	_ramdac_DEFINED_
#define _ramdac_DEFINED_

#include <pixrect/pixrect.h>

#define RAMDAC_READMASK		04
#define RAMDAC_BLINKMASK	05
#define RAMDAC_COMMAND		06
#define RAMDAC_CTRLTEST		07

/* 3 Brooktree ramdac 457 or 458 packed in a 32-bit register */
/* fbunit defined in <pixrect/pixrect.h> */
struct ramdac {
    union fbunit    addr_reg,	       /* address register */
                    lut_data,	       /* lut data port */
                    command,	       /* command/control port */
                    overlay;	       /* overlay lut port */
};

#define ASSIGN_LUT(lut, value) (lut).packed = (value & 0xff) | \
	((value & 0xff) << 8) | ((value & 0xff) << 16)
/*
 * To initialize do this:
        struct ramdac  *lut;

        lut->addr_reg.packed = 0;
        ASSIGN_LUT (lut->addr_reg, CG8_RAMDAC_CTRLTEST);
	ASSIGN_LUT(lut->command, 04);
	ASSIGN_LUT (lut->addr_reg, CG8_RAMDAC_COMMAND);
        ASSIGN_LUT (lut->command, 0x43);
        ASSIGN_LUT (lut->addr_reg, CG8_RAMDAC_READMASK);
        ASSIGN_LUT (lut->command, 0xff);
        ASSIGN_LUT (lut->addr_reg, CG8_RAMDAC_BLINKMASK);
        ASSIGN_LUT (lut->command, 0);
 * followed by the colormap initialization.
 */

#endif	_ramdac_DEFINED_
