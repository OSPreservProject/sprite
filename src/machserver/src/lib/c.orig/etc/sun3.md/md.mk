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
# Tue Jun  4 16:36:04 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/nlist.c getgrent.c getgrgid.c getgrnam.c getpass.c getpwent.c getwd.c ndbm.c pfs.c regex.c setegid.c seteuid.c setgid.c setrgid.c setruid.c setuid.c syslog.c utime.c a.out.c getopt.c popen.c remque.c siglist.c alarm.c ecvt.c fmt.c gcvt.c insque.c isatty.c mktemp.c signal.c ttyname.c ttyslot.c crypt.c initgroups.c iszero.c option.c panic.c pdev.c ttyDriver.c ttyPdev.c getlogin.c swapBuffer.c gtty.c getusershell.c ldexp.c shm.c stty.c pause.c sleep.c ftime.c status.c statusPrint.c frexp.c isinf.c fsDispatch.c Misc_InvokeEditor.c Rpc_GetName.c isnan.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/nlist.o sun3.md/getgrent.o sun3.md/getgrgid.o sun3.md/getgrnam.o sun3.md/getpass.o sun3.md/getpwent.o sun3.md/getwd.o sun3.md/ndbm.o sun3.md/pfs.o sun3.md/regex.o sun3.md/setegid.o sun3.md/seteuid.o sun3.md/setgid.o sun3.md/setrgid.o sun3.md/setruid.o sun3.md/setuid.o sun3.md/syslog.o sun3.md/utime.o sun3.md/a.out.o sun3.md/getopt.o sun3.md/popen.o sun3.md/remque.o sun3.md/siglist.o sun3.md/alarm.o sun3.md/ecvt.o sun3.md/fmt.o sun3.md/gcvt.o sun3.md/insque.o sun3.md/isatty.o sun3.md/mktemp.o sun3.md/signal.o sun3.md/ttyname.o sun3.md/ttyslot.o sun3.md/crypt.o sun3.md/initgroups.o sun3.md/iszero.o sun3.md/option.o sun3.md/panic.o sun3.md/pdev.o sun3.md/ttyDriver.o sun3.md/ttyPdev.o sun3.md/getlogin.o sun3.md/swapBuffer.o sun3.md/gtty.o sun3.md/getusershell.o sun3.md/ldexp.o sun3.md/shm.o sun3.md/stty.o sun3.md/pause.o sun3.md/sleep.o sun3.md/ftime.o sun3.md/status.o sun3.md/statusPrint.o sun3.md/frexp.o sun3.md/isinf.o sun3.md/fsDispatch.o sun3.md/Misc_InvokeEditor.o sun3.md/Rpc_GetName.o sun3.md/isnan.o
CLEANOBJS	= sun3.md/nlist.o sun3.md/getgrent.o sun3.md/getgrgid.o sun3.md/getgrnam.o sun3.md/getpass.o sun3.md/getpwent.o sun3.md/getwd.o sun3.md/ndbm.o sun3.md/pfs.o sun3.md/regex.o sun3.md/setegid.o sun3.md/seteuid.o sun3.md/setgid.o sun3.md/setrgid.o sun3.md/setruid.o sun3.md/setuid.o sun3.md/syslog.o sun3.md/utime.o sun3.md/a.out.o sun3.md/getopt.o sun3.md/popen.o sun3.md/remque.o sun3.md/siglist.o sun3.md/alarm.o sun3.md/ecvt.o sun3.md/fmt.o sun3.md/gcvt.o sun3.md/insque.o sun3.md/isatty.o sun3.md/mktemp.o sun3.md/signal.o sun3.md/ttyname.o sun3.md/ttyslot.o sun3.md/crypt.o sun3.md/initgroups.o sun3.md/iszero.o sun3.md/option.o sun3.md/panic.o sun3.md/pdev.o sun3.md/ttyDriver.o sun3.md/ttyPdev.o sun3.md/getlogin.o sun3.md/swapBuffer.o sun3.md/gtty.o sun3.md/getusershell.o sun3.md/ldexp.o sun3.md/shm.o sun3.md/stty.o sun3.md/pause.o sun3.md/sleep.o sun3.md/ftime.o sun3.md/status.o sun3.md/statusPrint.o sun3.md/frexp.o sun3.md/isinf.o sun3.md/fsDispatch.o sun3.md/Misc_InvokeEditor.o sun3.md/Rpc_GetName.o sun3.md/isnan.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
