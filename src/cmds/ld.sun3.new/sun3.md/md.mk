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
# Thu Dec  5 18:59:14 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cplus-dem.c getopt.c ld.c getopt1.c
HDRS		= getopt.h symseg.h
MDPUBHDRS	= 
OBJS		= sun3.md/cplus-dem.o sun3.md/getopt.o sun3.md/ld.o sun3.md/getopt1.o
CLEANOBJS	= sun3.md/cplus-dem.o sun3.md/getopt.o sun3.md/ld.o sun3.md/getopt1.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
