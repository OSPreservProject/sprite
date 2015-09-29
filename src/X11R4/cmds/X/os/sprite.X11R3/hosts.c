/*-
 * hosts.c --
 *	Functions to pretend to have a host access list.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /mic/X11R3/src/cmds/Xsp/os/sprite/RCS/hosts.c,v 1.6 89/10/25 17:22:02 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteos.h"

#include    "Xproto.h"
#include    "dixstruct.h"
#include    "opaque.h"

/*-
 *-----------------------------------------------------------------------
 * ResetHosts --
 *	Reset the host list from the initialization file, or wherever.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
ResetHosts (display)
    char    *display;	/* ASCII display number */
{
}

/*-
 *-----------------------------------------------------------------------
 * AddHost --
 *	Add a host to those allowed access
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
AddHost (client, family, length, pAddr)
    ClientPtr  	  client;
    int	    	  family;
    unsigned   	  length;
    pointer 	  pAddr;
{
    return (Success);
}

/*-
 *-----------------------------------------------------------------------
 * RemoveHost --
 *	Remove a host from those allowed access.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
RemoveHost (client, family, length, pAddr)
    ClientPtr  	  client;
    int	    	  family;
    unsigned	  length;
    pointer 	  pAddr;
{
    return (Success);
}

/*-
 *-----------------------------------------------------------------------
 * GetHosts --
 *	Get all the hosts in the access control list.
 *
 * Results:
 *	The number of hosts, size of the data buffer and whether the
 *	mechanism is enabled.
 *	Since there is no access control mechanism (short of chmod...)
 *	this always returns 0 hosts, 0 bytes of data and access-control
 *	disabled.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
GetHosts (data, pNumHosts, pEnabled)
    pointer 	  *data;
    int	    	  *pNumHosts;
    BOOL	  *pEnabled;
{
    *data = 0;
    *pEnabled = DisableAccess;
    *pNumHosts = 0;
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * ChangeAccessControl --
 *	Turn the access control mechanism on or off....Ha!
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
ChangeAccessControl (client, fEnabled)
    ClientPtr	  client;
    int		  fEnabled;
{
    return (BadImplementation);
}

/*-
 *-----------------------------------------------------------------------
 * InvalidHost --
 *	Used by the connection managers to decide if a connection should
 *	be allowed. First argument tells the type of connection and next
 *	two are the parameters for the connection.
 *
 * Results:
 *	TRUE if not ok. FALSE if ok.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Bool
InvalidHost (type, p1, p2)
    int	    type; 	/* Connection type */
    int	    p1;	  	/* UID for Pdev, family for TCP */
    int	    p2;	  	/* HostID for Pdev, addr for TCP */
{
    return (FALSE);
}
