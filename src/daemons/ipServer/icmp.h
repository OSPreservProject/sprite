/*
 * icmp.h --
 *
 *	Declarations of external ICMP-related routines.
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
 *
 *
 * $Header: /sprite/src/daemons/ipServer/RCS/icmp.h,v 1.3 89/08/15 19:55:15 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_ICMP
#define _IPS_ICMP


extern void		ICMP_Init();
extern void		ICMP_SendErrorMsg();

#endif /* _IPS_ICMP */
