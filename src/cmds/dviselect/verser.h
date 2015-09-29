/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/* verser globals */

/*
 * Verser was rewritten based on another program by the same name.
 * The original program was written by Janet Incerpi of Brown University
 * and was for the original version of TeX which also used a different kind
 * of font file.  It was modified at the University of Washington by
 * Richard Furuta (bringing it up to TeX82 and PXL files) and Carl Binding
 * (adding horizontal printing).  I then tore it to shreds and rebuilt
 * it; the new one is much faster (though less portable:  it has inline
 * assembly code in various critical routines).
 *
 * Chris Torek, 20 May 1984, University of Maryland CS/EE
 *
 * The program has since gone through much revision.  The details are
 * rather boring, but there is one important point:  The intermediate
 * file format has changed.
 */

/*
 * Version number.  Increment this iff the intermediate file format
 * makes an incompatible change.  This number may not be > 127.
 */
#define	VERSION	1		/* was version 0 */

#ifndef ACCOUNT_FILE
/*
#define ACCOUNT_FILE "/usr/adm/vpacct"	/* if defined, the name of
					   the paper accounting file */
#endif	ACCOUNT_FILE

#ifndef VERSATEC_FILE
#define VERSATEC_FILE "/dev/vp0"/* the name of the Versatec */
#endif	VERSATEC_FILE

#define NFONTS	 100		/* max number of fonts */

#define FONTSHIFT 14		/* font shift in fcp's */
#define CHARSHIFT  7		/* char shift in fcp's */
#define CHARMASK 127		/* char mask in fcp's - 128 chars/font */
#define PARTMASK 127		/* part mask in fcp's */

#define	ROWS	400		/* lines in buffer (200 lines/inch) */
#define	COLUMNS	264		/* 2112 bits per line / 8 bits per char */
#define MIN_OUT  30		/* MIN_OUT lines in buffer causes output
				   to be written right away */
#define MaxCharHeight (ROWS-1)	/* max bit height of a single char or rule */
#define MaxPageHeight  1685	/* max bit height of a page */
#define MaxPageWidth   2112	/* max bit width  of a page */
#define FFMargin	 39	/* vert. offset after a formfeed (bits) */

#define DefaultMaxDrift 2

#define DefaultLeftMargin   150
#define MinimumLeftMargin   10
#define DefaultTopMargin    200
#define MinimumTopMargin    10
#define DefaultBottomMargin 200

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif	min
