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
# Mon Sep 16 16:40:30 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= diskIO.c diskPrint.c diskFragIO.c diskHeader.c
HDRS		= disk.h
MDPUBHDRS	= 
OBJS		= sun3.md/diskIO.o sun3.md/diskPrint.o sun3.md/diskFragIO.o sun3.md/diskHeader.o
CLEANOBJS	= sun3.md/diskIO.o sun3.md/diskPrint.o sun3.md/diskFragIO.o sun3.md/diskHeader.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
