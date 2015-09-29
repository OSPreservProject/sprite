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
# Tue Jun 16 11:10:43 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= brk.c chown.c _exit.c mmap.c open.c chdir.c compatSig.c select.c socket.c close.c creat.c dup.c errno.c fcntl.c flock.c fork.c fsync.c ftruncate.c getdirentries.c getdtablesize.c kill.c wait.c getgid.c getgroups.c getitimer.c access.c geteuid.c getpagesize.c getpgrp.c getppid.c getpriority.c getrlimit.c getrusage.c killpg.c link.c lseek.c sigvec.c stat.c mkdir.c vfork.c ioctl.c pipe.c read.c readlink.c readv.c rename.c rmdir.c chmod.c getuid.c setgroups.c setpgrp.c setpriority.c setrlimit.c settimeofday.c sigblock.c sigpause.c sigsetmask.c symlink.c truncate.c umask.c unlink.c getpid.c utimes.c write.c writev.c execve.c mknod.c setfiletype.c setregid.c setreuid.c profil.c semop.c tell.c gethostid.c getdomainname.c gettimeofday.c compatMapCode.c gethostname.c sethostname.c
HDRS		= compatInt.h compatSig.h
MDPUBHDRS	= 
OBJS		= sun4.md/mkdir.o sun4.md/brk.o sun4.md/chown.o sun4.md/_exit.o sun4.md/mmap.o sun4.md/open.o sun4.md/chdir.o sun4.md/compatSig.o sun4.md/select.o sun4.md/socket.o sun4.md/close.o sun4.md/creat.o sun4.md/dup.o sun4.md/errno.o sun4.md/fcntl.o sun4.md/flock.o sun4.md/fork.o sun4.md/fsync.o sun4.md/ftruncate.o sun4.md/getdirentries.o sun4.md/getdtablesize.o sun4.md/kill.o sun4.md/wait.o sun4.md/getgid.o sun4.md/getgroups.o sun4.md/getitimer.o sun4.md/access.o sun4.md/geteuid.o sun4.md/getpagesize.o sun4.md/getpgrp.o sun4.md/getppid.o sun4.md/getpriority.o sun4.md/getrlimit.o sun4.md/getrusage.o sun4.md/killpg.o sun4.md/link.o sun4.md/lseek.o sun4.md/sigvec.o sun4.md/stat.o sun4.md/vfork.o sun4.md/ioctl.o sun4.md/pipe.o sun4.md/read.o sun4.md/readlink.o sun4.md/readv.o sun4.md/rename.o sun4.md/rmdir.o sun4.md/chmod.o sun4.md/getuid.o sun4.md/setgroups.o sun4.md/setpgrp.o sun4.md/setpriority.o sun4.md/setrlimit.o sun4.md/settimeofday.o sun4.md/sigblock.o sun4.md/sigpause.o sun4.md/sigsetmask.o sun4.md/symlink.o sun4.md/truncate.o sun4.md/umask.o sun4.md/unlink.o sun4.md/getpid.o sun4.md/utimes.o sun4.md/write.o sun4.md/writev.o sun4.md/execve.o sun4.md/mknod.o sun4.md/setfiletype.o sun4.md/setregid.o sun4.md/setreuid.o sun4.md/profil.o sun4.md/semop.o sun4.md/tell.o sun4.md/gethostid.o sun4.md/getdomainname.o sun4.md/gettimeofday.o sun4.md/compatMapCode.o sun4.md/gethostname.o sun4.md/sethostname.o
CLEANOBJS	= sun4.md/brk.o sun4.md/chown.o sun4.md/_exit.o sun4.md/mmap.o sun4.md/open.o sun4.md/chdir.o sun4.md/compatSig.o sun4.md/select.o sun4.md/socket.o sun4.md/close.o sun4.md/creat.o sun4.md/dup.o sun4.md/errno.o sun4.md/fcntl.o sun4.md/flock.o sun4.md/fork.o sun4.md/fsync.o sun4.md/ftruncate.o sun4.md/getdirentries.o sun4.md/getdtablesize.o sun4.md/kill.o sun4.md/wait.o sun4.md/getgid.o sun4.md/getgroups.o sun4.md/getitimer.o sun4.md/access.o sun4.md/geteuid.o sun4.md/getpagesize.o sun4.md/getpgrp.o sun4.md/getppid.o sun4.md/getpriority.o sun4.md/getrlimit.o sun4.md/getrusage.o sun4.md/killpg.o sun4.md/link.o sun4.md/lseek.o sun4.md/sigvec.o sun4.md/stat.o sun4.md/mkdir.o sun4.md/vfork.o sun4.md/ioctl.o sun4.md/pipe.o sun4.md/read.o sun4.md/readlink.o sun4.md/readv.o sun4.md/rename.o sun4.md/rmdir.o sun4.md/chmod.o sun4.md/getuid.o sun4.md/setgroups.o sun4.md/setpgrp.o sun4.md/setpriority.o sun4.md/setrlimit.o sun4.md/settimeofday.o sun4.md/sigblock.o sun4.md/sigpause.o sun4.md/sigsetmask.o sun4.md/symlink.o sun4.md/truncate.o sun4.md/umask.o sun4.md/unlink.o sun4.md/getpid.o sun4.md/utimes.o sun4.md/write.o sun4.md/writev.o sun4.md/execve.o sun4.md/mknod.o sun4.md/setfiletype.o sun4.md/setregid.o sun4.md/setreuid.o sun4.md/profil.o sun4.md/semop.o sun4.md/tell.o sun4.md/gethostid.o sun4.md/getdomainname.o sun4.md/gettimeofday.o sun4.md/compatMapCode.o sun4.md/gethostname.o sun4.md/sethostname.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
