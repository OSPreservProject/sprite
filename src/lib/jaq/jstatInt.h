/*
 * jstatInt.h --
 *
 *	Declarations for use by the jstat utility for Jaquith archive system
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/jstatInt.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */

#ifndef _JSTATINT
#define _JSTATINT

#define DEF_MAIL ""
#define DEF_DEVICES 0
#define DEF_QUEUES 0
#define DEF_ARCHLIST 0

typedef struct Parms {
    char *server;
    int port;
    char *arch;
    char *mail;
    int devices;
    int queues;
    int archList;
} Parms;

#endif /* _JSTATINT */


