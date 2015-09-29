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
# Tue Oct 24 00:43:15 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= sun3.md/fsStubs.s sun3.md/netStubs.s sun3.md/procStubs.s sun3.md/profStubs.s sun3.md/sigStubs.s sun3.md/syncStubs.s sun3.md/sysStubs.s sun3.md/testStubs.s sun3.md/vmStubs.s Fs_IOControl.c Fs_Read.c Fs_ReadVector.c Fs_Select.c Fs_Truncate.c Fs_Write.c Fs_WriteBack.c Fs_WriteVector.c Ioc_ClearBits.c Ioc_GetFlags.c Ioc_GetOwner.c Ioc_Lock.c Ioc_Map.c Ioc_NumReadable.c Ioc_Reposition.c Ioc_SetBits.c Ioc_SetFlags.c Ioc_SetOwner.c Ioc_Truncate.c Ioc_Unlock.c Ioc_WriteBack.c Sig_Pause.c proc.c procEnviron.c
HDRS		= sun3.md/userSysCallInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/fsStubs.o sun3.md/netStubs.o sun3.md/procStubs.o sun3.md/profStubs.o sun3.md/sigStubs.o sun3.md/syncStubs.o sun3.md/sysStubs.o sun3.md/testStubs.o sun3.md/vmStubs.o sun3.md/Fs_IOControl.o sun3.md/Fs_Read.o sun3.md/Fs_ReadVector.o sun3.md/Fs_Select.o sun3.md/Fs_Truncate.o sun3.md/Fs_Write.o sun3.md/Fs_WriteBack.o sun3.md/Fs_WriteVector.o sun3.md/Ioc_ClearBits.o sun3.md/Ioc_GetFlags.o sun3.md/Ioc_GetOwner.o sun3.md/Ioc_Lock.o sun3.md/Ioc_Map.o sun3.md/Ioc_NumReadable.o sun3.md/Ioc_Reposition.o sun3.md/Ioc_SetBits.o sun3.md/Ioc_SetFlags.o sun3.md/Ioc_SetOwner.o sun3.md/Ioc_Truncate.o sun3.md/Ioc_Unlock.o sun3.md/Ioc_WriteBack.o sun3.md/Sig_Pause.o sun3.md/proc.o sun3.md/procEnviron.o
CLEANOBJS	= sun3.md/fsStubs.o sun3.md/netStubs.o sun3.md/procStubs.o sun3.md/profStubs.o sun3.md/sigStubs.o sun3.md/syncStubs.o sun3.md/sysStubs.o sun3.md/testStubs.o sun3.md/vmStubs.o sun3.md/Fs_IOControl.o sun3.md/Fs_Read.o sun3.md/Fs_ReadVector.o sun3.md/Fs_Select.o sun3.md/Fs_Truncate.o sun3.md/Fs_Write.o sun3.md/Fs_WriteBack.o sun3.md/Fs_WriteVector.o sun3.md/Ioc_ClearBits.o sun3.md/Ioc_GetFlags.o sun3.md/Ioc_GetOwner.o sun3.md/Ioc_Lock.o sun3.md/Ioc_Map.o sun3.md/Ioc_NumReadable.o sun3.md/Ioc_Reposition.o sun3.md/Ioc_SetBits.o sun3.md/Ioc_SetFlags.o sun3.md/Ioc_SetOwner.o sun3.md/Ioc_Truncate.o sun3.md/Ioc_Unlock.o sun3.md/Ioc_WriteBack.o sun3.md/Sig_Pause.o sun3.md/proc.o sun3.md/procEnviron.o
DISTDIR        ?= @(DISTDIR)
