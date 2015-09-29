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
# Tue Jun  4 16:35:32 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= getgrent.c getgrgid.c getgrnam.c getpass.c getpwent.c getwd.c ndbm.c pfs.c regex.c setegid.c seteuid.c setgid.c setrgid.c setruid.c setuid.c syslog.c utime.c a.out.c getopt.c popen.c remque.c siglist.c alarm.c ecvt.c fmt.c gcvt.c insque.c isatty.c mktemp.c signal.c ttyname.c ttyslot.c crypt.c initgroups.c iszero.c option.c panic.c pdev.c ttyDriver.c ttyPdev.c getlogin.c swapBuffer.c gtty.c getusershell.c ldexp.c shm.c stty.c pause.c sleep.c ftime.c status.c statusPrint.c frexp.c isinf.c fsDispatch.c Misc_InvokeEditor.c Rpc_GetName.c isnan.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/Rpc_GetName.o ds3100.md/a.out.o ds3100.md/alarm.o ds3100.md/crypt.o ds3100.md/ecvt.o ds3100.md/fmt.o ds3100.md/frexp.o ds3100.md/fsDispatch.o ds3100.md/ftime.o ds3100.md/gcvt.o ds3100.md/getgrent.o ds3100.md/getgrgid.o ds3100.md/getgrnam.o ds3100.md/getlogin.o ds3100.md/getopt.o ds3100.md/getpass.o ds3100.md/getpwent.o ds3100.md/getusershell.o ds3100.md/getwd.o ds3100.md/gtty.o ds3100.md/initgroups.o ds3100.md/insque.o ds3100.md/isatty.o ds3100.md/isinf.o ds3100.md/isnan.o ds3100.md/iszero.o ds3100.md/ldexp.o ds3100.md/mktemp.o ds3100.md/ndbm.o ds3100.md/nlist.o ds3100.md/option.o ds3100.md/panic.o ds3100.md/pause.o ds3100.md/pdev.o ds3100.md/pfs.o ds3100.md/popen.o ds3100.md/regex.o ds3100.md/remque.o ds3100.md/setegid.o ds3100.md/seteuid.o ds3100.md/setgid.o ds3100.md/setrgid.o ds3100.md/setruid.o ds3100.md/setuid.o ds3100.md/shm.o ds3100.md/siglist.o ds3100.md/signal.o ds3100.md/sleep.o ds3100.md/status.o ds3100.md/statusPrint.o ds3100.md/stty.o ds3100.md/swapBuffer.o ds3100.md/syslog.o ds3100.md/ttyDriver.o ds3100.md/ttyPdev.o ds3100.md/ttyname.o ds3100.md/ttyslot.o ds3100.md/utime.o ds3100.md/Misc_InvokeEditor.o
CLEANOBJS	= ds3100.md/getgrent.o ds3100.md/getgrgid.o ds3100.md/getgrnam.o ds3100.md/getpass.o ds3100.md/getpwent.o ds3100.md/getwd.o ds3100.md/ndbm.o ds3100.md/pfs.o ds3100.md/regex.o ds3100.md/setegid.o ds3100.md/seteuid.o ds3100.md/setgid.o ds3100.md/setrgid.o ds3100.md/setruid.o ds3100.md/setuid.o ds3100.md/syslog.o ds3100.md/utime.o ds3100.md/a.out.o ds3100.md/getopt.o ds3100.md/popen.o ds3100.md/remque.o ds3100.md/siglist.o ds3100.md/alarm.o ds3100.md/ecvt.o ds3100.md/fmt.o ds3100.md/gcvt.o ds3100.md/insque.o ds3100.md/isatty.o ds3100.md/mktemp.o ds3100.md/signal.o ds3100.md/ttyname.o ds3100.md/ttyslot.o ds3100.md/crypt.o ds3100.md/initgroups.o ds3100.md/iszero.o ds3100.md/option.o ds3100.md/panic.o ds3100.md/pdev.o ds3100.md/ttyDriver.o ds3100.md/ttyPdev.o ds3100.md/getlogin.o ds3100.md/swapBuffer.o ds3100.md/gtty.o ds3100.md/getusershell.o ds3100.md/ldexp.o ds3100.md/shm.o ds3100.md/stty.o ds3100.md/pause.o ds3100.md/sleep.o ds3100.md/ftime.o ds3100.md/status.o ds3100.md/statusPrint.o ds3100.md/frexp.o ds3100.md/isinf.o ds3100.md/fsDispatch.o ds3100.md/Misc_InvokeEditor.o ds3100.md/Rpc_GetName.o ds3100.md/isnan.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= ds3100.md/nlist.o
