/*
 * devNet.h --
 *
 *	This defines the interface to the file system net device. 
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVNET
#define _DEVNET

#include "sprite.h"

/*
 * Forward routines. 
 */


extern	ReturnStatus	DevNet_FsOpen();
extern	ReturnStatus	DevNet_FsRead();
extern	ReturnStatus	DevNet_FsWrite();
extern	ReturnStatus	DevNet_FsIOControl();
extern	ReturnStatus	DevNet_FsClose();
extern	ReturnStatus	DevNet_FsSelect();

extern	void	DevNetEtherHandler();

#endif _DEVNET

