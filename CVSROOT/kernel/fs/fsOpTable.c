/* 
 * fsOpTable.c --
 *
 *	The operation tables for the file system.  They are encountered
 *	by the system in roughly the order they are presented here.  First
 *	the Domain Lookup routines are used for name operations.  They
 *	are used by Fsprefix_LookupOperation and Fsprefix_TwoNameOperation which use
 *	the prefix table to choose a server.  If a stream is to be made
 *	then the Open operations are used.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsutil.h"
#include "fsNameOps.h"
#include "fsdm.h"

static ReturnStatus NoProc();

/*
 * Domain specific routine table for lookup operations.
 * The following operate on a single pathname.  They are called via
 *	Fsprefix_LookupOperation with arguments described in fsOpTable.h
 *	DomainImport
 *	DomainExport
 *	DomainOpen
 *	DomainGetAttrPath
 *	DomainSetAttrPath
 *	DomainMakeDevice
 *	DomainMakeDir
 *	DomainRemove
 *	DomainRemoveDir
 * The following operate on two pathnames.
 *	DomainRename
 *	DomainHardLink
 */

/*
 * THIS ARRAY INDEXED BY DOMAIN TYPE.  Do not arbitrarily insert entries.
 */
ReturnStatus (*fs_DomainLookup[FS_NUM_DOMAINS][FS_NUM_NAME_OPS])() = {
/* FS_LOCAL_DOMAIN */
    {NoProc, NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc, NoProc},
/* FS_REMOTE_SPRITE_DOMAIN */
     {NoProc, NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc, NoProc},
/* FS_PSEUDO_DOMAIN */
    {NoProc, NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc, NoProc},
/* FS_NFS_DOMAIN */
    {NoProc, NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc,
     NoProc, NoProc, NoProc, NoProc},
};

/*
 * Domain specific get/set attributes table.  These routines are used
 * to get/set attributes on the name server given a fileID (not a pathname).
 */
Fs_AttrOps fs_AttrOpTable[FS_NUM_DOMAINS] = {
/* FS_LOCAL_DOMAIN */
    { NoProc, NoProc },
/* FS_REMOTE_SPRITE_DOMAIN */
    { NoProc, NoProc },
/* FS_PSEUDO_DOMAIN */
    { NoProc, NoProc },
/* FS_NFS_DOMAIN */
    { NoProc, NoProc },
};

static ReturnStatus
NoProc()
{
    return(FAILURE);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_InstallDomainLookupOps --
 *
 *	Install the fs_DomainLookup entry for a particular domain type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fs_DomainLookup table is modified.
 *
 *----------------------------------------------------------------------
 */

void
Fs_InstallDomainLookupOps(domainType, lookupTablePtr, attrOpTablePtr)
    int		domainType;	/* Domain type - FS_LOCAL_DOMAIN, 
				 * FS_REMOTE_SPRITE_DOMAIN, etc. */
    ReturnStatus  (*(*lookupTablePtr)[FS_NUM_NAME_OPS])(); 
				/* Name operation routines for the domain. */
    Fs_AttrOps	   *attrOpTablePtr; /* Domain specific get/set attributes table.
				     * entry for the domain. */
{
    int	i;

    if ((domainType < 0) || (domainType >= FS_NUM_DOMAINS)) {
	panic("Bad domain type %d in Fs_InstallDomainLookupOps\n", domainType);
	return;
    }

    for (i = 0; i < FS_NUM_NAME_OPS; i++) { 
	fs_DomainLookup[domainType][i] = (*lookupTablePtr)[i];
    }
    fs_AttrOpTable[domainType] = *attrOpTablePtr;
}
