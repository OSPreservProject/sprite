/*
 * fsLocalDomain.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLCL
#define _FSLCL

#include "fscache.h"
#include "fsio.h"


/*
 * Misc. routines.
 */
extern void	     Fslcl_DomainInit();
extern ReturnStatus  Fslcl_DeleteFileDesc();
extern void Fslcl_NameInitializeOps();



#endif /* _FSLCL */
