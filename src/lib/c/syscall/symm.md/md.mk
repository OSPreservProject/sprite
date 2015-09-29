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
# Mon Jun  8 14:37:20 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= symm.md/schedStubs.s symm.md/fsStubs.s symm.md/netStubs.s symm.md/procStubs.s symm.md/profStubs.s symm.md/sigStubs.s symm.md/syncStubs.s symm.md/sysStubs.s symm.md/testStubs.s symm.md/vmStubs.s proc.c Fs_Read.c Fs_ReadVector.c Fs_Select.c Fs_Write.c Ioc_GetFlags.c Ioc_Lock.c Ioc_Reposition.c procEnviron.c Fs_Truncate.c Fs_WriteBack.c Fs_WriteVector.c Ioc_ClearBits.c Ioc_Map.c Ioc_SetBits.c Ioc_SetFlags.c Ioc_SetOwner.c Fs_IOControl.c Ioc_GetOwner.c Ioc_NumReadable.c Ioc_Truncate.c Ioc_Unlock.c Ioc_WriteBack.c Sig_Pause.c
HDRS		= symm.md/userSysCallInt.h
MDPUBHDRS	= 
OBJS		= symm.md/schedStubs.o symm.md/fsStubs.o symm.md/netStubs.o symm.md/procStubs.o symm.md/profStubs.o symm.md/sigStubs.o symm.md/syncStubs.o symm.md/sysStubs.o symm.md/testStubs.o symm.md/vmStubs.o symm.md/proc.o symm.md/Fs_Read.o symm.md/Fs_ReadVector.o symm.md/Fs_Select.o symm.md/Fs_Write.o symm.md/Ioc_GetFlags.o symm.md/Ioc_Lock.o symm.md/Ioc_Reposition.o symm.md/procEnviron.o symm.md/Fs_Truncate.o symm.md/Fs_WriteBack.o symm.md/Fs_WriteVector.o symm.md/Ioc_ClearBits.o symm.md/Ioc_Map.o symm.md/Ioc_SetBits.o symm.md/Ioc_SetFlags.o symm.md/Ioc_SetOwner.o symm.md/Fs_IOControl.o symm.md/Ioc_GetOwner.o symm.md/Ioc_NumReadable.o symm.md/Ioc_Truncate.o symm.md/Ioc_Unlock.o symm.md/Ioc_WriteBack.o symm.md/Sig_Pause.o
CLEANOBJS	= symm.md/schedStubs.o symm.md/fsStubs.o symm.md/netStubs.o symm.md/procStubs.o symm.md/profStubs.o symm.md/sigStubs.o symm.md/syncStubs.o symm.md/sysStubs.o symm.md/testStubs.o symm.md/vmStubs.o symm.md/proc.o symm.md/Fs_Read.o symm.md/Fs_ReadVector.o symm.md/Fs_Select.o symm.md/Fs_Write.o symm.md/Ioc_GetFlags.o symm.md/Ioc_Lock.o symm.md/Ioc_Reposition.o symm.md/procEnviron.o symm.md/Fs_Truncate.o symm.md/Fs_WriteBack.o symm.md/Fs_WriteVector.o symm.md/Ioc_ClearBits.o symm.md/Ioc_Map.o symm.md/Ioc_SetBits.o symm.md/Ioc_SetFlags.o symm.md/Ioc_SetOwner.o symm.md/Fs_IOControl.o symm.md/Ioc_GetOwner.o symm.md/Ioc_NumReadable.o symm.md/Ioc_Truncate.o symm.md/Ioc_Unlock.o symm.md/Ioc_WriteBack.o symm.md/Sig_Pause.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile
SACREDOBJS	= 
