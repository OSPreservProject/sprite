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
# Tue Jun  9 14:15:27 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/emuAsm.s emuFork.c emuInit.c _exit.c spriteSrvUser.c mapStatus.c syscalls.c getpid.c ioctl.c sleepTmp.c Fs_Read.c Fs_Select.c Fs_Write.c Ioc_GetOwner.c Ioc_SetOwner.c Sig_Pause.c brk.c fcntl.c mknod.c open.c proc.c select.c stat.c umask.c utimes.c Fs_IOControl.c Fs_WriteBack.c getuid.c signals.c Sys_CallName.c access.c chdir.c chmod.c chown.c close.c creat.c dup.c fsync.c geteuid.c getgroups.c getitimer.c getpgrp.c getppid.c getrusage.c link.c mkdir.c pipe.c readlink.c rename.c rmdir.c setgroups.c setpgrp.c setregid.c setreuid.c symlink.c unlink.c
HDRS		= ds3100.md/spriteEmuMach.h compatInt.h spriteEmuInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Fs_IOControl.o ds3100.md/Fs_Read.o ds3100.md/Fs_Select.o ds3100.md/Fs_Write.o ds3100.md/Fs_WriteBack.o ds3100.md/Ioc_GetOwner.o ds3100.md/Ioc_SetOwner.o ds3100.md/Sig_Pause.o ds3100.md/Sys_CallName.o ds3100.md/_exit.o ds3100.md/access.o ds3100.md/brk.o ds3100.md/chdir.o ds3100.md/chmod.o ds3100.md/chown.o ds3100.md/close.o ds3100.md/creat.o ds3100.md/dup.o ds3100.md/emuAsm.o ds3100.md/emuFork.o ds3100.md/emuInit.o ds3100.md/fcntl.o ds3100.md/fsync.o ds3100.md/geteuid.o ds3100.md/getgroups.o ds3100.md/getitimer.o ds3100.md/getpgrp.o ds3100.md/getpid.o ds3100.md/getppid.o ds3100.md/getrusage.o ds3100.md/getuid.o ds3100.md/ioctl.o ds3100.md/link.o ds3100.md/mapStatus.o ds3100.md/mkdir.o ds3100.md/mknod.o ds3100.md/open.o ds3100.md/pipe.o ds3100.md/proc.o ds3100.md/readlink.o ds3100.md/rename.o ds3100.md/rmdir.o ds3100.md/select.o ds3100.md/setgroups.o ds3100.md/setpgrp.o ds3100.md/setregid.o ds3100.md/setreuid.o ds3100.md/signals.o ds3100.md/sleepTmp.o ds3100.md/spriteSrvUser.o ds3100.md/stat.o ds3100.md/symlink.o ds3100.md/syscalls.o ds3100.md/umask.o ds3100.md/unlink.o ds3100.md/utimes.o
CLEANOBJS	= ds3100.md/emuAsm.o ds3100.md/emuFork.o ds3100.md/emuInit.o ds3100.md/_exit.o ds3100.md/spriteSrvUser.o ds3100.md/mapStatus.o ds3100.md/syscalls.o ds3100.md/getpid.o ds3100.md/ioctl.o ds3100.md/sleepTmp.o ds3100.md/Fs_Read.o ds3100.md/Fs_Select.o ds3100.md/Fs_Write.o ds3100.md/Ioc_GetOwner.o ds3100.md/Ioc_SetOwner.o ds3100.md/Sig_Pause.o ds3100.md/brk.o ds3100.md/fcntl.o ds3100.md/mknod.o ds3100.md/open.o ds3100.md/proc.o ds3100.md/select.o ds3100.md/stat.o ds3100.md/umask.o ds3100.md/utimes.o ds3100.md/Fs_IOControl.o ds3100.md/Fs_WriteBack.o ds3100.md/getuid.o ds3100.md/signals.o ds3100.md/Sys_CallName.o ds3100.md/access.o ds3100.md/chdir.o ds3100.md/chmod.o ds3100.md/chown.o ds3100.md/close.o ds3100.md/creat.o ds3100.md/dup.o ds3100.md/fsync.o ds3100.md/geteuid.o ds3100.md/getgroups.o ds3100.md/getitimer.o ds3100.md/getpgrp.o ds3100.md/getppid.o ds3100.md/getrusage.o ds3100.md/link.o ds3100.md/mkdir.o ds3100.md/pipe.o ds3100.md/readlink.o ds3100.md/rename.o ds3100.md/rmdir.o ds3100.md/setgroups.o ds3100.md/setpgrp.o ds3100.md/setregid.o ds3100.md/setreuid.o ds3100.md/symlink.o ds3100.md/unlink.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
