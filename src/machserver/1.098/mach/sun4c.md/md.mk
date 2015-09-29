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
# Tue Jul  2 12:28:12 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/addsub.c sun4c.md/bootSysAsm.s sun4c.md/compare.c sun4c.md/div.c sun4c.md/fpu_simulator.c sun4c.md/iu_simulator.c sun4c.md/machAsm.s sun4c.md/machCode.c sun4c.md/machFPUGlue.c sun4c.md/machIntr.s sun4c.md/machMon.c sun4c.md/machTrap.s sun4c.md/mul.c sun4c.md/pack.c sun4c.md/unpack.c sun4c.md/utility.c sun4c.md/uword.c sun4c.md/machMigrate.c
HDRS		= sun4c.md/fpu_simulator.h sun4c.md/globals.h sun4c.md/ieee.h sun4c.md/mach.h sun4c.md/machAsmDefs.h sun4c.md/machConst.h sun4c.md/machInt.h sun4c.md/machMon.h sun4c.md/machSig.h sun4c.md/machTypes.h
MDPUBHDRS	= sun4c.md/mach.h sun4c.md/machAsmDefs.h sun4c.md/machConst.h sun4c.md/machMon.h sun4c.md/machSig.h sun4c.md/machTypes.h
OBJS		= sun4c.md/bootSysAsm.o sun4c.md/addsub.o sun4c.md/compare.o sun4c.md/div.o sun4c.md/fpu_simulator.o sun4c.md/iu_simulator.o sun4c.md/machAsm.o sun4c.md/machCode.o sun4c.md/machFPUGlue.o sun4c.md/machIntr.o sun4c.md/machMigrate.o sun4c.md/machMon.o sun4c.md/machTrap.o sun4c.md/mul.o sun4c.md/pack.o sun4c.md/unpack.o sun4c.md/utility.o sun4c.md/uword.o
CLEANOBJS	= sun4c.md/addsub.o sun4c.md/bootSysAsm.o sun4c.md/compare.o sun4c.md/div.o sun4c.md/fpu_simulator.o sun4c.md/iu_simulator.o sun4c.md/machAsm.o sun4c.md/machCode.o sun4c.md/machFPUGlue.o sun4c.md/machIntr.o sun4c.md/machMon.o sun4c.md/machTrap.o sun4c.md/mul.o sun4c.md/pack.o sun4c.md/unpack.o sun4c.md/utility.o sun4c.md/uword.o sun4c.md/machMigrate.o
INSTFILES	= sun4c.md/md.mk sun4c.md/md.mk.sed sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
