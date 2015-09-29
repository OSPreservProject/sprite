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
# Mon Jun  8 14:21:04 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Bit_Expand.c Bit_FindFirstClear.c Bit_FindFirstSet.c Bit_Intersect.c Bit_Union.c Bit_AnySet.c
HDRS		= bitInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/Bit_Expand.o sun4.md/Bit_FindFirstClear.o sun4.md/Bit_FindFirstSet.o sun4.md/Bit_Intersect.o sun4.md/Bit_Union.o sun4.md/Bit_AnySet.o
CLEANOBJS	= sun4.md/Bit_Expand.o sun4.md/Bit_FindFirstClear.o sun4.md/Bit_FindFirstSet.o sun4.md/Bit_Intersect.o sun4.md/Bit_Union.o sun4.md/Bit_AnySet.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
