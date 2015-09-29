/*
 * nlist.h --
 *
 *	Declarations of the ultrix namelist structure.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.2 89/01/07 04:12:44 rab Exp $ SPRITE (Berkeley)

 */
/*	@(#)nlist.h	4.3	(ULTRIX)	9/1/88				      */

#ifndef _NLIST
#define _NLIST

/* constants */

/* data structures */

/* procedures */

#define	N_UNDF	0x0		/* undefined */
struct nlist {
    char *n_name;
    unsigned long n_value;
    short n_type;		/* 0 if not there, 1 if found */
    short reserved;
};
#endif /* _NLIST */
