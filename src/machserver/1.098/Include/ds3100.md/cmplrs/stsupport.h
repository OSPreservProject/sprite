/*	@(#)stsupport.h	4.2	(ULTRIX)	8/9/88				      */
#ifdef mips
/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: stsupport.h,v 1031.1 88/04/18 17:18:50 bettina Exp $ */
/*
 * Author	Mark I. Himelstein
 * Date Started	5/15/85
 * Purpose	provide support to uc to produce mips symbol tables.
 */

#ifdef LANGUAGE_C
AUXU _auxtemp;
#define AUXINT(c) ((_auxtemp.isym = c), _auxtemp)


/* the following struct frames the FDR dtructure and is used at runtime
 *	to access the objects in the FDR with pointers (since the FDR
 *	only has indeces.
 */
typedef struct {
	pFDR	pfd;		/* file descriptor for this file */
	pSYMR	psym;		/* symbols for this file */
	long	csymMax;	/* max allocated */
	pAUXU	paux;		/* auxiliaries for this file */
	long	cauxMax;	/* max allocated */
	char	*pss;		/* strings space for this file */
	long	cbssMax;	/* max bytes allowed in ss */
	pOPTR	popt;		/* optimization table for this file */
	long	coptMax;	/* max allocated */
	pLINER	pline;		/* lines for this file */
	long	clineMax;	/* max allocated */
	pRFDT	prfd;		/* file indirect for this file */
	long	crfdMax;	/* max allocated */
	pPDR	ppd;		/* procedure descriptor tables */
	long	cpdMax;		/* max allocated */
	long	freadin;	/* set if read in */
	} CFDR, *pCFDR;
#define cbCFDR sizeof (CFDR)
#define cfdNil ((pCFDR) 0)
#define icfdNil -1


/* the following struct embodies the HDRR dtructure and is used at runtime
 *	to access the objects in the HDRR with pointers (since the HDRR
 *	only has indeces.
 */
typedef struct {
	long	fappend;	/* are we appending to this beast ? */
	pCFDR	pcfd;		/* the compile time file descriptors */
	pFDR	pfd;		/* all of the file descriptors in this cu */
	long	cfd;		/* count of file descriptors */
	long	cfdMax;		/* max file descriptors */
	pSYMR	psym;		/* the symbols for this cu */
	pEXTR	pext;		/* externals for this cu */
	long	cext;		/* number of externals */
	long	cextMax;	/* max currently allowed */
	char	*pssext;	/* string space for externals */
	long	cbssext;	/* # of bytes in ss */
	long	cbssextMax;	/* # of bytes allowed in ss */
	pAUXU	paux;		/* auxiliaries for this cu */
	char	*pss;		/* string space for this cu */
	pDNR	pdn;		/* dense number table for this cu */
	long	cdn;		/* number of dn's */
	long	cdnMax;		/* max currently allowed */
	pOPTR	popt;		/* optimization table for this cu */
	pLINER	pline;		/* lines for this cu */
	pRFDT	prfd;		/* file indirect for this cu */
	pPDR	ppd;		/* procedure descriptor tables */
	int	flags;		/* which has been read in already */
	int	fswap;		/* do dubtables need to be swapped */
	HDRR	hdr;		/* header from disk */
	} CHDRR, *pCHDRR;
#define cbCHDRR sizeof (CHDRR)
#define chdrNil ((pCHDRR) 0)
#define ichdrNil -1
#endif LANGUAGE_C

/* constants and macros */

#define ST_FILESINIT	25 	/* initial number of files */
#define ST_STRTABINIT	512 	/* initial number of bytes in strring space */
#define ST_EXTINIT	32	/* initial number of symbols/file */
#define ST_SYMINIT	64	/* initial number of symbols/file */
#define ST_AUXINIT	64	/* initial number of auxiliaries/file */
#define ST_LINEINIT	512	/* initial number of auxiliaries/file */
#define ST_PDINIT	32	/* initial number of procedures in one file */
#define ST_DNINIT	128	/* initial # of dense numbers */
#define ST_FDISS	1	/* we expect a fd's iss to be this */
#define ST_IDNINIT	2	/* start the dense num tab with this entry */
#define ST_PROCTIROFFSET 1	/* offset from aux of proc's tir */
#define ST_RELOC	1	/* this sym has been reloced already */

#ifdef LANGUAGE_FORTRAN
#define ST_EXTIFD	0x7fffffff	/* ifd for externals */
#define ST_RFDESCAPE	0xfff	/* rndx.rfd escape says next aux is rfd */
#define ST_ANONINDEX	0xfffff /* rndx.index for anonymous names */
#endif LANGUAGE_FORTRAN

#ifdef LANGUAGE_C
#define ST_EXTIFD	0x7fffffff	/* ifd for externals */
#define ST_RFDESCAPE	0xfff	/* rndx.rfd escape says next aux is rfd */
#define ST_ANONINDEX	0xfffff /* rndx.index for anonymous names */
#define ST_PEXTS	0x01	/* mask, if set externals */
#define ST_PSYMS	0x02	/* mask, if set symbols */
#define ST_PLINES	0x04	/* mask, if set lines */
#define ST_PHEADER	0x08	/* mask, if set headers */
#define ST_PDNS		0x10	/* mask, if set dense numbers */
#define ST_POPTS	0x20	/* mask, if set optimization entries */
#define ST_PRFDS	0x40	/* mask, if set file indirect entries */
#define ST_PSSS		0x80	/* mask, if set string space */
#define ST_PPDS		0x100	/* mask, if set proc descriptors */
#define ST_PFDS		0x200	/* mask, if set file descriptors */
#define ST_PAUXS	0x400	/* mask, if set auxiliaries */
#define ST_PSSEXTS	0x800	/* mask, if set external string space */
#endif LANGUAGE_C

#ifdef LANGUAGE_PASCAL
#define ST_EXTIFD	16#7fffffff	/* ifd for externals */
#define ST_RFDESCAPE	16#fff	/* rndx.rfd escape says next aux is rfd */
#define ST_ANONINDEX	16#fffff /* rndx.index for anonymous names */
#define ST_PEXTS	16#01	/* mask, if set externals */
#define ST_PSYMS	16#02	/* mask, if set symbols */
#define ST_PLINES	16#04	/* mask, if set lines */
#define ST_HEADER	16#08	/* mask, if set header */
#define ST_PDNS		16#10	/* mask, if set dense numbers */
#define ST_POPTS	16#20	/* mask, if set optimization entries */
#define ST_PRFDS	16#40	/* mask, if set file indirect entries */
#define ST_PSSS		16#80	/* mask, if set string space */
#define ST_PPDS		16#100	/* mask, if set proc descriptors */
#define ST_PFDS		16#200	/* mask, if set file descriptors */
#define ST_PAUXS	16#400	/* mask, if set auxiliaries */
#define ST_PSSEXTS	16#800	/* mask, if set external string space */
#endif LANGUAGE_PASCAL

#define ST_FCOMPLEXBT(bt) ((bt == btStruct) || (bt == btUnion) || (bt == btTypedef) || (bt == btEnum))
#define valueNil	0
#define export

/*
 * Constants to describe aux types used in swap_aux( , ,type);
 */
#define ST_AUX_TIR	0
#define ST_AUX_RNDXR	1
#define ST_AUX_DNLOW	2
#define ST_AUX_DNMAC	3
#define ST_AUX_ISYM	4
#define ST_AUX_ISS	5
#define ST_AUX_WIDTH	6
#endif mips
