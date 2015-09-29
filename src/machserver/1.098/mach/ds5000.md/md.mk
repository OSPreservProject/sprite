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
# Tue Jul  2 12:27:54 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/compatMapCode.c ds5000.md/compatSig.c ds5000.md/cvtStat.c ds5000.md/fcntl.c ds5000.md/ioctl.c ds5000.md/loMem.s ds5000.md/machAsm.s ds5000.md/machCode.c ds5000.md/machDis.c ds5000.md/machMigrate.c ds5000.md/machMon.c ds5000.md/machUNIXSyscall.c ds5000.md/signals.c ds5000.md/socket.c
HDRS		= ds5000.md/compatSig.h ds5000.md/mach.h ds5000.md/machAddrs.h ds5000.md/machAsmDefs.h ds5000.md/machConst.h ds5000.md/machInt.h ds5000.md/machMon.h ds5000.md/machTypes.h ds5000.md/stat.h ds5000.md/sysinfo.h ds5000.md/ultrixSignal.h
MDPUBHDRS	= ds5000.md/mach.h ds5000.md/machAddrs.h ds5000.md/machAsmDefs.h ds5000.md/machConst.h ds5000.md/machMon.h ds5000.md/machTypes.h
OBJS		= ds5000.md/loMem.o ds5000.md/compatMapCode.o ds5000.md/compatSig.o ds5000.md/cvtStat.o ds5000.md/fcntl.o ds5000.md/ioctl.o ds5000.md/machAsm.o ds5000.md/machCode.o ds5000.md/machDis.o ds5000.md/machMigrate.o ds5000.md/machMon.o ds5000.md/machUNIXSyscall.o ds5000.md/signals.o ds5000.md/socket.o ds5000.md/softfp.o
CLEANOBJS	= ds5000.md/compatMapCode.o ds5000.md/compatSig.o ds5000.md/cvtStat.o ds5000.md/fcntl.o ds5000.md/ioctl.o ds5000.md/loMem.o ds5000.md/machAsm.o ds5000.md/machCode.o ds5000.md/machDis.o ds5000.md/machMigrate.o ds5000.md/machMon.o ds5000.md/machUNIXSyscall.o ds5000.md/signals.o ds5000.md/socket.o
INSTFILES	= ds5000.md/md.mk ds5000.md/md.mk.sed ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= ds5000.md/softfp.o
