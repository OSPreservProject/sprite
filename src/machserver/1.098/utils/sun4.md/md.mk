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
# Tue Jul  2 14:18:44 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/dumpEvents.c dump.c hash.c trace.c traceLog.c
HDRS		= bf.h byte.h dump.h dumpInt.h hash.h sospRecord.h trace.h traceLog.h
MDPUBHDRS	= 
OBJS		= sun4.md/dump.o sun4.md/dumpEvents.o sun4.md/hash.o sun4.md/trace.o sun4.md/traceLog.o
CLEANOBJS	= sun4.md/dumpEvents.o sun4.md/dump.o sun4.md/hash.o sun4.md/trace.o sun4.md/traceLog.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
