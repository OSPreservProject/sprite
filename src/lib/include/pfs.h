/*
 * pfs.h --
 *
 * Definitions for pseudo-file-system library routines.  The man page
 * for Pfs has necessary documentation.
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
 * $Header: /sprite/src/lib/include/RCS/pfs.h,v 1.4 89/06/23 11:27:48 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _PFSLIB
#define _PFSLIB

#include <fs.h>
#include <pdev.h>
#include <dev/pfs.h>

/*
 * Boolean that can be toggled by applications command line arguments.
 * This causes print statements that trace pfs operations.
 */
extern int pfs_Trace;

/*
 * The library keeps a set of callback procedures, one for each pfs request
 * that arrives on the naming request stream.  Fields can be set to NULL to get
 * a default (no-op) handler for the operation.  See the man page for the
 * calling sequence of each call-back procedure.
 */

typedef struct {
    int (*open)();		/* PFS_OPEN */
    int (*getAttr)();		/* PFS_GET_ATTR */
    int (*setAttr)();		/* PFS_SET_ATTR */
    int (*makeDevice)();	/* PFS_MAKE_DEVICE */
    int (*makeDir)();		/* PFS_MAKE_DIR */
    int (*remove)();		/* PFS_REMOVE */
    int (*removeDir)();		/* PFS_REMOVE_DIR */
    int (*rename)();		/* PFS_RENAME */
    int (*hardLink)();		/* PFS_HARD_LINK */
    int (*symLink)();		/* PFS_SYM_LINK */
    int (*domainInfo)();	/* PFS_DOMAIN_INFO */
} Pfs_CallBacks;

/*
 * A Pfs_Token is returned from the Pfs_Open call.  It is passed
 * back to the Pfs_SetHandler and Pfs_Close calls.
 */

typedef char *Pfs_Token;

#define PFS_MAGIC	0x4a3b2c1d

extern char pfs_ErrorMsg[];

extern	Pfs_Token	        Pfs_Open();
extern	int		       (*Pfs_SetHandler())();
extern	Pdev_Stream	       *Pfs_OpenConnection();
extern	int			Pfs_PassFile();
extern	void			Pfs_Close();
#endif /* _PFSLIB */
