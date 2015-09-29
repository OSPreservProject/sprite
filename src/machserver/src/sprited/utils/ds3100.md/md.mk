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
# Sat Nov  2 22:43:15 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/utilsMachAsm.s utilsMisc.c hash.c trace.c mapStatus.c
HDRS		= ds3100.md/utilsMach.h bf.h byte.h dump.h hash.h trace.h utils.h
MDPUBHDRS	= ds3100.md/utilsMach.h
OBJS		= ds3100.md/utilsMachAsm.o ds3100.md/utilsMisc.o ds3100.md/hash.o ds3100.md/trace.o ds3100.md/mapStatus.o
CLEANOBJS	= ds3100.md/utilsMachAsm.o ds3100.md/utilsMisc.o ds3100.md/hash.o ds3100.md/trace.o ds3100.md/mapStatus.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
