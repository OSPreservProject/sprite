/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/*		C-Tex I/O definitions		*/

/*
 * For non-ASCII systems, someone would have to do a great deal of work
 * in the various I/O routines to use the two translation tables.  I have
 * not the motivation.
 */
#ifdef notdef
int	InX[128];		/* input translation table */
int	OutX[128];		/* output translation table */
#endif

/*
 * These `magic constants' are really the best we can do.  Using termcap
 * would not be quite right, because (1) the C-TeX run may not be on a
 * terminal at all, and (2) the log file could very likely be viewed from
 * a different terminal than was used for the C-TeX run.
 */
#define MaxPrLineLen	72	/* will try to never print lines longer
				   than this */
#define MaxErrLineLen	72	/* errors (context actually) will never
				   be longer than this */
#define HalfErrLine	42	/* about 1/2 MaxErrLineLen; this will be
				   the width of first lines of contexts */

/*
 * Input will be read into lines of InBufLen characters.  This limits
 * several things, including control sequence name length and length of
 * strings read by \read.
 */
#define InBufLen	512

/* Output descriptors.  Note that 0-15 are indexes into OutF. */
#define OutIgnore	16	/* nowhere; output is silently discarded */
#define OutTerm		17	/* to terminal (stdout) */
#define OutLog		18	/* to log file */
#define OutTermAndLog	19	/* to terminal & log file both */
#define OutString	20	/* to PoolString */
#define OutCircBuf	21	/* to CircBuf */

int	OutFD;			/* the current output descriptor */
FILE	*LogF;			/* the log file */
FILE	*InF[16];		/* the 16 \read descriptors */
FILE	*OutF[16];		/* the 16 \write descriptors */
char	CircBuf[MaxErrLineLen];	/* this will be used for showing error
				   context; its length determines the
				   maximum amount of context that can
				   be shown.  This is a way of getting
				   around a possibly massive amount of
				   stuff leading up to the point of the
				   error, without using backpointers.
				   Inefficient perhaps, but it does not
				   occur often. */

/*
 * On occasion it is nice to be able to interrupt the current output
 * descriptor and switch to a new one.  This does not happen often,
 * so we just have a small stack of them here.
 */
int	OutFDstack[20];
int	*OutFDsp;

/* macros to switch to a new output, then restore the old */
#define PushOut(fd)	(*OutFDsp++ = OutFD, OutFD = (fd))
#define PopOut()	(OutFD = *--OutFDsp)

/*
 * The following magic variables are used by ShowContext to control the info
 * being inserted into the CircBuf.
 */
int	OutTally;		/* total characters recently printed */
int	OutSC_max;		/* max needed by ShowContext */

/*
 * UpdateTerminal makes sure that all output is visible by the user.
 * In this case all we need to do is fflush(stdout).
 */
#define UpdateTerminal() ((void) fflush(stdout))
