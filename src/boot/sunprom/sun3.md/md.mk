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
# Fri Jun 14 16:12:25 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/devFsOpTable.c sun3.md/vm.c sun3.md/start.s sun3.md/vmSunAsm.s fileLoad.c machStubs.c main.c mem.c devConfig.c fs.c fsIndex.c stubs.c byte.c devSunProm.c fsDisk.c
HDRS		= sun3.md/machMon.h sun3.md/vmMachInt.h boot.h fsBoot.h fsDisk.h fsIndex.h
MDPUBHDRS	= 
OBJS		= sun3.md/byte.o sun3.md/devConfig.o sun3.md/devFsOpTable.o sun3.md/devSunProm.o sun3.md/fileLoad.o sun3.md/fs.o sun3.md/fsDisk.o sun3.md/fsIndex.o sun3.md/machStubs.o sun3.md/main.o sun3.md/mem.o sun3.md/start.o sun3.md/stubs.o sun3.md/vm.o sun3.md/vmSunAsm.o
CLEANOBJS	= sun3.md/devFsOpTable.o sun3.md/vm.o sun3.md/start.o sun3.md/vmSunAsm.o sun3.md/fileLoad.o sun3.md/machStubs.o sun3.md/main.o sun3.md/mem.o sun3.md/devConfig.o sun3.md/fs.o sun3.md/fsIndex.o sun3.md/stubs.o sun3.md/byte.o sun3.md/devSunProm.o sun3.md/fsDisk.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
