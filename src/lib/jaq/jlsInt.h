/*
 * jlsInt.h --
 *
 *	Declarations for use by the jls utility for tapestry archive system
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
 * $Header: /sprite/lib/forms/RCS/jlsInt.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */

#ifndef _JLSINT
#define _JLSINT

#define MAXARG 100

#define DEF_RANGE ""
#define DEF_ASOF ""
#define DEF_SINCE ""
#define DEF_ABS ""
#define DEF_OWNER ""
#define DEF_GROUP ""
#define DEF_SYNC 0
#define DEF_MAIL ""
#define DEF_RECURSE 1
#define DEF_MODDATE 0
#define DEF_ALL 0
#define DEF_FIRSTVER -1
#define DEF_LASTVER -1
#define DEF_FIRSTDATE -1
#define DEF_LASTDATE -1
#define DEF_LPRINTFMT 0
#define DEF_IPRINTFMT 0
#define DEF_UPRINTFMT 0
#define DEF_APRINTFMT 0
#define DEF_SPRINTFMT 0
#define DEF_GPRINTFMT 0
#define DEF_DIRONLY 0
#define DEF_RAW 0

typedef struct parmTag {
    char *server;
    int port;
    char *arch;
    char *range;
    char *asof;
    char *since;
    char *abs;
    char *owner;
    char *group;
    int sync;
    char *mail;
    int recurse;
    int modDate;
    int all;
    int firstVer;
    int lastVer;
    int firstDate;
    int lastDate;
    int lPrintFmt;
    int iPrintFmt;
    int uPrintFmt;
    int aPrintFmt;
    int sPrintFmt;
    int gPrintFmt;
    int dirOnly;
    int raw;
} Parms;

#endif /* _JLSINT */
