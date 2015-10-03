/*
 * devSCSIC90.h --
 *
 *	Declarations of exported scsi stuff.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEV_SCSIC90
#define _DEV_SCSIC90

#ifdef sun4c
/*
 * This routine is called by the sun4c lance chip ethernet driver to
 * reset the dma controller, since only the scsi code knows when this
 * is okay or not.
 */
void	Dev_ScsiResetDMA _ARGS_ ((void));
void	Dev_ChangeScsiDebugLevel _ARGS_ ((int level));
#endif sun4c

#endif /* _DEV_SCSIC90 */
