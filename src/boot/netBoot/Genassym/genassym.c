
/*
 * @(#)genassym.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This module generates "assym.h" which contains all ".h" values
 * needed by assembler code.  These values are automatically generated
 * from the original ".h" files and automatically keep track of 
 * structure rearrangements, etc.  However, the first time you use
 * such a symbol, you have to add the printf() for it to this module.
 */

#include "sunromvec.h"
#include "cpu.addrs.h"
#include "cpu.map.h"
#include "interreg.h"
#include "enable.h"

/*
 * These unions are used to define page map entries and enable register
 * values, and get at their hex representations.
 */
union longmap {
	long	longval;
	struct pgmapent pgmapent;
};


main()
{
	/*
	 * Declare assorted registers that we're interested in.
	 */
	union longmap mapper;
	struct intstack *ip = 0;	/* Assume structs start at 0 */
	struct monintstack *mp = 0;

	/*
	 * Fields from cpu.addrs.h
	 */
	printf("#define SERIAL0_BASE 0x%x\n", SERIAL0_BASE);
	printf("#define INTERRUPT_BASE 0x%x\n", INTERRUPT_BASE);
	printf("#define CLOCK_BASE 0x%x\n", CLOCK_BASE);
	printf("#define MEMORY_ERR_BASE 0x%x\n", MEMORY_ERR_BASE);
	printf("#define MONBSS_BASE 0x%x\n", MONBSS_BASE);
	printf("#define STACK_BASE 0x%x\n", STACK_BASE);
	printf("#define TRAPVECTOR_BASE 0x%x\n", TRAPVECTOR_BASE);
	printf("#define PROM_BASE 0x%x\n", PROM_BASE);

	/*
	 * Fields from cpu.map.h
	 */
	printf("\n");
	printf("#define NUMCONTEXTS %d\n", NUMCONTEXTS);
	printf("#define NUMPMEGS %d\n", NUMPMEGS);
	printf("#define PGSPERSEG %d\n", PGSPERSEG);
	printf("#define BYTESPERPG %d\n", BYTESPERPG);
	printf("#define BYTES_PG_SHIFT %d\n", BYTES_PG_SHIFT);
	printf("#define ADRSPC_SIZE 0x%x\n", ADRSPC_SIZE);
	printf("#define	MAPADDRMASK 0x%x\n", MAPADDRMASK);
	printf("#define PMAPOFF 0x%x\n", PMAPOFF);
	printf("#define SMAPOFF 0x%x\n", SMAPOFF);
	printf("#define	IDPROMOFF 0x%x\n", IDPROMOFF);
	printf("#define CONTEXTOFF 0x%x\n", CONTEXTOFF);
	printf("#define CONTEXTMASK 0x%x\n", CONTEXTMASK);
	printf("#define FC_MAP 0x%x\n", FC_MAP);
	printf("#define FC_SP 0x%x\n", FC_SP);
	printf("#define LEDOFF 0x%x\n", LEDOFF);
	printf("#define ENABLEOFF 0x%x\n", ENABLEOFF);
	printf("#define BUSERROFF 0x%x\n", BUSERROFF);
	printf("#define PMREALBITS 0x%x\n", PMREALBITS);

	printf("\n");

	/* Page map entry for the canonical invalid page */
	mapper.longval = 0;
	mapper.pgmapent.pm_valid	= 0;
	mapper.pgmapent.pm_permissions	= PMP_RO_SUP;
	mapper.pgmapent.pm_type		= VPM_MEMORY;
	mapper.pgmapent.pm_accessed	= 0;
	mapper.pgmapent.pm_modified	= 0;
	mapper.pgmapent.pm_page		= 0;
	printf("#define PME_INVALID 0x%x\n", mapper.longval);

	/*
	 * Fields from interreg.h
	 */
	printf("\n");
	printf("#define	IR_ENA_INT 0x%x\n", IR_ENA_INT);
	printf("#define	IR_ENA_CLK7 0x%x\n", IR_ENA_CLK7);

	/*
	 * Fields from enable.h
	 */
	printf("\n");
	printf("#define	ENA_DIAG 0x%x\n", ENA_DIAG);
	printf("#define	ENA_COPY 0x%x\n", ENA_COPY);
	printf("#define	ENA_VIDEO 0x%x\n", ENA_VIDEO);
	printf("#define	ENA_CACHE 0x%x\n", ENA_CACHE);
	printf("#define	ENA_SDVMA 0x%x\n", ENA_SDVMA);
	printf("#define	ENA_FPP 0x%x\n", ENA_FPP);
	printf("#define	ENA_NOTBOOT 0x%x\n", ENA_NOTBOOT);

	exit(0);
}

