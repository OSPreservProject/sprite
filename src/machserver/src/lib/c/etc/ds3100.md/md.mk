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
# Sun Apr 26 21:37:52 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= status.c Rpc_GetName.c alarm.c crypt.c ecvt.c fmt.c frexp.c fsDispatch.c ftime.c gcvt.c getgrent.c getgrgid.c getgrnam.c getlogin.c getopt.c getpass.c getpwent.c getwd.c gtty.c initgroups.c insque.c isatty.c isinf.c isnan.c iszero.c ldexp.c mktemp.c ndbm.c option.c panic.c pause.c pdev.c pfs.c popen.c regex.c setegid.c seteuid.c setgid.c setruid.c setuid.c signal.c statusPrint.c stty.c swapBuffer.c syslog.c ttyDriver.c ttyname.c utime.c valloc.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/Rpc_GetName.o ds3100.md/alarm.o ds3100.md/crypt.o ds3100.md/ecvt.o ds3100.md/fmt.o ds3100.md/frexp.o ds3100.md/fsDispatch.o ds3100.md/ftime.o ds3100.md/gcvt.o ds3100.md/getgrent.o ds3100.md/getgrgid.o ds3100.md/getgrnam.o ds3100.md/getlogin.o ds3100.md/getopt.o ds3100.md/getpass.o ds3100.md/getpwent.o ds3100.md/getwd.o ds3100.md/gtty.o ds3100.md/initgroups.o ds3100.md/insque.o ds3100.md/isatty.o ds3100.md/isinf.o ds3100.md/isnan.o ds3100.md/iszero.o ds3100.md/ldexp.o ds3100.md/mktemp.o ds3100.md/ndbm.o ds3100.md/option.o ds3100.md/panic.o ds3100.md/pause.o ds3100.md/popen.o ds3100.md/regex.o ds3100.md/setegid.o ds3100.md/seteuid.o ds3100.md/setgid.o ds3100.md/setruid.o ds3100.md/setuid.o ds3100.md/signal.o ds3100.md/status.o ds3100.md/statusPrint.o ds3100.md/stty.o ds3100.md/swapBuffer.o ds3100.md/syslog.o ds3100.md/ttyDriver.o ds3100.md/ttyname.o ds3100.md/utime.o ds3100.md/valloc.o ds3100.md/pdev.o ds3100.md/pfs.o
CLEANOBJS	= ds3100.md/status.o ds3100.md/Rpc_GetName.o ds3100.md/alarm.o ds3100.md/crypt.o ds3100.md/ecvt.o ds3100.md/fmt.o ds3100.md/frexp.o ds3100.md/fsDispatch.o ds3100.md/ftime.o ds3100.md/gcvt.o ds3100.md/getgrent.o ds3100.md/getgrgid.o ds3100.md/getgrnam.o ds3100.md/getlogin.o ds3100.md/getopt.o ds3100.md/getpass.o ds3100.md/getpwent.o ds3100.md/getwd.o ds3100.md/gtty.o ds3100.md/initgroups.o ds3100.md/insque.o ds3100.md/isatty.o ds3100.md/isinf.o ds3100.md/isnan.o ds3100.md/iszero.o ds3100.md/ldexp.o ds3100.md/mktemp.o ds3100.md/ndbm.o ds3100.md/option.o ds3100.md/panic.o ds3100.md/pause.o ds3100.md/pdev.o ds3100.md/pfs.o ds3100.md/popen.o ds3100.md/regex.o ds3100.md/setegid.o ds3100.md/seteuid.o ds3100.md/setgid.o ds3100.md/setruid.o ds3100.md/setuid.o ds3100.md/signal.o ds3100.md/statusPrint.o ds3100.md/stty.o ds3100.md/swapBuffer.o ds3100.md/syslog.o ds3100.md/ttyDriver.o ds3100.md/ttyname.o ds3100.md/utime.o ds3100.md/valloc.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
