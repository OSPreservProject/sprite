/*
 * cleanerInt.h --
 *
 *	Declarations for use by the cleaner process for Jaquith archive system
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
 * Quote:
 *      "Beware of all enterprises that require new clothes."
 *      -- Henry David Thoreau
 *
 * $Header: /sprite/lib/forms/RCS/cleaner.h,v 1.0 91/02/09 13:24:52 mottsmth Exp $ SPRITE (Berkeley)
 */

#ifndef _JCLEANINT
#define _JCLEANINT

#define SLOPSPACEK 2048       /* Safety space at end of volume */
#define LOGFREQ 5             /* Log cleaning record every Nth tbuf */

#define DEF_ARCHLIST "*.arch"
#define DEF_USER "root"
#define DEF_DEBUG 0
#define DEF_TBUFID -1
#define DEF_NEWVOL 0
#define DEF_DISKLOW 70
#define DEF_DISKHIGH 80

typedef struct parmTag {
    char *arch;
    char *root;
    char *userName;
    char *hostName;
    int debug;
    int tBufId;
    int newVol;
    int diskLow;
    int diskHigh;
} Parms;

#endif /* _JCLEANINT */

