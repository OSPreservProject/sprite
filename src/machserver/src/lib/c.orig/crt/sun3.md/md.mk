#
# Prototype Makefile for machine-dependent directories.
#
# A file of this form resides in each ".md" subdirectory of a
# command.  Its name is typically "md.mk".  During makes in the
# parent directory, this file (or a similar file in a sibling
# subdirectory) is included to define machine-specific things
# such as additional source and object files.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.md
# Mon Nov  5 08:33:07 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/start.s sun3.md/Faintd.s sun3.md/dummy.s sun3.md/modf.s sun3.md/lmult.s sun3.md/ldivt.s sun3.md/alloca.s sun3.md/gmcount.s sun3.md/gstart.s sun3.md/gmon.c sun3.md/_setjmp.s sun3.md/setjmp.s sun3.md/fp_globals.s sun3.md/ptwo.s sun3.md/lmodt.s
HDRS		= sun3.md/DEFSInt.h sun3.md/divide.h sun3.md/fpcrtInt.h sun3.md/gmon.h
MDPUBHDRS	= 
OBJS		= sun3.md/Faddd.o sun3.md/Fadds.o sun3.md/Fdtos.o sun3.md/Ffloat.o sun3.md/Ffltd.o sun3.md/Fflts.o sun3.md/Fmuld.o sun3.md/Fmuls.o sun3.md/Fstod.o sun3.md/float_switch.o sun3.md/frexp.o sun3.md/ldexp.o sun3.md/start.o sun3.md/Faintd.o sun3.md/dummy.o sun3.md/modf.o sun3.md/lmult.o sun3.md/ldivt.o sun3.md/alloca.o sun3.md/gmcount.o sun3.md/gstart.o sun3.md/gmon.o sun3.md/_setjmp.o sun3.md/setjmp.o sun3.md/fp_globals.o sun3.md/ptwo.o sun3.md/lmodt.o
CLEANOBJS	= sun3.md/start.o sun3.md/Faintd.o sun3.md/dummy.o sun3.md/modf.o sun3.md/lmult.o sun3.md/ldivt.o sun3.md/alloca.o sun3.md/gmcount.o sun3.md/gstart.o sun3.md/gmon.o sun3.md/_setjmp.o sun3.md/setjmp.o sun3.md/fp_globals.o sun3.md/ptwo.o sun3.md/lmodt.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= sun3.md/Faddd.o sun3.md/Fadds.o sun3.md/Fdtos.o sun3.md/Ffloat.o sun3.md/Ffltd.o sun3.md/Fflts.o sun3.md/Fmuld.o sun3.md/Fmuls.o sun3.md/Fstod.o sun3.md/float_switch.o sun3.md/frexp.o sun3.md/ldexp.o
