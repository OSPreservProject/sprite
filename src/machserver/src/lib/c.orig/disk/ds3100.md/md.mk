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
# Sun Feb 25 17:42:38 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.5 90/02/05 13:31:23 rab Exp $
#
# Allow mkmf

SRCS		= diskFragIO.c diskHeader.c diskIO.c diskPrint.c
HDRS		= disk.h
MDPUBHDRS	= 
OBJS		= ds3100.md/diskFragIO.o ds3100.md/diskHeader.o ds3100.md/diskIO.o ds3100.md/diskPrint.o
CLEANOBJS	= ds3100.md/diskFragIO.o ds3100.md/diskHeader.o ds3100.md/diskIO.o ds3100.md/diskPrint.o
