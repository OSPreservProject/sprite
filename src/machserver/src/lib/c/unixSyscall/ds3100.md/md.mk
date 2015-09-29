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
# Tue Jun  9 14:18:55 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= compatMapCode.c compatSig.c errno.c flock.c ftruncate.c getdirentries.c getdtablesize.c kill.c lseek.c read.c readv.c sigblock.c sigpause.c sigsetmask.c sigvec.c tell.c truncate.c wait.c write.c writev.c execve.c getgid.c gethostname.c getpagesize.c getrlimit.c gettimeofday.c killpg.c setpriority.c setrlimit.c settimeofday.c
HDRS		= compatInt.h compatSig.h
MDPUBHDRS	= 
OBJS		= ds3100.md/compatMapCode.o ds3100.md/compatSig.o ds3100.md/errno.o ds3100.md/flock.o ds3100.md/ftruncate.o ds3100.md/getdirentries.o ds3100.md/getdtablesize.o ds3100.md/getgid.o ds3100.md/gethostname.o ds3100.md/getpagesize.o ds3100.md/getrlimit.o ds3100.md/gettimeofday.o ds3100.md/kill.o ds3100.md/killpg.o ds3100.md/lseek.o ds3100.md/read.o ds3100.md/readv.o ds3100.md/setpriority.o ds3100.md/setrlimit.o ds3100.md/settimeofday.o ds3100.md/sigblock.o ds3100.md/sigpause.o ds3100.md/sigsetmask.o ds3100.md/sigvec.o ds3100.md/tell.o ds3100.md/truncate.o ds3100.md/wait.o ds3100.md/write.o ds3100.md/writev.o ds3100.md/execve.o
CLEANOBJS	= ds3100.md/compatMapCode.o ds3100.md/compatSig.o ds3100.md/errno.o ds3100.md/flock.o ds3100.md/ftruncate.o ds3100.md/getdirentries.o ds3100.md/getdtablesize.o ds3100.md/kill.o ds3100.md/lseek.o ds3100.md/read.o ds3100.md/readv.o ds3100.md/sigblock.o ds3100.md/sigpause.o ds3100.md/sigsetmask.o ds3100.md/sigvec.o ds3100.md/tell.o ds3100.md/truncate.o ds3100.md/wait.o ds3100.md/write.o ds3100.md/writev.o ds3100.md/execve.o ds3100.md/getgid.o ds3100.md/gethostname.o ds3100.md/getpagesize.o ds3100.md/getrlimit.o ds3100.md/gettimeofday.o ds3100.md/killpg.o ds3100.md/setpriority.o ds3100.md/setrlimit.o ds3100.md/settimeofday.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
