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
# Tue Oct 24 00:29:51 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= DbLockDesc.c Db_Close.c Db_Get.c Db_Open.c Db_Put.c Db_ReadEntry.c Db_WriteEntry.c
HDRS		= dbInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/DbLockDesc.o ds3100.md/Db_Close.o ds3100.md/Db_Get.o ds3100.md/Db_Open.o ds3100.md/Db_Put.o ds3100.md/Db_ReadEntry.o ds3100.md/Db_WriteEntry.o
CLEANOBJS	= ds3100.md/DbLockDesc.o ds3100.md/Db_Close.o ds3100.md/Db_Get.o ds3100.md/Db_Open.o ds3100.md/Db_Put.o ds3100.md/Db_ReadEntry.o ds3100.md/Db_WriteEntry.o
DISTDIR        ?= @(DISTDIR)
