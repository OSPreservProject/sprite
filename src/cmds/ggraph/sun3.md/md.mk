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
# Thu Aug 24 13:05:58 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.3 88/06/06 17:23:47 ouster Exp $
#
# Allow mkmf

SRCS		= axis.c commands.c ggraph.c ggraphdata.c hist.c legend.c points.c set.c symbols.c
HDRS		= commands.h ggraph.h ggraphdefs.h ggraphstruct.h
MDPUBHDRS	= 
OBJS		= sun3.md/axis.o sun3.md/commands.o sun3.md/ggraph.o sun3.md/ggraphdata.o sun3.md/hist.o sun3.md/legend.o sun3.md/points.o sun3.md/set.o sun3.md/symbols.o
CLEANOBJS	= sun3.md/axis.o sun3.md/commands.o sun3.md/ggraph.o sun3.md/ggraphdata.o sun3.md/hist.o sun3.md/legend.o sun3.md/points.o sun3.md/set.o sun3.md/symbols.o
