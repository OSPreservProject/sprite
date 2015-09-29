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
# Sun Nov 24 16:11:45 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sysCalls.c sysCode.c sysPrintf.c spriteSrvServer.c sysTestCall.c sysSysCall.c
HDRS		= spriteSrv.h sys.h sysInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/spriteSrvServer.o ds3100.md/sysCalls.o ds3100.md/sysCode.o ds3100.md/sysPrintf.o ds3100.md/sysSysCall.o ds3100.md/sysTestCall.o
CLEANOBJS	= ds3100.md/sysCalls.o ds3100.md/sysCode.o ds3100.md/sysPrintf.o ds3100.md/spriteSrvServer.o ds3100.md/sysTestCall.o ds3100.md/sysSysCall.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
