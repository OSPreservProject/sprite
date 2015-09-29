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
# Tue Jul 31 17:01:54 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= genind.c mkind.c qsort.c scanid.c scanst.c sortid.c
HDRS		= genind.h mkind.h scanid.h scanst.h
MDPUBHDRS	= 
OBJS		= sun3.md/genind.o sun3.md/mkind.o sun3.md/qsort.o sun3.md/scanid.o sun3.md/scanst.o sun3.md/sortid.o
CLEANOBJS	= sun3.md/genind.o sun3.md/mkind.o sun3.md/qsort.o sun3.md/scanid.o sun3.md/scanst.o sun3.md/sortid.o
INSTFILES	= Makefile
SACREDOBJS	= 
