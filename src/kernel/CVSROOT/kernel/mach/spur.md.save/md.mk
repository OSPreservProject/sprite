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
# Thu Aug 23 12:53:56 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/loMem.s spur.md/machCCRegs.s spur.md/machCPC.c spur.md/machCode.c spur.md/machConfig.c spur.md/machInitMemory.s spur.md/machMigrate.c spur.md/machMon.c spur.md/machPhys.s spur.md/machRefresh.c
HDRS		= spur.md/devCC.h spur.md/mach.h spur.md/machAsmDefs.h spur.md/machCCRegs.h spur.md/machConfig.h spur.md/machConst.h spur.md/machInt.h spur.md/machMon.h spur.md/reg.h spur.md/spurMem.h machSymAssym.h
MDPUBHDRS	= spur.md/mach.h spur.md/machAsmDefs.h spur.md/machCCRegs.h spur.md/machConfig.h spur.md/machConst.h spur.md/machMon.h
OBJS		= spur.md/loMem.o spur.md/machCCRegs.o spur.md/machCPC.o spur.md/machCode.o spur.md/machConfig.o spur.md/machInitMemory.o spur.md/machMigrate.o spur.md/machMon.o spur.md/machPhys.o spur.md/machRefresh.o
CLEANOBJS	= spur.md/loMem.o spur.md/machCCRegs.o spur.md/machCPC.o spur.md/machCode.o spur.md/machConfig.o spur.md/machInitMemory.o spur.md/machMigrate.o spur.md/machMon.o spur.md/machPhys.o spur.md/machRefresh.o
INSTFILES	= spur.md/md.mk spur.md/md.mk.sed spur.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
