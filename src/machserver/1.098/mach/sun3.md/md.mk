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
# Tue Jul  2 12:27:59 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/machCode.c sun3.md/machIntr.s sun3.md/machAsm.s sun3.md/machMon.c sun3.md/machEeprom.c sun3.md/machMigrate.c sun3.md/bootSysAsm.s sun3.md/machVector.s sun3.md/machTrap.s
HDRS		= sun3.md/mach.h sun3.md/machAsmDefs.h sun3.md/machConst.h sun3.md/machEeprom.h sun3.md/machInt.h sun3.md/machMon.h sun3.md/machTypes.h
MDPUBHDRS	= sun3.md/mach.h sun3.md/machAsmDefs.h sun3.md/machConst.h sun3.md/machEeprom.h sun3.md/machMon.h sun3.md/machTypes.h
OBJS		= sun3.md/bootSysAsm.o sun3.md/machAsm.o sun3.md/machCode.o sun3.md/machEeprom.o sun3.md/machIntr.o sun3.md/machMigrate.o sun3.md/machMon.o sun3.md/machTrap.o sun3.md/machVector.o
CLEANOBJS	= sun3.md/machCode.o sun3.md/machIntr.o sun3.md/machAsm.o sun3.md/machMon.o sun3.md/machEeprom.o sun3.md/machMigrate.o sun3.md/bootSysAsm.o sun3.md/machVector.o sun3.md/machTrap.o
INSTFILES	= sun3.md/md.mk sun3.md/md.mk.sed sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
