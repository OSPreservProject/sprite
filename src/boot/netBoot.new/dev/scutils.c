
/*
 * @(#)scutils.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/globram.h"
#include "../sun3/structconst.h"
#include "../sun3/cpu.addrs.h"
#include "../sun3/cpu.map.h"
#include "../h/pginit.h"
#define	u_short	unsigned short
#define	u_char	unsigned char
#define	u_int		unsigned int
#include "../sun3/memreg.h"
#include "../dev/cg2reg.h"
#include "../h/eeprom.h"

#define	TEMP_PAGE ((unsigned short *)(VIDEOMEM_BASE))
#define	ENA_VIDEO	0x08
#define DIAGSW	        0x01

struct pginit colorzap[] = {
	{VIDEOMEM_BASE, 1,		/* Video memory */
		{1, PMP_ALL, VPM_VME_COLOR, 0, 0, VMEPG_COLOR}},
	{((char *)VIDEOMEM_BASE)+128*1024, PGINITEND,
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},
};

struct cg2statusreg zero_statreg = {0};

/* ======================================================================
   Author : Peter Costello & John Gilmore
   Date   : April 21, 1982 & August 6, 1984
   Purpose : This routine initializes the color board. It clears the 
	frame buffer to color 0, enables all video planes, loads the 
	default color map, and enables video.
   Error Handling: Returns -1 if no color board.
   Returns: 1 if color board config is 1024x1024, 0 if 1152x900.
	   -1 if no color board.
   ====================================================================== */

int
init_scolor()
{
	register unsigned short *cmp, *endp;
	register unsigned short *reg = TEMP_PAGE;
	register struct cg2statusreg *statreg =
		(struct cg2statusreg *)(TEMP_PAGE + COLOR_STAT_PME_OFF);
	long oldpgmap;
	int result;

	set_enable(get_enable() | ENA_VIDEO);   /* turn on video */
	oldpgmap = getpgmap(reg);

	if ((EEPROM->ee_diag.eed_console < EED_CONS_TTYA || 
			EEPROM->ee_diag.eed_console > EED_CONS_COLOR) &&
			(get_enable() & DIAGSW) == 0)
				return -1; 
	/* 
	 * Initialize status register just in case, and check for
	 * whether the board really exists.
	 */
	setpgmap(reg, PME_COLOR_STAT);

	if (peek(reg) == -1) {
		setpgmap(reg, oldpgmap);		/* Restore temp page */
		result = -1;
	} else {
		setpgmap(reg, PME_COLOR_MAPS); 	/* Set up Color Maps */

		cmp = (unsigned short *)(reg + COLOR_MAPS_PME_OFF);
		endp = cmp + 256*3;

		while (cmp < endp) {
			*cmp++ = -1;
			*cmp++ = 0;
		}

		setpgmap(statreg, PME_COLOR_STAT);
                statreg->update_cmap = 1;
                while ( (statreg->retrace));            /* See retrace */
                while (!(statreg->retrace));            /* End of retrace */
                while ( (statreg->retrace));            /* Start of next one */
                statreg->update_cmap = 0;

		setpgmap(reg, PME_COLOR_ZOOM);
		*(short *)(reg + COLOR_ZOOM_PME_OFF) = 0;
		setpgmap(reg, PME_COLOR_WPAN);
		*(short *)(reg + COLOR_WPAN_PME_OFF) = 0;
		setpgmap(reg, PME_COLOR_PPAN);
		*(short *)(reg + COLOR_PPAN_PME_OFF) = 0;
		setpgmap(reg, PME_COLOR_VZOOM);
		*(short *)(reg + COLOR_VZOOM_PME_OFF) = 0xFF;
		setpgmap(reg, PME_COLOR_MASK);
                *(short *)(reg + COLOR_MASK_PME_OFF) = 0x01;

		setpgmap(reg, PME_COLOR_STAT);
		*statreg = zero_statreg;	/* Clear status register */
		statreg->video_enab = 1;	/* Enable video */
		result = 0;			/* Tell caller if 1024x1024 */
		if (statreg->resolution)
			result = 1;
		setupmap(colorzap);	/* map in color buffer */
	}
	return result;
}

