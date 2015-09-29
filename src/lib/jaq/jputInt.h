/*
 * jputInt.h --
 *
 *	Declarations for use by the jput utility for Jaquith archive system
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
 * $Header: /sprite/lib/forms/RCS/jputInt.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */

#ifndef _JPUTINT
#define _JPUTINT

#define LINKNAMELEN 100 /* a Tar restriction */

#define DEF_ABSTRACT ""
#define DEF_ABSFILTER ""
#define DEF_MAIL ""
#define DEF_FORCE 0
#define DEF_SYNC 0
#define DEF_RECURSE INT_MAX
#define DEF_LINK 0
#define DEF_VERBOSE 0
#define DEF_MODTIME ""
#define DEF_MODTIMEVAL 0
#define DEF_FILTER ""
#define DEF_FILTEROPT ""
#define DEF_ABSFILTEROPT ""
#define DEF_PRUNE ""
#define DEF_PRUNEPATH ""
#define DEF_IGNORE "{#*#,*~}"
#define DEF_CROSS 0
#define DEF_NEWVOL 0
#define DEF_LOCAL 0
#define DEF_ACKFREQ 10

typedef struct Parms {
    char *server;
    int port;
    char *arch;
    char *abstract;
    char *absFilter;
    char *mail;
    int force;
    int sync;
    int newvol;
    int local;
    int recurse;
    int link;
    int verbose;
    char *modTime;
    time_t modTimeVal;
    char *absFilterOpt;
    char *prune;
    char *prunePath;
    char *ignore;
    int cross;
    int ackFreq;
} Parms;

#endif /* _JPUTINT */

