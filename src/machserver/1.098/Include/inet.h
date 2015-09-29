/*
 * inet.h --
 *
 *	This file defines constants for the Internet Protocols server (inet).
 *
 * Copyright 1987, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/inet.h,v 1.3 89/06/23 11:30:20 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _INET
#define _INET

/*
 * Names of the various socket devices. The %s should be filled in with
 * host name.
 */

#define INET_STREAM_NAME_FORMAT	"/hosts/%s/netTCP"
#define INET_DGRAM_NAME_FORMAT	"/hosts/%s/netUDP"
#define INET_RAW_NAME_FORMAT	"/hosts/%s/netIP"


/*
 * Port numbers below INET_PRIV_PORTS are reserved to processes with super-user
 * privileges. Port numbers above INET_SERVER_PORTS are reserved for servers.
 */

#define INET_PRIV_PORTS		1024
#define INET_SERVER_PORTS	5000

#endif /* _INET */
