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
# Thu Jul 26 18:52:44 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= _exit.c access.c brk.c chdir.c chmod.c chown.c close.c compatMapCode.c compatSig.c creat.c dup.c errno.c execve.c fcntl.c flock.c fork.c fsync.c ftruncate.c getdirentries.c getdtablesize.c geteuid.c getgid.c getgroups.c gethostname.c getitimer.c getpagesize.c getpgrp.c getpid.c getppid.c getpriority.c getrlimit.c getrusage.c gettimeofday.c getuid.c ioctl.c kill.c killpg.c link.c lseek.c mkdir.c mknod.c mmap.c open.c pipe.c profil.c read.c readlink.c readv.c rename.c rmdir.c select.c semop.c setfiletype.c setgroups.c setpgrp.c setpriority.c setregid.c setreuid.c setrlimit.c settimeofday.c sigblock.c sigpause.c sigsetmask.c sigvec.c socket.c stat.c symlink.c tell.c truncate.c umask.c unlink.c utimes.c vfork.c wait.c write.c writev.c
HDRS		= compatInt.h compatSig.h
MDPUBHDRS	= 
OBJS		= sun3.md/_exit.o sun3.md/access.o sun3.md/brk.o sun3.md/chdir.o sun3.md/chmod.o sun3.md/chown.o sun3.md/close.o sun3.md/compatMapCode.o sun3.md/compatSig.o sun3.md/creat.o sun3.md/dup.o sun3.md/errno.o sun3.md/execve.o sun3.md/fcntl.o sun3.md/flock.o sun3.md/fork.o sun3.md/fsync.o sun3.md/ftruncate.o sun3.md/getdirentries.o sun3.md/getdtablesize.o sun3.md/geteuid.o sun3.md/getgid.o sun3.md/getgroups.o sun3.md/gethostname.o sun3.md/getitimer.o sun3.md/getpagesize.o sun3.md/getpgrp.o sun3.md/getpid.o sun3.md/getppid.o sun3.md/getpriority.o sun3.md/getrlimit.o sun3.md/getrusage.o sun3.md/gettimeofday.o sun3.md/getuid.o sun3.md/ioctl.o sun3.md/kill.o sun3.md/killpg.o sun3.md/link.o sun3.md/lseek.o sun3.md/mkdir.o sun3.md/mknod.o sun3.md/mmap.o sun3.md/open.o sun3.md/pipe.o sun3.md/profil.o sun3.md/read.o sun3.md/readlink.o sun3.md/readv.o sun3.md/rename.o sun3.md/rmdir.o sun3.md/select.o sun3.md/semop.o sun3.md/setfiletype.o sun3.md/setgroups.o sun3.md/setpgrp.o sun3.md/setpriority.o sun3.md/setregid.o sun3.md/setreuid.o sun3.md/setrlimit.o sun3.md/settimeofday.o sun3.md/sigblock.o sun3.md/sigpause.o sun3.md/sigsetmask.o sun3.md/sigvec.o sun3.md/socket.o sun3.md/stat.o sun3.md/symlink.o sun3.md/tell.o sun3.md/truncate.o sun3.md/umask.o sun3.md/unlink.o sun3.md/utimes.o sun3.md/vfork.o sun3.md/wait.o sun3.md/write.o sun3.md/writev.o
CLEANOBJS	= sun3.md/_exit.o sun3.md/access.o sun3.md/brk.o sun3.md/chdir.o sun3.md/chmod.o sun3.md/chown.o sun3.md/close.o sun3.md/compatMapCode.o sun3.md/compatSig.o sun3.md/creat.o sun3.md/dup.o sun3.md/errno.o sun3.md/execve.o sun3.md/fcntl.o sun3.md/flock.o sun3.md/fork.o sun3.md/fsync.o sun3.md/ftruncate.o sun3.md/getdirentries.o sun3.md/getdtablesize.o sun3.md/geteuid.o sun3.md/getgid.o sun3.md/getgroups.o sun3.md/gethostname.o sun3.md/getitimer.o sun3.md/getpagesize.o sun3.md/getpgrp.o sun3.md/getpid.o sun3.md/getppid.o sun3.md/getpriority.o sun3.md/getrlimit.o sun3.md/getrusage.o sun3.md/gettimeofday.o sun3.md/getuid.o sun3.md/ioctl.o sun3.md/kill.o sun3.md/killpg.o sun3.md/link.o sun3.md/lseek.o sun3.md/mkdir.o sun3.md/mknod.o sun3.md/mmap.o sun3.md/open.o sun3.md/pipe.o sun3.md/profil.o sun3.md/read.o sun3.md/readlink.o sun3.md/readv.o sun3.md/rename.o sun3.md/rmdir.o sun3.md/select.o sun3.md/semop.o sun3.md/setfiletype.o sun3.md/setgroups.o sun3.md/setpgrp.o sun3.md/setpriority.o sun3.md/setregid.o sun3.md/setreuid.o sun3.md/setrlimit.o sun3.md/settimeofday.o sun3.md/sigblock.o sun3.md/sigpause.o sun3.md/sigsetmask.o sun3.md/sigvec.o sun3.md/socket.o sun3.md/stat.o sun3.md/symlink.o sun3.md/tell.o sun3.md/truncate.o sun3.md/umask.o sun3.md/unlink.o sun3.md/utimes.o sun3.md/vfork.o sun3.md/wait.o sun3.md/write.o sun3.md/writev.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
