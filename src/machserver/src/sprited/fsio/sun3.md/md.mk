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
# Thu Nov 21 19:54:05 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fsioClientList.c fsioFile.c fsioLock.c fsioPipe.c fsioDevice.c fsioOps.c fsioStream.c fsioStreamOpTable.c fsioMigrate.c
HDRS		= fsio.h fsioDevice.h fsioFile.h fsioLock.h fsioPipe.h fsioRpc.h
MDPUBHDRS	= 
OBJS		= sun3.md/fsioClientList.o sun3.md/fsioFile.o sun3.md/fsioLock.o sun3.md/fsioOps.o sun3.md/fsioPipe.o sun3.md/fsioStream.o sun3.md/fsioStreamOpTable.o sun3.md/fsioDevice.o sun3.md/fsioMigrate.o
CLEANOBJS	= sun3.md/fsioClientList.o sun3.md/fsioFile.o sun3.md/fsioLock.o sun3.md/fsioPipe.o sun3.md/fsioDevice.o sun3.md/fsioOps.o sun3.md/fsioStream.o sun3.md/fsioStreamOpTable.o sun3.md/fsioMigrate.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
