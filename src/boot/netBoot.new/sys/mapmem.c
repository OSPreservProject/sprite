
/*
 * @(#)mapmem.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * mapmem.c:  Map things the way the Monitor likes them.
 *
 * This is called from a temporary stack by assembler code just after
 * diagnostics are finished.
 */

#include "../sun3/sunmon.h"
#include "../sun3/cpu.map.h"
#include "../sun3/cpu.addrs.h"
#include "../h/sunromvec.h"
#include "../h/pginit.h"
#include "../h/globram.h"
#include "../h/enable.h"
#include "../sun3/structconst.h"

/*
 * Table of page map entries used to map on-board I/O and PROM pages.
 */
struct pginit mapinit[] = {
	/*
	 * Initialization table for Sun-3 VME systems
	 */
#ifdef PRISM
	{(char *)PRISM_CFB_BASE, 1,
		{1, PMP_SUP, VPM_MEMORY, 0, 0, MEMPG_PRISM_CFB}},
		/*
		 * PRISM_CFB_BASE = 0xFD00000; base of color fb (virtual addr).
		 * MEMPG_PRISM_CFB = 0xFE800000 >> 13; physical page number.
		 */
#endif PRISM
	{(char *)KEYBMOUSE_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_KBM}},

	{(char *)SERIAL0_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_SERIAL0}},

	{(char *)EEPROM_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_EEPROM}},

	{(char *)CLOCK_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_CLOCK}},

	{(char *)MEMORY_ERR_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_MEMORY_ERR}},

	{(char *)INTERRUPT_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_INTERRUPT}},

	{(char *)ETHER_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_ETHER}},

	{(char *)COLORMAP_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_COLORMAP}},

	{(char *)AMD_ETHER_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_AMD_ETHER}},

	{(char *)SCSI_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_SCSI}},

	{(char *)DES_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_DES}},

	{(char *)ECC_CTRL_BASE, 0,
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_ECC_CTRL}},

	{((char *)ECC_CTRL_BASE)+BYTESPERPG, 0,		/* Unused */
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},

	{VIDEOMEM_BASE, 1,		/* Video memory (if we have any) */
		{1, PMP_SUP, VPM_MEMORY_NOCACHE, 0, 0, MEMPG_VIDEO}},
#ifdef PRISM
	{BW_ENABLE_MEM_BASE, 1,
		{1, PMP_SUP, VPM_MEMORY, 0, 0, MEMPG_BW_ENABLE}},
#endif PRISM

	{STACK_BASE, PGINITSKIP, 	/* Skip this stuff, already set up */
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},

	{MAINMEM_BITMAP, PGINITSKIP, 	/* Skip this stuff, already set up */
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},

	{BOOTMAP_BASE, 0, 		/* Boot map area, invalid for now */
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},

	{INVPMEG_BASE, 0,               /* Invalid pages, done later by hand */
                {0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},

	{(char *)PROM_BASE, 0,		/* Prom space */
		{1, PMP_SUP, VPM_IO, 0, 0, VIOPG_PROM}},

	{((char *)PROM_BASE)+PROMSIZE, 0,	/* Invalid all the way */
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},

	{(char *)(MONSHORTPAGE&MAPADDRMASK), PGINITEND,	/* End of table */
		{0, PMP_RO_SUP, VPM_MEMORY, 0, 0, 0}},
};

/*
 * Initial mapping of memory:
 *
 * We set up a useful segment map, then a useful page map.
 *
 * We leave things in context 0.
 * 
 * This code takes no special care to avoid trashing its own stack,
 * other than setting up segment 0 in each context safely before
 * entering that context.  It assumes that its caller put its stack
 * in segment 0 and knew what pmeg and pagemap it would eventually set
 * its own stack to, to avoid unpleasant surprises.
 */
