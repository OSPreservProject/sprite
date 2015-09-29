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
# Sat Sep 28 11:09:21 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/exec.c ds3100.md/mips-pinsn.c ds3100.md/mips-tdep.c ds3100.md/mipsread.c ds3100.md/coredep.c ds3100.md/infptrace.c ds3100.md/mips-xdep.c ds3100.md/initialized_all_files.c ds3100.md/ptraceMips.c blockframe.c breakpoint.c command.c core.c environ.c eval.c expprint.c findvar.c infcmd.c inflow.c infrun.c main.c printcmd.c remote.c source.c stack.c symfile.c symmisc.c symtab.c utils.c valarith.c valops.c valprint.c values.c coffread.c cplus-dem.c dbxread.c expread.y ieee-float.c inftarg.c mem-break.c signame.c target.c version.c
HDRS		= ds3100.md/tm-mips.h ds3100.md/tm.h ds3100.md/xm-mips.h ds3100.md/xm.h breakpoint.h command.h defs.h environ.h expression.h frame.h gdbcmd.h gdbcore.h getpagesize.h ieee-float.h inferior.h param-no-tm.h param.h regex.h signals.h signame.h symfile.h symtab.h target.h tdesc.h terminal.h tm-68k.h tm-i960.h tm-sunos.h value.h
MDPUBHDRS	= 
OBJS		= ds3100.md/blockframe.o ds3100.md/breakpoint.o ds3100.md/coffread.o ds3100.md/command.o ds3100.md/core.o ds3100.md/coredep.o ds3100.md/cplus-dem.o ds3100.md/dbxread.o ds3100.md/environ.o ds3100.md/eval.o ds3100.md/exec.o ds3100.md/expprint.o ds3100.md/expread.o ds3100.md/findvar.o ds3100.md/ieee-float.o ds3100.md/infcmd.o ds3100.md/inflow.o ds3100.md/infptrace.o ds3100.md/infrun.o ds3100.md/inftarg.o ds3100.md/initialized_all_files.o ds3100.md/main.o ds3100.md/mem-break.o ds3100.md/mips-pinsn.o ds3100.md/mips-tdep.o ds3100.md/mips-xdep.o ds3100.md/mipsread.o ds3100.md/printcmd.o ds3100.md/ptraceMips.o ds3100.md/remote.o ds3100.md/signame.o ds3100.md/source.o ds3100.md/stack.o ds3100.md/symfile.o ds3100.md/symmisc.o ds3100.md/symtab.o ds3100.md/target.o ds3100.md/utils.o ds3100.md/valarith.o ds3100.md/valops.o ds3100.md/valprint.o ds3100.md/values.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/exec.o ds3100.md/mips-pinsn.o ds3100.md/mips-tdep.o ds3100.md/mipsread.o ds3100.md/coredep.o ds3100.md/infptrace.o ds3100.md/mips-xdep.o ds3100.md/initialized_all_files.o ds3100.md/ptraceMips.o ds3100.md/blockframe.o ds3100.md/breakpoint.o ds3100.md/command.o ds3100.md/core.o ds3100.md/environ.o ds3100.md/eval.o ds3100.md/expprint.o ds3100.md/findvar.o ds3100.md/infcmd.o ds3100.md/inflow.o ds3100.md/infrun.o ds3100.md/main.o ds3100.md/printcmd.o ds3100.md/remote.o ds3100.md/source.o ds3100.md/stack.o ds3100.md/symfile.o ds3100.md/symmisc.o ds3100.md/symtab.o ds3100.md/utils.o ds3100.md/valarith.o ds3100.md/valops.o ds3100.md/valprint.o ds3100.md/values.o ds3100.md/coffread.o ds3100.md/cplus-dem.o ds3100.md/dbxread.o ds3100.md/expread.o ds3100.md/ieee-float.o ds3100.md/inftarg.o ds3100.md/mem-break.o ds3100.md/signame.o ds3100.md/target.o ds3100.md/version.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
