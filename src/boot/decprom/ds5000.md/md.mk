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
# Fri Aug 30 18:39:28 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/main.c ds5000.md/start.s ds5000.md/devFsOpTable.c ds5000.md/devDecProm.c fs.c byte.c fileLoad.c fsDisk.c fsIndex.c machStubs.c mem.c
HDRS		= boot.h fsBoot.h fsIndex.h
MDPUBHDRS	= 
OBJS		= ds5000.md/byte.o ds5000.md/devDecProm.o ds5000.md/devFsOpTable.o ds5000.md/fileLoad.o ds5000.md/fs.o ds5000.md/fsDisk.o ds5000.md/fsIndex.o ds5000.md/machStubs.o ds5000.md/main.o ds5000.md/mem.o ds5000.md/start.o
CLEANOBJS	= ds5000.md/main.o ds5000.md/start.o ds5000.md/devFsOpTable.o ds5000.md/devDecProm.o ds5000.md/fs.o ds5000.md/byte.o ds5000.md/fileLoad.o ds5000.md/fsDisk.o ds5000.md/fsIndex.o ds5000.md/machStubs.o ds5000.md/mem.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile tags
SACREDOBJS	= 
