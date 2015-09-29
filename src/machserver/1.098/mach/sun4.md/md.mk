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
# Tue Jul  2 12:28:06 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/machTrap.s sun4.md/fpu_simulator.c sun4.md/machCode.c sun4.md/machMon.c sun4.md/machIntr.s sun4.md/compare.c sun4.md/div.c sun4.md/addsub.c sun4.md/iu_simulator.c sun4.md/mul.c sun4.md/pack.c sun4.md/unpack.c sun4.md/utility.c sun4.md/uword.c sun4.md/bootSysAsm.s sun4.md/machTrace.s sun4.md/machMigrate.c sun4.md/machAsm.s sun4.md/machFPUGlue.c
HDRS		= sun4.md/fpu_simulator.h sun4.md/globals.h sun4.md/ieee.h sun4.md/mach.h sun4.md/machAsmDefs.h sun4.md/machConst.h sun4.md/machInt.h sun4.md/machMon.h sun4.md/machSig.h sun4.md/machTypes.h
MDPUBHDRS	= sun4.md/mach.h sun4.md/machAsmDefs.h sun4.md/machConst.h sun4.md/machMon.h sun4.md/machSig.h sun4.md/machTypes.h
OBJS		= sun4.md/bootSysAsm.o sun4.md/addsub.o sun4.md/compare.o sun4.md/div.o sun4.md/fpu_simulator.o sun4.md/iu_simulator.o sun4.md/machAsm.o sun4.md/machCode.o sun4.md/machFPUGlue.o sun4.md/machIntr.o sun4.md/machMigrate.o sun4.md/machMon.o sun4.md/machTrace.o sun4.md/machTrap.o sun4.md/mul.o sun4.md/pack.o sun4.md/unpack.o sun4.md/utility.o sun4.md/uword.o
CLEANOBJS	= sun4.md/machTrap.o sun4.md/fpu_simulator.o sun4.md/machCode.o sun4.md/machMon.o sun4.md/machIntr.o sun4.md/compare.o sun4.md/div.o sun4.md/addsub.o sun4.md/iu_simulator.o sun4.md/mul.o sun4.md/pack.o sun4.md/unpack.o sun4.md/utility.o sun4.md/uword.o sun4.md/bootSysAsm.o sun4.md/machTrace.o sun4.md/machMigrate.o sun4.md/machAsm.o sun4.md/machFPUGlue.o
INSTFILES	= sun4.md/md.mk sun4.md/md.mk.sed sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
