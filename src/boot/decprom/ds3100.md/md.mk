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
# Mon Feb 26 04:45:45 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.5 90/02/05 13:31:23 rab Exp $
#
# Allow mkmf

SRCS		= ds3100.md/devFsOpTable.c ds3100.md/start.s byte.c devDecProm.c fileLoad.c fs.c fsDisk.c fsIndex.c machStubs.c main.c mem.c
HDRS		= boot.h fsBoot.h fsIndex.h
MDPUBHDRS	= 
OBJS		= ds3100.md/byte.o ds3100.md/devDecProm.o ds3100.md/devFsOpTable.o ds3100.md/fileLoad.o ds3100.md/fs.o ds3100.md/fsDisk.o ds3100.md/fsIndex.o ds3100.md/machStubs.o ds3100.md/main.o ds3100.md/mem.o ds3100.md/start.o
CLEANOBJS	= ds3100.md/devFsOpTable.o ds3100.md/start.o ds3100.md/byte.o ds3100.md/devDecProm.o ds3100.md/fileLoad.o ds3100.md/fs.o ds3100.md/fsDisk.o ds3100.md/fsIndex.o ds3100.md/machStubs.o ds3100.md/main.o ds3100.md/mem.o
