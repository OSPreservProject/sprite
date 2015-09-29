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
# Fri Jun 14 16:12:36 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/start.s sun4.md/devFsOpTable.c sun4.md/vmSunAsm.s sun4.md/vm.c fileLoad.c machStubs.c main.c mem.c devConfig.c fs.c fsIndex.c stubs.c byte.c devSunProm.c fsDisk.c
HDRS		= sun4.md/machMon.h boot.h fsBoot.h fsDisk.h fsIndex.h
MDPUBHDRS	= 
OBJS		= sun4.md/byte.o sun4.md/devConfig.o sun4.md/devFsOpTable.o sun4.md/devSunProm.o sun4.md/fileLoad.o sun4.md/fs.o sun4.md/fsDisk.o sun4.md/fsIndex.o sun4.md/machStubs.o sun4.md/main.o sun4.md/mem.o sun4.md/start.o sun4.md/stubs.o sun4.md/vmSunAsm.o sun4.md/vm.o
CLEANOBJS	= sun4.md/start.o sun4.md/devFsOpTable.o sun4.md/vmSunAsm.o sun4.md/vm.o sun4.md/fileLoad.o sun4.md/machStubs.o sun4.md/main.o sun4.md/mem.o sun4.md/devConfig.o sun4.md/fs.o sun4.md/fsIndex.o sun4.md/stubs.o sun4.md/byte.o sun4.md/devSunProm.o sun4.md/fsDisk.o
INSTFILES	= sun4.md/md.mk Makefile local.mk tags
SACREDOBJS	= 
