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
# Tue Oct 24 00:30:01 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= DbLockDesc.c Db_Close.c Db_Get.c Db_Open.c Db_Put.c Db_ReadEntry.c Db_WriteEntry.c
HDRS		= dbInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/DbLockDesc.o sun3.md/Db_Close.o sun3.md/Db_Get.o sun3.md/Db_Open.o sun3.md/Db_Put.o sun3.md/Db_ReadEntry.o sun3.md/Db_WriteEntry.o
CLEANOBJS	= sun3.md/DbLockDesc.o sun3.md/Db_Close.o sun3.md/Db_Get.o sun3.md/Db_Open.o sun3.md/Db_Put.o sun3.md/Db_ReadEntry.o sun3.md/Db_WriteEntry.o
DISTDIR        ?= @(DISTDIR)
