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
# Mon Dec 14 17:06:12 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= size.c print68k.c printSpur.c printMips.c
HDRS		= size.h
MDPUBHDRS	= 
OBJS		= ds3100.md/print68k.o ds3100.md/printMips.o ds3100.md/printSpur.o ds3100.md/size.o
CLEANOBJS	= ds3100.md/size.o ds3100.md/print68k.o ds3100.md/printSpur.o ds3100.md/printMips.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
