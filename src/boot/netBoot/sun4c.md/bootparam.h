/*
 * bootparam.h
 *
 * @(#)bootparam.h 1.5 88/02/08 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Constants for stand-alone I/O (bootstrap) code.
 */
#ifdef SUN4
#define	BBSIZE   (64*512) /* Boot block size.               */
#ifdef CACHE
#define LOADADDR 0x20000  /* Load address of boot programs. */
#else CACHE
#define LOADADDR 0x4000   /* Load address of boot programs. */
#endif CACHE
#else SUN4
#define BBSIZE   8192     /* Boot block size.               */
#define LOADADDR 0x4000   /* Load address of boot programs. */
#endif SUN4

#define	DEV_BSIZE 512
