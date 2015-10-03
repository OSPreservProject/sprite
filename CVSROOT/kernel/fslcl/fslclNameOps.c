/* 
 * fsLclNameOps.c --
 *
 *	Routine for initializing the fsOpTable switch entries for 
 *	local naming domain.
 *
 * Copyright 1989 Regents of the University of California
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
#endif /* not lint */

#include <sprite.h>
#include <fs.h>
#include <fsio.h>
#include <fsNameOps.h>
#include <fslcl.h>
#include <fslclInt.h>

/*
 * fs_DomainLookup for FS_REMOTE_SPRITE_DOMAIN type.
 */

static Fs_DomainLookupOps lclDomainLookup = {
     Fsio_NoProc, FslclExport, FslclOpen, FslclGetAttrPath,
     FslclSetAttrPath, FslclMakeDevice, FslclMakeDir,
     FslclRemove, FslclRemoveDir, FslclRename, FslclHardLink,
};

static Fs_AttrOps lclAttrOpTable =   { FslclGetAttr, FslclSetAttr };


/*
 *----------------------------------------------------------------------
 *
 * FsLclInitializeOps --
 *
 *	Initialize the fsOpTable switch for the remote domain naming 
 *	and remote domain streams.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Fslcl_NameInitializeOps()
{

    Fs_InstallDomainLookupOps(FS_LOCAL_DOMAIN, &lclDomainLookup, 
			&lclAttrOpTable);
}

