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
# Tue Nov 19 19:15:03 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/fsMach.c fsInit.c fsNameOps.c fsOpTable.c fsSelect.c fsSysCall.c fsAttributes.c fsCommand.c fsPageOps.c fsStreamOps.c fsTopStream.c fsTopMigrate.c
HDRS		= sun3.md/fsMach.h fs.h fsNameOps.h fsStat.h fsUnixStubs.h
MDPUBHDRS	= sun3.md/fsMach.h
OBJS		= sun3.md/fsAttributes.o sun3.md/fsCommand.o sun3.md/fsInit.o sun3.md/fsMach.o sun3.md/fsNameOps.o sun3.md/fsOpTable.o sun3.md/fsPageOps.o sun3.md/fsSelect.o sun3.md/fsStreamOps.o sun3.md/fsSysCall.o sun3.md/fsTopStream.o sun3.md/fsTopMigrate.o
CLEANOBJS	= sun3.md/fsMach.o sun3.md/fsInit.o sun3.md/fsNameOps.o sun3.md/fsOpTable.o sun3.md/fsSelect.o sun3.md/fsSysCall.o sun3.md/fsAttributes.o sun3.md/fsCommand.o sun3.md/fsPageOps.o sun3.md/fsStreamOps.o sun3.md/fsTopStream.o sun3.md/fsTopMigrate.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
