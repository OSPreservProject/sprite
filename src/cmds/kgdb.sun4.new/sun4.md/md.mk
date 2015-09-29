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
# Fri Aug 30 14:54:02 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= blockframe.c breakpoint.c coffread.c command.c core.c cplus-dem.c dbxread.c environ.c eval.c expprint.c expread.y findvar.c ieee-float.c infcmd.c inflow.c infrun.c inftarg.c kgdb_ptrace.c kgdbcmd.c main.c mem-break.c printcmd.c remote.c signame.c source.c stack.c symfile.c symmisc.c symtab.c target.c utils.c valarith.c valops.c valprint.c values.c version.c exec.c infptrace.c initialized_all_files.c pinsn.c sparc-tdep.c sparc-xdep.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/blockframe.o sun4.md/breakpoint.o sun4.md/coffread.o sun4.md/command.o sun4.md/core.o sun4.md/cplus-dem.o sun4.md/dbxread.o sun4.md/environ.o sun4.md/eval.o sun4.md/exec.o sun4.md/expprint.o sun4.md/findvar.o sun4.md/ieee-float.o sun4.md/infcmd.o sun4.md/inflow.o sun4.md/infptrace.o sun4.md/infrun.o sun4.md/inftarg.o sun4.md/initialized_all_files.o sun4.md/main.o sun4.md/mem-break.o sun4.md/pinsn.o sun4.md/printcmd.o sun4.md/remote.o sun4.md/signame.o sun4.md/source.o sun4.md/sparc-tdep.o sun4.md/sparc-xdep.o sun4.md/stack.o sun4.md/symfile.o sun4.md/symmisc.o sun4.md/symtab.o sun4.md/target.o sun4.md/utils.o sun4.md/valarith.o sun4.md/valops.o sun4.md/valprint.o sun4.md/values.o sun4.md/expread.o sun4.md/kgdb_ptrace.o sun4.md/kgdbcmd.o sun4.md/version.o
CLEANOBJS	= sun4.md/blockframe.o sun4.md/breakpoint.o sun4.md/coffread.o sun4.md/command.o sun4.md/core.o sun4.md/cplus-dem.o sun4.md/dbxread.o sun4.md/environ.o sun4.md/eval.o sun4.md/expprint.o sun4.md/expread.o sun4.md/findvar.o sun4.md/ieee-float.o sun4.md/infcmd.o sun4.md/inflow.o sun4.md/infrun.o sun4.md/inftarg.o sun4.md/kgdb_ptrace.o sun4.md/kgdbcmd.o sun4.md/main.o sun4.md/mem-break.o sun4.md/printcmd.o sun4.md/remote.o sun4.md/signame.o sun4.md/source.o sun4.md/stack.o sun4.md/symfile.o sun4.md/symmisc.o sun4.md/symtab.o sun4.md/target.o sun4.md/utils.o sun4.md/valarith.o sun4.md/valops.o sun4.md/valprint.o sun4.md/values.o sun4.md/version.o sun4.md/exec.o sun4.md/infptrace.o sun4.md/initialized_all_files.o sun4.md/pinsn.o sun4.md/sparc-tdep.o sun4.md/sparc-xdep.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
