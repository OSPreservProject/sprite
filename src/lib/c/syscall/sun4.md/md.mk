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
# Mon Jun  8 14:37:12 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/fsStubs.s sun4.md/netStubs.s sun4.md/procStubs.s sun4.md/profStubs.s sun4.md/sigStubs.s sun4.md/syncStubs.s sun4.md/sysStubs.s sun4.md/testStubs.s sun4.md/vmStubs.s proc.c Fs_Read.c Fs_ReadVector.c Fs_Select.c Fs_Write.c Ioc_GetFlags.c Ioc_Lock.c Ioc_Reposition.c procEnviron.c Fs_Truncate.c Fs_WriteBack.c Fs_WriteVector.c Ioc_ClearBits.c Ioc_Map.c Ioc_SetBits.c Ioc_SetFlags.c Ioc_SetOwner.c Fs_IOControl.c Ioc_GetOwner.c Ioc_NumReadable.c Ioc_Truncate.c Ioc_Unlock.c Ioc_WriteBack.c Sig_Pause.c
HDRS		= sun4.md/userSysCallInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/fsStubs.o sun4.md/netStubs.o sun4.md/procStubs.o sun4.md/profStubs.o sun4.md/sigStubs.o sun4.md/syncStubs.o sun4.md/sysStubs.o sun4.md/testStubs.o sun4.md/vmStubs.o sun4.md/proc.o sun4.md/Fs_Read.o sun4.md/Fs_ReadVector.o sun4.md/Fs_Select.o sun4.md/Fs_Write.o sun4.md/Ioc_GetFlags.o sun4.md/Ioc_Lock.o sun4.md/Ioc_Reposition.o sun4.md/procEnviron.o sun4.md/Fs_Truncate.o sun4.md/Fs_WriteBack.o sun4.md/Fs_WriteVector.o sun4.md/Ioc_ClearBits.o sun4.md/Ioc_Map.o sun4.md/Ioc_SetBits.o sun4.md/Ioc_SetFlags.o sun4.md/Ioc_SetOwner.o sun4.md/Fs_IOControl.o sun4.md/Ioc_GetOwner.o sun4.md/Ioc_NumReadable.o sun4.md/Ioc_Truncate.o sun4.md/Ioc_Unlock.o sun4.md/Ioc_WriteBack.o sun4.md/Sig_Pause.o
CLEANOBJS	= sun4.md/fsStubs.o sun4.md/netStubs.o sun4.md/procStubs.o sun4.md/profStubs.o sun4.md/sigStubs.o sun4.md/syncStubs.o sun4.md/sysStubs.o sun4.md/testStubs.o sun4.md/vmStubs.o sun4.md/proc.o sun4.md/Fs_Read.o sun4.md/Fs_ReadVector.o sun4.md/Fs_Select.o sun4.md/Fs_Write.o sun4.md/Ioc_GetFlags.o sun4.md/Ioc_Lock.o sun4.md/Ioc_Reposition.o sun4.md/procEnviron.o sun4.md/Fs_Truncate.o sun4.md/Fs_WriteBack.o sun4.md/Fs_WriteVector.o sun4.md/Ioc_ClearBits.o sun4.md/Ioc_Map.o sun4.md/Ioc_SetBits.o sun4.md/Ioc_SetFlags.o sun4.md/Ioc_SetOwner.o sun4.md/Fs_IOControl.o sun4.md/Ioc_GetOwner.o sun4.md/Ioc_NumReadable.o sun4.md/Ioc_Truncate.o sun4.md/Ioc_Unlock.o sun4.md/Ioc_WriteBack.o sun4.md/Sig_Pause.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
