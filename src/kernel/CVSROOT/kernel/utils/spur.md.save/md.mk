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
# Thu Aug 23 16:05:50 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/dumpEvents.c dump.c hash.c trace.c
HDRS		= byte.h dump.h dumpInt.h hash.h trace.h
MDPUBHDRS	= 
OBJS		= spur.md/dumpEvents.o spur.md/dump.o spur.md/hash.o spur.md/trace.o
CLEANOBJS	= spur.md/dumpEvents.o spur.md/dump.o spur.md/hash.o spur.md/trace.o
INSTFILES	= spur.md/md.mk spur.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