unsigned long
mapmem(memsize)
	unsigned long memsize;
{
	register char *i;
	register unsigned short j;
	register context_t con;

	/*
	 * Map segments of all contexts to pmegs.
	 *
	 * Address Range	Mapped To		Why
	 * 0x00000000	8M	Pmegs 0x00 - 0x40	Main Memory
	 * 0x00800000	246M	Invalid Pmeg		Big Hole
	 * 0x0FE00000	1M	Pmegs 0xFF - 0xF8	MONSTART->MONEND
	 * 0x0FF00000	512K	Pmegs 0xF7 - 0xF4	DVMA
	 * 0x0FF80000	384K	Invalid Pmeg		Little Hole
	 * 0x0FFE0000	128K	One Pmeg from F7-F0	MONSHORTSEG
	 *
	 * We want to use the last pmeg as the Invalid Pmeg, but we want
	 * to make the last virtual address usable, since it contains
	 * four of the eight pages addressable with Short Abs. addresses.
	 * To do this, we swap the last pmeg and the one that
	 * would normally be mapped at the virtual address of the Invalid
	 * Pmeg.
	 *
	 * For the Prism, we leave 1Mbyte (8 pmegs) just before MONSTART
	 * for the color frame buffer.
	 *
	 */
	for (con = 0; con < NUMCONTEXTS; con++) {
		setcxsegmap_noint(con, (char *)0, 0);
		setcontext(con);
		i = 0;
		j = 0;
		for (; i < (char *)MAINMEM_MAP_SIZE;i += PGSPERSEG * BYTESPERPG)
			setsegmap(i, j++);
		for (; i < (char *)MONBEG; i += PGSPERSEG * BYTESPERPG)
			setsegmap(i, SEG_INVALID);

		j = SEG_INVALID - 1;			/* 0xFF - 1 = 0xFE */
		for (; i < (char *)MONEND; i += PGSPERSEG * BYTESPERPG)
			setsegmap(i, j--);
		for (; i < (char *)DVMA_BASE+DVMA_MAP_SIZE;
						i += PGSPERSEG * BYTESPERPG)
			setsegmap(i, j--);
		for (; i < (char *)ADRSPC_SIZE;	i += PGSPERSEG * BYTESPERPG)
			setsegmap(i, SEG_INVALID);

		j = getsegmap(INVPMEG_BASE);	/* INVPMEG_BASE=0xFE40000,
						 * which is between MONSTART
						 * and MONEND.
						 */
		setsegmap(INVPMEG_BASE, SEG_INVALID);
		setsegmap(MONSHORTSEG, j);

	}
	setcontext((context_t)0);

	/*
	 * Map all of memory according to initialization table
	 *
	 * First, we must map in the initialization table, since it's
	 * accessed as data.  We map two pages in case it spans a
	 * page boundary.
	 * 
	 * This code takes no special care to avoid trashing its own stack.
	 * It therefore assumes that its caller knew what page its would
	 * set its own stack to, and initialized the stack that way.
	 */
	setpgmap(((char *)mapinit)           , PME_PROM); 
	setpgmap(((char *)mapinit)+BYTESPERPG, PME_PROM);
	setupmap(mapinit);		/* Initialize whole page map */

	/*
	 * Initialize the stolen RAM pages.
	 *
	 * These are taken from the last N pages of working memory.
	 *
	 * Note that MONBSS_BASE is defined in some header files to be
	 * same as MONSHORTPAGE which is defined as STRTDATA.
	 */

	setpgmap(STACK_BASE,  0xd0000000 + (memsize >> 13) - 1);
        setpgmap(MONBSS_BASE, 0xd0000000 + (memsize >> 13) - 2);
	
	return (memsize - 2*BYTESPERPG);
}


/*
 * This subroutine initializes large sections of the page map.
 * It is called to initially set up the maps, and also called when
 * setting up a "Sun-1 fake" environment.
 *
 * k points to the first element of an entry in table (e.g., KEYBMOUSE_BASE,
 * SERIALO_BASE, etc.).  Each time the outer 'for' loop executes, k is pointing
 * to the next entry.  If the 2nd element of an entry is PGINITEND, then the
 * outer 'for' loop is not executed.  If it's 0 then only one page entry is
 * is used (e.g., {1, PMP_SUP, VPM_IO, 0, 0, VIOPG_KBM} for KEYBMOUSE_BASE)
 * for that device.  Note however, that even when it's 0 that the same entry
 * may be entered into several locations of the page map, as is the case for
 * PROM_BASE, which has 8 identical entries into the page map.  If the 2nd
 * element is 1, then a new page entry is generated for each page of address
 * space taken by that device (e.g., VIDEOMEM takes up 16 pages so it has
 * 16 different entries (the actual difference between entries is the physical
 * page number field of an entry) into the page table.
 *
 */
setupmap(table)
	struct pginit table[];
{
	register struct pginit *k;
	char *i;
	struct pgmapent pg;

	for (k = table; k->pi_incr != PGINITEND; k++) {
		i = k->pi_addr;
		pg = k->pi_pm;
		if (k->pi_incr != PGINITSKIP) {
			for (; i < k[1].pi_addr;
			       i += BYTESPERPG, pg.pm_page += k->pi_incr) {
				setpgmap(i, pg);
			}
		}
	}	/* end of for (k=table; ... ) */
}	/* end of setupmap() */

