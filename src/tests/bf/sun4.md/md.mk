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
# Tue Dec 15 22:10:01 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= bf.c ByteGet.c ByteSet.c ByteTest.c HalfwordGet.c HalfwordSet.c HalfwordTest.c WordGet.c WordSet.c WordTest.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/bf.o sun4.md/ByteGet.o sun4.md/ByteSet.o sun4.md/ByteTest.o sun4.md/HalfwordGet.o sun4.md/HalfwordSet.o sun4.md/HalfwordTest.o sun4.md/WordGet.o sun4.md/WordSet.o sun4.md/WordTest.o
CLEANOBJS	= sun4.md/bf.o sun4.md/ByteGet.o sun4.md/ByteSet.o sun4.md/ByteTest.o sun4.md/HalfwordGet.o sun4.md/HalfwordSet.o sun4.md/HalfwordTest.o sun4.md/WordGet.o sun4.md/WordSet.o sun4.md/WordTest.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
