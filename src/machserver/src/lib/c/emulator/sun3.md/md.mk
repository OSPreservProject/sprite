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
# Sun Dec  8 18:11:29 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/emuAsm.s emuFork.c emuInit.c _exit.c spriteSrvUser.c mapStatus.c syscalls.c execve.c sleepTmp.c getpid.c gettimeofday.c ioctl.c brk.c stat.c unixSyscalls.c Fs_IOControl.c Fs_Read.c Ioc_GetOwner.c Ioc_SetOwner.c Fs_Select.c Fs_Write.c proc.c Fs_WriteBack.c
HDRS		= sun3.md/spriteEmuMach.h compatInt.h spriteEmuInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/emuAsm.o sun3.md/emuFork.o sun3.md/emuInit.o sun3.md/_exit.o sun3.md/spriteSrvUser.o sun3.md/mapStatus.o sun3.md/syscalls.o sun3.md/execve.o sun3.md/sleepTmp.o sun3.md/getpid.o sun3.md/gettimeofday.o sun3.md/ioctl.o sun3.md/brk.o sun3.md/stat.o sun3.md/unixSyscalls.o sun3.md/Fs_IOControl.o sun3.md/Fs_Read.o sun3.md/Ioc_GetOwner.o sun3.md/Ioc_SetOwner.o sun3.md/Fs_Select.o sun3.md/Fs_Write.o sun3.md/proc.o sun3.md/Fs_WriteBack.o
CLEANOBJS	= sun3.md/emuAsm.o sun3.md/emuFork.o sun3.md/emuInit.o sun3.md/_exit.o sun3.md/spriteSrvUser.o sun3.md/mapStatus.o sun3.md/syscalls.o sun3.md/execve.o sun3.md/sleepTmp.o sun3.md/getpid.o sun3.md/gettimeofday.o sun3.md/ioctl.o sun3.md/brk.o sun3.md/stat.o sun3.md/unixSyscalls.o sun3.md/Fs_IOControl.o sun3.md/Fs_Read.o sun3.md/Ioc_GetOwner.o sun3.md/Ioc_SetOwner.o sun3.md/Fs_Select.o sun3.md/Fs_Write.o sun3.md/proc.o sun3.md/Fs_WriteBack.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
