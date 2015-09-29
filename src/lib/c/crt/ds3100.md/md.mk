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
# Mon Jun  8 14:22:00 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/setjmp.s ds3100.md/alloca.c ds3100.md/mon.c
HDRS		= ds3100.md/gmon.h
MDPUBHDRS	= 
OBJS		= ds3100.md/alloca.o ds3100.md/modf.o ds3100.md/mon.o ds3100.md/setjmp.o
CLEANOBJS	= ds3100.md/setjmp.o ds3100.md/alloca.o ds3100.md/mon.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= ds3100.md/modf.o
