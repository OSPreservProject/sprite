/* 
 * fsOpTable.c --
 *
 *	The operation tables for the file system.  Skeletons are defined
 *	here, and they are initialized via calls by each module.  There
 *	are 4 main operation tables.  The 'Domain Lookup' routines are
 *	operations on pathnames.  The 'File Open' routines are invoked
 *	on the file server depending on the type of file being opened -
 *	these routines do preliminary open-time setup.  The 'Stream Op'
 *	table are the object-specific routines on I/O streams.  Finally,
 *	there is also an 'Attr Op' table used when getting attributes
 *	of objects.
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


#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsdm.h>

#include <stdio.h>

static ReturnStatus NoProc();

/*
 * Domain specific routine table for lookup operations.
 * The following operate on a single pathname.  They are called via
 * Fsprefix_LookupOperation with arguments described in fsOpTable.h
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
Fs_InstallDomainLookupOps(domainType, lookupOpsPtr, attrOpTablePtr)
    int		domainType;	/* Domain type - FS_LOCAL_DOMAIN, 
				 * FS_REMOTE_SPRITE_DOMAIN, etc. */
    Fs_DomainLookupOps *lookupOpsPtr;
				/* Name operation routines for the domain. */
    Fs_AttrOps	   *attrOpTablePtr; /* Domain specific get/set attributes table.
				     * entry for the domain. */
{
    int	i;

    if ((domainType < 0) || (domainType >= FS_NUM_DOMAINS)) {
	printf("Bad domain type %d in Fs_InstallDomainLookupOps\n", domainType);
	return;
    }
    fs_DomainLookup[domainType][FS_DOMAIN_IMPORT] = lookupOpsPtr->import;
    fs_DomainLookup[domainType][FS_DOMAIN_EXPORT] = lookupOpsPtr->export;
    fs_DomainLookup[domainType][FS_DOMAIN_OPEN] = lookupOpsPtr->open;
    fs_DomainLookup[domainType][FS_DOMAIN_GET_ATTR] = lookupOpsPtr->getAttrPath;
    fs_DomainLookup[domainType][FS_DOMAIN_SET_ATTR] = lookupOpsPtr->setAttrPath;
    fs_DomainLookup[domainType][FS_DOMAIN_MAKE_DEVICE] = 
						lookupOpsPtr->makeDevice;
    fs_DomainLookup[domainType][FS_DOMAIN_MAKE_DIR] = lookupOpsPtr->makeDir;
    fs_DomainLookup[domainType][FS_DOMAIN_REMOVE] = lookupOpsPtr->remove;
    fs_DomainLookup[domainType][FS_DOMAIN_REMOVE_DIR] = lookupOpsPtr->removeDir;
    fs_DomainLookup[domainType][FS_DOMAIN_RENAME] = lookupOpsPtr->rename;
    fs_DomainLookup[domainType][FS_DOMAIN_HARD_LINK] = lookupOpsPtr->hardLink;
    for (i = 0; i < FS_NUM_NAME_OPS; i++) { 
	if ((char *)(fs_DomainLookup[domainType][i]) == (char *) 0) {
		panic(
	"Fs_InstallDomainLookupOps missing routine for domainType %d\n", 
			domainType);
	}
    }
    fs_AttrOpTable[domainType] = *attrOpTablePtr;
}
