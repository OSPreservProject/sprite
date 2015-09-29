/*
 * ccdev.h --
 *
 *	Declarations of SPUR Cache Controller device interface.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/ccdev.h,v 1.3 89/04/12 20:35:04 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVCC_H
#define _DEVCC_H

#include "spur.md/kernel/mach.h"	/* For MACH_MAX_NUM_PROCESSORS */
#include "spur.md/kernel/timerTick.h"

/* constants */
/*
 * Cache Controller Mode register bits.
 * (see SPUR-MSA page 24-25)
 */

#define	MODE_PERF_COUNTER_MASK	0x07
#define	MODE_GARTH		0x08
#define	MODE_T0_ENABLE		0x10
#define MODE_T1_ENABLE		0x20
#define	MODE_T2_ENABLE		0x40
#define	MODE_DAVID		0x80

/*
 * Mode register performance counters mode bits.
 *	(MODE_REG & MACH_MODE_PERF_COUNTER_MASK)
 * (See SPUR-MSA page 25)
 */

#define	MODE_PERF_COUNTER_OFF	0x00
#define	MODE_PERF_COUNTER_SNOOP	0x01
#define MODE_PERF_COUNTER_USER	0x05
#define MODE_PERF_COUNTER_SYS	0x06
#define	MODE_PERF_COUNTER_BOTH	0x07

#define	DEV_CC_MAX_OFFSET	sizeof(CCdev)	/* Size of CC. */

/*
 * User changable mode register bits.
 */

#define	DEV_CC_USER_MODE_BITS	((unsigned char) \
			~(MODE_T0_ENABLE|MODE_T1_ENABLE|MODE_T2_ENABLE))

/*
 * IoControl's for SPUR CC.
 *
 * IOC_CCDEV_SET_MODE - Set the mode register. Inbuffer should contain a
 * single unsigned char specifing the new mode register values.  Note that
 * only bits specified in DEV_CC_USER_MODE_BITS maybe changed by the user.
 * Other bits will be left unmodified. 
 * IOC_CCDEV_READ_T0 - Do a stable read of the 64bit counter T0. The outBuffer
 * contains a devCounter structure.
 */

#define	IOC_CCDEV		(2 << 16)
#define	IOC_CCDEV_SET_MODE	(IOC_CCDEV | 0x1) 	 
#define	IOC_CCDEV_READ_T0	(IOC_CCDEV | 0x2) 	 


/* data structures */

/*
 * CCdevCounters - Memory image of Cache Controller counter registers C0-C15.
 */

typedef struct CCdevCounters {
	unsigned int	value;		/* Value of counter. */
	unsigned int	padding;	/* Padding (not used). */
} CCdevCounters;

/*
 * CCdevRTPM - Memory image of Cache Contoller RPTM/GSN register. 
 */
typedef struct CCdevRTPM {
	unsigned int		:12;
	unsigned int	addr	:20;	/* Address */
	unsigned char	gsn;		/* GSN */
	unsigned int		:24;	/* padding. */
} CCdevRTPM;

/*
 * CCdevREG38 - Memory image of Cache Contoller 38 bit registers
 */
typedef struct CCdevREG38 {
	unsigned int	low;		/* Low 32 bits */
	unsigned char	high	:6;	/* High 6 bits */
	unsigned int		:26;	
} CCdevREG38;

/*
 * CCdev - Memory image of Cache Controller registers. 
 */

typedef struct CCdev {
	CCdevRTPM	rtpm[4];	/* RPTM{0,1,2,3} */
	CCdevREG38	gva;		/* GVA */
	CCdevREG38	pteva;		/* PTEVA */
	CCdevREG38	rpteva;		/* RPTEVA */
	CCdevREG38	g;		/* G */
	unsigned int	t0_low;		/* T0<31:0> */
	unsigned int		:32;	
	unsigned int	t0_high;	/* T0<63:32> */
	unsigned int		:32;	
	unsigned int	t1;		/* T1 */
	unsigned int		:32;	
	unsigned int	t2;		/* T2 */
	unsigned int		:32;	
	unsigned int	istatus;	/* Istatus */
	unsigned int		:32;	
	unsigned int	imask;		/* Imask */
	unsigned int		:32;	
	unsigned int	festatus;	/* FEStatus */
	unsigned int		:32;	
	unsigned char	slotid;		/* SlotId */
	unsigned char	mode;		/* Mode */
	unsigned int		:16;
	unsigned int		:32;	
	CCdevCounters		c[16];	/* C0 thru C15 */
} CCdev;

/*
 * A structure containing the 64bit version of the c0 thru c15 counters.
 */
typedef struct CCdev64bitCounter {
	unsigned int	low;	/* Low 32 bit of value. */
	unsigned int	high;	/* High 32 bit of value. */
} CCdev64bitCounter;

/*
 * The memory image of the processed cache controler device.
 */
typedef struct PCCdev {
	CCdev64bitCounter	t0;	/* The t0 64 bit free running counter */
	CCdev64bitCounter	c[16];	/* Counter c0 thru c15. */
} PCCdev[MACH_MAX_NUM_PROCESSORS];

/*
 * Compute offsets of a register.
 */

#define	CC_OFFSET_OF(field)	((int)&(((CCdev *) 0)->field))

/*
 * Smallest user writtable offset - nothing below the first counter.
 */

#define	DEV_CC_MIN_USER_OFFSET 	(CC_OFFSET_OF(c[0]))

#endif _DEVCC_H

