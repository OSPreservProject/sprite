/*
 * fslclInt.h --
 *
 *	Definitions of the parameters required for Local Domain operations.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSLOCALDOMAIN
#define _FSLOCALDOMAIN

#include "fscache.h"
#include "fsio.h"


/*
 * Image of a new directory.
 */
extern char *fslclEmptyDirBlock;


/*
 * Declarations for the Local Domain lookup operations called via
 * the switch in Fsprefix_LookupOperation.  These are called with a pathname.
 */
ReturnStatus FslclExport();
ReturnStatus FslclOpen();
ReturnStatus FslclLookup();
ReturnStatus FslclGetAttrPath();
ReturnStatus FslclSetAttrPath();
ReturnStatus FslclMakeDevice();
ReturnStatus FslclMakeDir();
ReturnStatus FslclRemove();
ReturnStatus FslclRemoveDir();
ReturnStatus FslclRename();
ReturnStatus FslclHardLink();

/*
 * Declarations for the Local Domain attribute operations called via
 * the fsAttrOpsTable switch.  These are called with a fileID.
 */
ReturnStatus FslclGetAttr();
ReturnStatus FslclSetAttr();

extern void FslclAssignAttrs();

#endif /* _FSLOCALDOMAIN */
