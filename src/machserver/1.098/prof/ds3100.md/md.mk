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
# Tue Jul  2 14:00:56 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/_mcount.c ds3100.md/profStack.s ds3100.md/profSubr.c profMigrate.c profProfil.c
HDRS		= ds3100.md/profInt.h prof.h
MDPUBHDRS	= 
OBJS		= ds3100.md/_mcount.o ds3100.md/profMigrate.o ds3100.md/profProfil.o ds3100.md/profStack.o ds3100.md/profSubr.o
CLEANOBJS	= ds3100.md/_mcount.o ds3100.md/profStack.o ds3100.md/profSubr.o ds3100.md/profMigrate.o ds3100.md/profProfil.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
