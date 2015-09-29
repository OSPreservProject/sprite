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
# Mon Aug 12 14:36:01 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/start.s sun4c.md/devFsOpTable.c sun4c.md/devSunProm.c byte.c devConfig.c fileLoad.c fsDisk.c fs.c fsIndex.c main.c mem.c stubs.c
HDRS		= boot.h fsBoot.h fsIndex.h
MDPUBHDRS	= 
OBJS		= sun4c.md/start.o sun4c.md/devFsOpTable.o sun4c.md/devSunProm.o sun4c.md/byte.o sun4c.md/devConfig.o sun4c.md/fileLoad.o sun4c.md/fsDisk.o sun4c.md/fs.o sun4c.md/fsIndex.o sun4c.md/main.o sun4c.md/mem.o sun4c.md/stubs.o
CLEANOBJS	= sun4c.md/start.o sun4c.md/devFsOpTable.o sun4c.md/devSunProm.o sun4c.md/byte.o sun4c.md/devConfig.o sun4c.md/fileLoad.o sun4c.md/fsDisk.o sun4c.md/fs.o sun4c.md/fsIndex.o sun4c.md/main.o sun4c.md/mem.o sun4c.md/stubs.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
