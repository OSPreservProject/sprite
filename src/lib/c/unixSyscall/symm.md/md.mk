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
# Tue Jun 16 11:11:00 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= symm.md/tmp_ctl.c brk.c chown.c _exit.c mmap.c open.c chdir.c compatSig.c select.c socket.c close.c creat.c dup.c errno.c fcntl.c flock.c fork.c fsync.c ftruncate.c getdirentries.c getdtablesize.c kill.c wait.c getgid.c getgroups.c getitimer.c access.c geteuid.c getpagesize.c getpgrp.c getppid.c getpriority.c getrlimit.c getrusage.c killpg.c link.c lseek.c sigvec.c stat.c mkdir.c vfork.c ioctl.c pipe.c read.c readlink.c readv.c rename.c rmdir.c chmod.c getuid.c setgroups.c setpgrp.c setpriority.c setrlimit.c settimeofday.c sigblock.c sigpause.c sigsetmask.c symlink.c truncate.c umask.c unlink.c getpid.c utimes.c write.c writev.c execve.c mknod.c setfiletype.c setregid.c setreuid.c profil.c semop.c tell.c gethostid.c getdomainname.c gettimeofday.c compatMapCode.c gethostname.c sethostname.c
HDRS		= symm.md/tmp_ctl.h compatInt.h compatSig.h
MDPUBHDRS	= 
OBJS		= symm.md/tmp_ctl.o symm.md/brk.o symm.md/chown.o symm.md/_exit.o symm.md/mmap.o symm.md/open.o symm.md/chdir.o symm.md/compatSig.o symm.md/select.o symm.md/socket.o symm.md/close.o symm.md/creat.o symm.md/dup.o symm.md/errno.o symm.md/fcntl.o symm.md/flock.o symm.md/fork.o symm.md/fsync.o symm.md/ftruncate.o symm.md/getdirentries.o symm.md/getdtablesize.o symm.md/kill.o symm.md/wait.o symm.md/getgid.o symm.md/getgroups.o symm.md/getitimer.o symm.md/access.o symm.md/geteuid.o symm.md/getpagesize.o symm.md/getpgrp.o symm.md/getppid.o symm.md/getpriority.o symm.md/getrlimit.o symm.md/getrusage.o symm.md/killpg.o symm.md/link.o symm.md/lseek.o symm.md/sigvec.o symm.md/stat.o symm.md/mkdir.o symm.md/vfork.o symm.md/ioctl.o symm.md/pipe.o symm.md/read.o symm.md/readlink.o symm.md/readv.o symm.md/rename.o symm.md/rmdir.o symm.md/chmod.o symm.md/getuid.o symm.md/setgroups.o symm.md/setpgrp.o symm.md/setpriority.o symm.md/setrlimit.o symm.md/settimeofday.o symm.md/sigblock.o symm.md/sigpause.o symm.md/sigsetmask.o symm.md/symlink.o symm.md/truncate.o symm.md/umask.o symm.md/unlink.o symm.md/getpid.o symm.md/utimes.o symm.md/write.o symm.md/writev.o symm.md/execve.o symm.md/mknod.o symm.md/setfiletype.o symm.md/setregid.o symm.md/setreuid.o symm.md/profil.o symm.md/semop.o symm.md/tell.o symm.md/gethostid.o symm.md/getdomainname.o symm.md/gettimeofday.o symm.md/compatMapCode.o symm.md/gethostname.o symm.md/sethostname.o
CLEANOBJS	= symm.md/tmp_ctl.o symm.md/brk.o symm.md/chown.o symm.md/_exit.o symm.md/mmap.o symm.md/open.o symm.md/chdir.o symm.md/compatSig.o symm.md/select.o symm.md/socket.o symm.md/close.o symm.md/creat.o symm.md/dup.o symm.md/errno.o symm.md/fcntl.o symm.md/flock.o symm.md/fork.o symm.md/fsync.o symm.md/ftruncate.o symm.md/getdirentries.o symm.md/getdtablesize.o symm.md/kill.o symm.md/wait.o symm.md/getgid.o symm.md/getgroups.o symm.md/getitimer.o symm.md/access.o symm.md/geteuid.o symm.md/getpagesize.o symm.md/getpgrp.o symm.md/getppid.o symm.md/getpriority.o symm.md/getrlimit.o symm.md/getrusage.o symm.md/killpg.o symm.md/link.o symm.md/lseek.o symm.md/sigvec.o symm.md/stat.o symm.md/mkdir.o symm.md/vfork.o symm.md/ioctl.o symm.md/pipe.o symm.md/read.o symm.md/readlink.o symm.md/readv.o symm.md/rename.o symm.md/rmdir.o symm.md/chmod.o symm.md/getuid.o symm.md/setgroups.o symm.md/setpgrp.o symm.md/setpriority.o symm.md/setrlimit.o symm.md/settimeofday.o symm.md/sigblock.o symm.md/sigpause.o symm.md/sigsetmask.o symm.md/symlink.o symm.md/truncate.o symm.md/umask.o symm.md/unlink.o symm.md/getpid.o symm.md/utimes.o symm.md/write.o symm.md/writev.o symm.md/execve.o symm.md/mknod.o symm.md/setfiletype.o symm.md/setregid.o symm.md/setreuid.o symm.md/profil.o symm.md/semop.o symm.md/tell.o symm.md/gethostid.o symm.md/getdomainname.o symm.md/gettimeofday.o symm.md/compatMapCode.o symm.md/gethostname.o symm.md/sethostname.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile
SACREDOBJS	= 
