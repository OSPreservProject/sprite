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
# Thu Nov 21 20:30:29 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fsrmtDomain.c fsrmtFile.c fsrmtIO.c fsrmtOps.c fsrmtPipe.c fsrmtAttributes.c fsrmtDevice.c
HDRS		= fsrmt.h fsrmtDomain.h fsrmtInt.h fsrmtNameOpsInt.h fsrmtRpcStubs.h
MDPUBHDRS	= 
OBJS		= ds3100.md/fsrmtDomain.o ds3100.md/fsrmtFile.o ds3100.md/fsrmtIO.o ds3100.md/fsrmtOps.o ds3100.md/fsrmtPipe.o ds3100.md/fsrmtAttributes.o ds3100.md/fsrmtDevice.o
CLEANOBJS	= ds3100.md/fsrmtDomain.o ds3100.md/fsrmtFile.o ds3100.md/fsrmtIO.o ds3100.md/fsrmtOps.o ds3100.md/fsrmtPipe.o ds3100.md/fsrmtAttributes.o ds3100.md/fsrmtDevice.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
