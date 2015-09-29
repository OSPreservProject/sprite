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
# Thu Nov 21 20:04:05 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fsrmtDomain.c fsrmtFile.c fsrmtIO.c fsrmtOps.c fsrmtPipe.c fsrmtAttributes.c fsrmtDevice.c
HDRS		= fsrmt.h fsrmtDomain.h fsrmtInt.h fsrmtNameOpsInt.h fsrmtRpcStubs.h
MDPUBHDRS	= 
OBJS		= sun3.md/fsrmtAttributes.o sun3.md/fsrmtDomain.o sun3.md/fsrmtFile.o sun3.md/fsrmtIO.o sun3.md/fsrmtOps.o sun3.md/fsrmtPipe.o sun3.md/fsrmtDevice.o
CLEANOBJS	= sun3.md/fsrmtDomain.o sun3.md/fsrmtFile.o sun3.md/fsrmtIO.o sun3.md/fsrmtOps.o sun3.md/fsrmtPipe.o sun3.md/fsrmtAttributes.o sun3.md/fsrmtDevice.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
