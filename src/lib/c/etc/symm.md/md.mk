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
# Mon Jun  8 14:25:30 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= symm.md/nlist.c getgrent.c getgrgid.c getgrnam.c getpass.c getpwent.c getwd.c ndbm.c pfs.c regex.c setegid.c seteuid.c setgid.c setrgid.c setruid.c setuid.c syslog.c utime.c a.out.c getopt.c popen.c remque.c siglist.c alarm.c ecvt.c fmt.c gcvt.c insque.c isatty.c mktemp.c signal.c ttyname.c ttyslot.c crypt.c initgroups.c iszero.c option.c panic.c pdev.c ttyDriver.c ttyPdev.c getlogin.c swapBuffer.c gtty.c getusershell.c ldexp.c shm.c stty.c pause.c sleep.c ftime.c status.c statusPrint.c frexp.c isinf.c fsDispatch.c isnan.c Misc_InvokeEditor.c Rpc_GetName.c valloc.c usleep.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/nlist.o symm.md/getgrent.o symm.md/getgrgid.o symm.md/getgrnam.o symm.md/getpass.o symm.md/getpwent.o symm.md/getwd.o symm.md/ndbm.o symm.md/pfs.o symm.md/regex.o symm.md/setegid.o symm.md/seteuid.o symm.md/setgid.o symm.md/setrgid.o symm.md/setruid.o symm.md/setuid.o symm.md/syslog.o symm.md/utime.o symm.md/a.out.o symm.md/getopt.o symm.md/popen.o symm.md/remque.o symm.md/siglist.o symm.md/alarm.o symm.md/ecvt.o symm.md/fmt.o symm.md/gcvt.o symm.md/insque.o symm.md/isatty.o symm.md/mktemp.o symm.md/signal.o symm.md/ttyname.o symm.md/ttyslot.o symm.md/crypt.o symm.md/initgroups.o symm.md/iszero.o symm.md/option.o symm.md/panic.o symm.md/pdev.o symm.md/ttyDriver.o symm.md/ttyPdev.o symm.md/getlogin.o symm.md/swapBuffer.o symm.md/gtty.o symm.md/getusershell.o symm.md/ldexp.o symm.md/shm.o symm.md/stty.o symm.md/pause.o symm.md/sleep.o symm.md/ftime.o symm.md/status.o symm.md/statusPrint.o symm.md/frexp.o symm.md/isinf.o symm.md/fsDispatch.o symm.md/isnan.o symm.md/Misc_InvokeEditor.o symm.md/Rpc_GetName.o symm.md/valloc.o symm.md/usleep.o
CLEANOBJS	= symm.md/nlist.o symm.md/getgrent.o symm.md/getgrgid.o symm.md/getgrnam.o symm.md/getpass.o symm.md/getpwent.o symm.md/getwd.o symm.md/ndbm.o symm.md/pfs.o symm.md/regex.o symm.md/setegid.o symm.md/seteuid.o symm.md/setgid.o symm.md/setrgid.o symm.md/setruid.o symm.md/setuid.o symm.md/syslog.o symm.md/utime.o symm.md/a.out.o symm.md/getopt.o symm.md/popen.o symm.md/remque.o symm.md/siglist.o symm.md/alarm.o symm.md/ecvt.o symm.md/fmt.o symm.md/gcvt.o symm.md/insque.o symm.md/isatty.o symm.md/mktemp.o symm.md/signal.o symm.md/ttyname.o symm.md/ttyslot.o symm.md/crypt.o symm.md/initgroups.o symm.md/iszero.o symm.md/option.o symm.md/panic.o symm.md/pdev.o symm.md/ttyDriver.o symm.md/ttyPdev.o symm.md/getlogin.o symm.md/swapBuffer.o symm.md/gtty.o symm.md/getusershell.o symm.md/ldexp.o symm.md/shm.o symm.md/stty.o symm.md/pause.o symm.md/sleep.o symm.md/ftime.o symm.md/status.o symm.md/statusPrint.o symm.md/frexp.o symm.md/isinf.o symm.md/fsDispatch.o symm.md/isnan.o symm.md/Misc_InvokeEditor.o symm.md/Rpc_GetName.o symm.md/valloc.o symm.md/usleep.o
INSTFILES	= symm.md/md.mk Makefile tags TAGS
SACREDOBJS	= 
