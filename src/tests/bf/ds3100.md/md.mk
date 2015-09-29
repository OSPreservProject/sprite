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
# Tue Dec 15 22:09:56 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= bf.c ByteGet.c ByteSet.c ByteTest.c HalfwordGet.c HalfwordSet.c HalfwordTest.c WordGet.c WordSet.c WordTest.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/bf.o ds3100.md/ByteGet.o ds3100.md/ByteSet.o ds3100.md/ByteTest.o ds3100.md/HalfwordGet.o ds3100.md/HalfwordSet.o ds3100.md/HalfwordTest.o ds3100.md/WordGet.o ds3100.md/WordSet.o ds3100.md/WordTest.o
CLEANOBJS	= ds3100.md/bf.o ds3100.md/ByteGet.o ds3100.md/ByteSet.o ds3100.md/ByteTest.o ds3100.md/HalfwordGet.o ds3100.md/HalfwordSet.o ds3100.md/HalfwordTest.o ds3100.md/WordGet.o ds3100.md/WordSet.o ds3100.md/WordTest.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
