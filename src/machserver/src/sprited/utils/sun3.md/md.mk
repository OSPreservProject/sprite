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
# Fri Oct 25 22:27:30 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= utilsMisc.c hash.c trace.c mapStatus.c
HDRS		= sun3.md/utilsMach.h bf.h byte.h dump.h hash.h trace.h utils.h
MDPUBHDRS	= sun3.md/utilsMach.h
OBJS		= sun3.md/hash.o sun3.md/mapStatus.o sun3.md/utilsMisc.o sun3.md/trace.o
CLEANOBJS	= sun3.md/utilsMisc.o sun3.md/hash.o sun3.md/trace.o sun3.md/mapStatus.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
