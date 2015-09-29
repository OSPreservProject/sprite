
/*	@(#)bootparam.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Constants for standalone I/O (bootstrap) code
 */
#define	LOADADDR	0x4000		/* Load address of boot pgms */
#define	BBSIZE		64*512		/* boot block size (from fs.h) */
#define	DEV_BSIZE	512		/* manifest */
