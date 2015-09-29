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
# Mon Jun  8 14:36:54 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/fsStubs.s ds3100.md/netStubs.s ds3100.md/procStubs.s ds3100.md/profStubs.s ds3100.md/sigStubs.s ds3100.md/syncStubs.s ds3100.md/sysStubs.s ds3100.md/testStubs.s ds3100.md/vmStubs.s ds3100.md/zebra.s proc.c Fs_Read.c Fs_ReadVector.c Fs_Select.c Fs_Write.c Ioc_GetFlags.c Ioc_Lock.c Ioc_Reposition.c procEnviron.c Fs_Truncate.c Fs_WriteBack.c Fs_WriteVector.c Ioc_ClearBits.c Ioc_Map.c Ioc_SetBits.c Ioc_SetFlags.c Ioc_SetOwner.c Fs_IOControl.c Ioc_GetOwner.c Ioc_NumReadable.c Ioc_Truncate.c Ioc_Unlock.c Ioc_WriteBack.c Sig_Pause.c
HDRS		= ds3100.md/userSysCallInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Fs_IOControl.o ds3100.md/Fs_Read.o ds3100.md/Fs_ReadVector.o ds3100.md/Fs_Select.o ds3100.md/Fs_Truncate.o ds3100.md/Fs_Write.o ds3100.md/Fs_WriteBack.o ds3100.md/Fs_WriteVector.o ds3100.md/Ioc_ClearBits.o ds3100.md/Ioc_GetFlags.o ds3100.md/Ioc_GetOwner.o ds3100.md/Ioc_Lock.o ds3100.md/Ioc_Map.o ds3100.md/Ioc_NumReadable.o ds3100.md/Ioc_Reposition.o ds3100.md/Ioc_SetBits.o ds3100.md/Ioc_SetFlags.o ds3100.md/Ioc_SetOwner.o ds3100.md/Ioc_Truncate.o ds3100.md/Ioc_Unlock.o ds3100.md/Ioc_WriteBack.o ds3100.md/Sig_Pause.o ds3100.md/fsStubs.o ds3100.md/netStubs.o ds3100.md/proc.o ds3100.md/procEnviron.o ds3100.md/procStubs.o ds3100.md/profStubs.o ds3100.md/sigStubs.o ds3100.md/syncStubs.o ds3100.md/sysStubs.o ds3100.md/testStubs.o ds3100.md/vmStubs.o ds3100.md/zebra.o
CLEANOBJS	= ds3100.md/fsStubs.o ds3100.md/netStubs.o ds3100.md/procStubs.o ds3100.md/profStubs.o ds3100.md/sigStubs.o ds3100.md/syncStubs.o ds3100.md/sysStubs.o ds3100.md/testStubs.o ds3100.md/vmStubs.o ds3100.md/zebra.o ds3100.md/proc.o ds3100.md/Fs_Read.o ds3100.md/Fs_ReadVector.o ds3100.md/Fs_Select.o ds3100.md/Fs_Write.o ds3100.md/Ioc_GetFlags.o ds3100.md/Ioc_Lock.o ds3100.md/Ioc_Reposition.o ds3100.md/procEnviron.o ds3100.md/Fs_Truncate.o ds3100.md/Fs_WriteBack.o ds3100.md/Fs_WriteVector.o ds3100.md/Ioc_ClearBits.o ds3100.md/Ioc_Map.o ds3100.md/Ioc_SetBits.o ds3100.md/Ioc_SetFlags.o ds3100.md/Ioc_SetOwner.o ds3100.md/Fs_IOControl.o ds3100.md/Ioc_GetOwner.o ds3100.md/Ioc_NumReadable.o ds3100.md/Ioc_Truncate.o ds3100.md/Ioc_Unlock.o ds3100.md/Ioc_WriteBack.o ds3100.md/Sig_Pause.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
