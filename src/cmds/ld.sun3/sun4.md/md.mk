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
# Fri Oct 26 00:57:48 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cplus-dem.c getopt.c getopt1.c ld.c
HDRS		= assert.h getopt.h symseg.h
MDPUBHDRS	= 
OBJS		= sun4.md/ld.o sun4.md/cplus-dem.o sun4.md/getopt.o sun4.md/getopt1.o
CLEANOBJS	= sun4.md/cplus-dem.o sun4.md/getopt.o sun4.md/getopt1.o sun4.md/ld.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
