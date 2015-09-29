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
# Thu Nov 21 20:32:11 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fsioClientList.c fsioFile.c fsioLock.c fsioPipe.c fsioDevice.c fsioOps.c fsioStream.c fsioStreamOpTable.c fsioMigrate.c
HDRS		= fsio.h fsioDevice.h fsioFile.h fsioLock.h fsioPipe.h fsioRpc.h
MDPUBHDRS	= 
OBJS		= ds3100.md/fsioClientList.o ds3100.md/fsioFile.o ds3100.md/fsioLock.o ds3100.md/fsioPipe.o ds3100.md/fsioDevice.o ds3100.md/fsioOps.o ds3100.md/fsioStream.o ds3100.md/fsioStreamOpTable.o ds3100.md/fsioMigrate.o
CLEANOBJS	= ds3100.md/fsioClientList.o ds3100.md/fsioFile.o ds3100.md/fsioLock.o ds3100.md/fsioPipe.o ds3100.md/fsioDevice.o ds3100.md/fsioOps.o ds3100.md/fsioStream.o ds3100.md/fsioStreamOpTable.o ds3100.md/fsioMigrate.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
