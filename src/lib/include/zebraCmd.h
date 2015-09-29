/*
 * zebraCmd.h --
 *
 *	Declarations for the Zebra_Cmd system call.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.8 92/03/02 15:32:57 bmiller Exp $ SPRITE (Berkeley)
 */

#ifndef _ZEBRA_CMD
#define _ZEBRA_CMD

/*
 * Commands.
 */

#define ZEBRA_ADD_DOMAIN	1
#define ZEBRA_DELETE_DOMAIN	2

/*
 * Argument for ZEBRA_DELETE_DOMAIN.
 */
typedef struct {
    int		serverID;
    int		domainID;
} Zebra_DeleteDomainCmd;

#endif /* _ZEBRA_CMD */

