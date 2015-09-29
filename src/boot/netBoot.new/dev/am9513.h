
/*	@(#)am9513.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * am9513.h
 *
 * AMD Am9513 Timer definitions
 */

struct am9513_device {
	unsigned short	clk_data;	/* Registers read/written here */
	unsigned short	clk_cmd;	/* Commands written here */
};

/*
 * Commands.  (partial list)
 */
#define	CLK_RESET	0xFFFF		/* Master reset of chip */
#define	CLK_16BIT	0xFFEF		/* Use 16-bit, not 8-bit, bus */
#define	CLK_ARM		0xFF20		/* Arm (start) one or more counters */
#define	CLK_LOAD	0xFF40		/* Load (from Load reg) one or more */
#define	CLK_LOAD_ARM	0xFF60		/* Load and arm one or more */
#define	CLK_DISARM_SAVE	0xFF80		/* Disarm and save one or more */
#define	CLK_CLEAR	0xFFE0		/* Clear the output of one counter */
#define	CLK_ACC_MODE	0xFF00		/* Access Mode reg of one counter */
#define	CLK_ACC_LOAD	0xFF08		/* Access Load reg of one counter */

#define	CLK_ALL		0x001F		/* Perform op on all counters */
#define	CLK_BIT(c)	(1<<((c)-1))	/* Turn counter # into bit mask */
#define	CLK_LAST	5		/* Last timer number in chip */

/*
 * Mode Register settings, per counter.  (partial list)
 */
#define	CLKM_DEFAULT	0x0B00		/* Default setting at power-up/reset */
#define	CLKM_DIV_BY_1	0x0B00		/* Don't pre-divide input freq */
#define	CLKM_DIV_BY_16	0x0C00		/* Pre-divide input freq by 16 */
#define	CLKM_REPEAT	0x0020		/* When it runs out, start over */
#define	CLKM_TOGGLE	0x0002		/* When it runs out, toggle output */

/*
 * Status register bits, partial list.
 */
#define CLKS_BIT(c)	(1<<(c))	/* Turn counter # into status mask */
