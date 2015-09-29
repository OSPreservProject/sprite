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
# Mon Jun  8 14:22:16 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= symm.md/longjmp.s symm.md/modf.c symm.md/setjmp.s symm.md/_setjmp.s symm.md/_longjmp.s symm.md/crt0.c
HDRS		= symm.md/DEFS.h symm.md/Setjmp.h symm.md/values.h
MDPUBHDRS	= 
OBJS		= symm.md/longjmp.o symm.md/modf.o symm.md/setjmp.o symm.md/_setjmp.o symm.md/_longjmp.o symm.md/crt0.o
CLEANOBJS	= symm.md/longjmp.o symm.md/modf.o symm.md/setjmp.o symm.md/_setjmp.o symm.md/_longjmp.o symm.md/crt0.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
