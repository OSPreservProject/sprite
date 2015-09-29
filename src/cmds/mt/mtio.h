/*
 * mt.h --
 *
 *	Declarations for tape handling.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/sys/mtio.h,v 1.6 90/03/29 12:44:34 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _MTIO
#define _MTIO

#define MTWEOF          0       /* write an end-of-file record */
#define MTFSF           1       /* forward space over file mark */
#define MTBSF           2       /* backward space over file mark (1/2" only
 )*/
#define MTFSR           3       /* forward space to inter-record gap */
#define MTBSR           4       /* backward space to inter-record gap */
#define MTREW           5       /* rewind */
#define MTOFFL          6       /* rewind and put the drive offline */
#define MTNOP           7       /* no operation, sets status only */
#define MTRETEN         8       /* retension the tape (cartridge tape only) */
#define MTERASE         9       /* erase the entire tape */
#define MTEOM           10      /* position to end of media */
#define MTNBSF          11      /* backward space file to BOF */
#define MTASF           12      /* absolute position */
#define MTSTATUS        13      /* status inquiry */
#define MTCACHE         14      /* enable tape cache */
#define MTNOCACHE       15      /* disable tape cache */

#endif


