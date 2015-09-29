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
# Mon Jun  8 14:22:12 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/start.s sun4.md/setjmp.s sun4.md/gmon.c sun4.md/gstart.s sun4.md/gmcount.s
HDRS		= sun4.md/gmon.h
MDPUBHDRS	= 
OBJS		= sun4.md/start.o sun4.md/setjmp.o sun4.md/gmon.o sun4.md/gstart.o sun4.md/gmcount.o
CLEANOBJS	= sun4.md/start.o sun4.md/setjmp.o sun4.md/gmon.o sun4.md/gstart.o sun4.md/gmcount.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
