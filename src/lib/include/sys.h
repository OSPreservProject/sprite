/*
 * sys.h --
 *
 *     User-level definitions of routines and types for the sys module.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/sys.h,v 1.11 91/09/01 01:57:20 dlong Exp $ SPRITE (Berkeley)
 *
 */

#ifndef _SYSUSER
#define _SYSUSER

#ifndef _SPRITE
#include <sprite.h>
#endif

typedef enum {
    SYS_WARNING,
    SYS_FATAL
} Sys_PanicLevel;

/*
 * Flags for Sys_Shutdown.
 *
 *    SYS_REBOOT         Reboot the system. 
 *    SYS_HALT           Halt the system.
 *    SYS_KILL_PROCESSES Kill all processes.
 *    SYS_DEBUG		 Enter the debugger.
 *    SYS_WRITE_BACK	 Write back the cache after killing all processes but
 *			 obviously before halting or rebooting.
 */

#define SYS_REBOOT              0x01
#define SYS_HALT                0x02
#define	SYS_KILL_PROCESSES	0x04
#define	SYS_DEBUG		0x08
#define	SYS_WRITE_BACK		0x10

/*
 * Structure that is filled in by Sys_GetMachineInfo.
 */


/*
 * Machine architecture and type values from Sys_GetMachineInfo().
 */

typedef struct {
    int architecture;		/* machine architecture */
    int type;			/* machine type */
    int	processors;		/* number of processors */
} Sys_MachineInfo;

#define SYS_SPUR		1
#define SYS_SUN2		2
#define SYS_SUN3		3
#define SYS_SUN4		4
#define SYS_MICROVAX_2		5
#define SYS_DS3100		6   /* DecStation 3100 */
#define SYS_SYM                 7   /* Sequent symmetry */
#define SYS_DS5000              8   /* DecStation 5000 */

#define SYS_SUN_ARCH_MASK	0xf0
#define	SYS_SUN_IMPL_MASK	0x0f

#define	SYS_SUN_2		0x00
#define	SYS_SUN_3		0x10
#define	SYS_SUN_4		0x20
#define	SYS_SUN_4_C		0x50

#define SYS_SUN_2_50		0x02
#define SYS_SUN_2_120		0x01
#define SYS_SUN_2_160		0x02
#define SYS_SUN_3_75		0x11
#define SYS_SUN_3_160		0x11
#define SYS_SUN_3_50		0x12
#define	SYS_SUN_3_60		0x17
#define	SYS_SUN_4_200		0x21
#define	SYS_SUN_4_260		0x21
#define	SYS_SUN_4_110		0x22
#define	SYS_SUN_4_330		0x23
#define	SYS_SUN_4_460		0x24
#define	SYS_SUN_4_470		0x24
#define	SYS_SUN_4_C_60		0x51
#define	SYS_SUN_4_C_40		0x52
#define	SYS_SUN_4_C_65		0x53
#define	SYS_SUN_4_C_20		0x54
#define	SYS_SUN_4_C_75		0x55

extern ReturnStatus		Sys_GetMachineInfo();

#endif /* _SYSUSER */
