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
# Tue Jul  2 12:27:48 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/machDis.c ds3100.md/loMem.s ds3100.md/machMigrate.c ds3100.md/machAsm.s ds3100.md/socket.c ds3100.md/ioctl.c ds3100.md/machCode.c ds3100.md/compatSig.c ds3100.md/cvtStat.c ds3100.md/fcntl.c ds3100.md/signals.c ds3100.md/compatMapCode.c ds3100.md/machMon.c ds3100.md/machUNIXSyscall.c
HDRS		= ds3100.md/compatSig.h ds3100.md/mach.h ds3100.md/machAddrs.h ds3100.md/machAsmDefs.h ds3100.md/machConst.h ds3100.md/machInt.h ds3100.md/machMon.h ds3100.md/machTypes.h ds3100.md/stat.h ds3100.md/sysinfo.h ds3100.md/ultrixSignal.h
MDPUBHDRS	= ds3100.md/mach.h ds3100.md/machAddrs.h ds3100.md/machAsmDefs.h ds3100.md/machConst.h ds3100.md/machMon.h ds3100.md/machTypes.h
OBJS		= ds3100.md/loMem.o ds3100.md/compatMapCode.o ds3100.md/compatSig.o ds3100.md/cvtStat.o ds3100.md/fcntl.o ds3100.md/ioctl.o ds3100.md/machAsm.o ds3100.md/machCode.o ds3100.md/machDis.o ds3100.md/machMigrate.o ds3100.md/machMon.o ds3100.md/machUNIXSyscall.o ds3100.md/signals.o ds3100.md/socket.o ds3100.md/softfp.o
CLEANOBJS	= ds3100.md/machDis.o ds3100.md/loMem.o ds3100.md/machMigrate.o ds3100.md/machAsm.o ds3100.md/socket.o ds3100.md/ioctl.o ds3100.md/machCode.o ds3100.md/compatSig.o ds3100.md/cvtStat.o ds3100.md/fcntl.o ds3100.md/signals.o ds3100.md/compatMapCode.o ds3100.md/machMon.o ds3100.md/machUNIXSyscall.o
INSTFILES	= ds3100.md/md.mk ds3100.md/md.mk.sed ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= ds3100.md/softfp.o
