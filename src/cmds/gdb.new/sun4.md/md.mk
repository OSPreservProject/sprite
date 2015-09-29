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
# Sat Sep 28 11:09:33 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/pinsn.c sun4.md/sparc-tdep.c sun4.md/sparc-xdep.c sun4.md/exec.c sun4.md/infptrace.c sun4.md/procDebugRegs.c sun4.md/ptrace.c sun4.md/initialized_all_files.c blockframe.c breakpoint.c command.c core.c environ.c eval.c expprint.c findvar.c infcmd.c inflow.c infrun.c main.c printcmd.c remote.c source.c stack.c symfile.c symmisc.c symtab.c utils.c valarith.c valops.c valprint.c values.c coffread.c cplus-dem.c dbxread.c expread.y ieee-float.c inftarg.c mem-break.c signame.c target.c version.c
HDRS		= sun4.md/opcode.h sun4.md/procDebugRegs.h sun4.md/sparc-opcode.h sun4.md/tm-sparc.h sun4.md/tm.h sun4.md/xm-sparc.h sun4.md/xm.h breakpoint.h command.h defs.h environ.h expression.h frame.h gdbcmd.h gdbcore.h getpagesize.h ieee-float.h inferior.h param-no-tm.h param.h regex.h signals.h signame.h symfile.h symtab.h target.h tdesc.h terminal.h tm-68k.h tm-i960.h tm-sunos.h value.h
MDPUBHDRS	= 
OBJS		= sun4.md/blockframe.o sun4.md/breakpoint.o sun4.md/coffread.o sun4.md/command.o sun4.md/core.o sun4.md/cplus-dem.o sun4.md/dbxread.o sun4.md/environ.o sun4.md/eval.o sun4.md/exec.o sun4.md/expprint.o sun4.md/expread.o sun4.md/findvar.o sun4.md/ieee-float.o sun4.md/infcmd.o sun4.md/inflow.o sun4.md/infptrace.o sun4.md/infrun.o sun4.md/inftarg.o sun4.md/initialized_all_files.o sun4.md/main.o sun4.md/mem-break.o sun4.md/pinsn.o sun4.md/printcmd.o sun4.md/procDebugRegs.o sun4.md/ptrace.o sun4.md/remote.o sun4.md/signame.o sun4.md/source.o sun4.md/sparc-tdep.o sun4.md/sparc-xdep.o sun4.md/stack.o sun4.md/symfile.o sun4.md/symmisc.o sun4.md/symtab.o sun4.md/target.o sun4.md/utils.o sun4.md/valarith.o sun4.md/valops.o sun4.md/valprint.o sun4.md/values.o sun4.md/version.o
CLEANOBJS	= sun4.md/pinsn.o sun4.md/sparc-tdep.o sun4.md/sparc-xdep.o sun4.md/exec.o sun4.md/infptrace.o sun4.md/procDebugRegs.o sun4.md/ptrace.o sun4.md/initialized_all_files.o sun4.md/blockframe.o sun4.md/breakpoint.o sun4.md/command.o sun4.md/core.o sun4.md/environ.o sun4.md/eval.o sun4.md/expprint.o sun4.md/findvar.o sun4.md/infcmd.o sun4.md/inflow.o sun4.md/infrun.o sun4.md/main.o sun4.md/printcmd.o sun4.md/remote.o sun4.md/source.o sun4.md/stack.o sun4.md/symfile.o sun4.md/symmisc.o sun4.md/symtab.o sun4.md/utils.o sun4.md/valarith.o sun4.md/valops.o sun4.md/valprint.o sun4.md/values.o sun4.md/coffread.o sun4.md/cplus-dem.o sun4.md/dbxread.o sun4.md/expread.o sun4.md/ieee-float.o sun4.md/inftarg.o sun4.md/mem-break.o sun4.md/signame.o sun4.md/target.o sun4.md/version.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
