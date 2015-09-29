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
# Tue Jul  2 14:18:48 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/dumpEvents.c dump.c hash.c trace.c traceLog.c
HDRS		= bf.h byte.h dump.h dumpInt.h hash.h sospRecord.h trace.h traceLog.h
MDPUBHDRS	= 
OBJS		= sun4c.md/dump.o sun4c.md/dumpEvents.o sun4c.md/hash.o sun4c.md/trace.o sun4c.md/traceLog.o
CLEANOBJS	= sun4c.md/dumpEvents.o sun4c.md/dump.o sun4c.md/hash.o sun4c.md/trace.o sun4c.md/traceLog.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
