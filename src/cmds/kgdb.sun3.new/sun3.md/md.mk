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
# Mon Nov 19 17:47:21 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= blockframe.c breakpoint.c command.c funmap.c dbxread.c environ.c eval.c expprint.c findvar.c infcmd.c inflow.c infrun.c keymaps.c kgdbcmd.c main.c obstack.c printcmd.c ptrace.c remote.c source.c stack.c symmisc.c symtab.c utils.c valarith.c valops.c valprint.c values.c version.c core.c dep.c initialized_all_files.c pinsn.c readline.c expread.y history.c copying.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/blockframe.o sun3.md/breakpoint.o sun3.md/command.o sun3.md/copying.o sun3.md/core.o sun3.md/dbxread.o sun3.md/dep.o sun3.md/environ.o sun3.md/eval.o sun3.md/expprint.o sun3.md/expread.o sun3.md/findvar.o sun3.md/funmap.o sun3.md/history.o sun3.md/infcmd.o sun3.md/inflow.o sun3.md/infrun.o sun3.md/initialized_all_files.o sun3.md/keymaps.o sun3.md/kgdbcmd.o sun3.md/main.o sun3.md/obstack.o sun3.md/pinsn.o sun3.md/printcmd.o sun3.md/ptrace.o sun3.md/readline.o sun3.md/remote.o sun3.md/source.o sun3.md/stack.o sun3.md/symmisc.o sun3.md/symtab.o sun3.md/utils.o sun3.md/valarith.o sun3.md/valops.o sun3.md/valprint.o sun3.md/values.o sun3.md/version.o
CLEANOBJS	= sun3.md/blockframe.o sun3.md/breakpoint.o sun3.md/command.o sun3.md/funmap.o sun3.md/dbxread.o sun3.md/environ.o sun3.md/eval.o sun3.md/expprint.o sun3.md/findvar.o sun3.md/infcmd.o sun3.md/inflow.o sun3.md/infrun.o sun3.md/keymaps.o sun3.md/kgdbcmd.o sun3.md/main.o sun3.md/obstack.o sun3.md/printcmd.o sun3.md/ptrace.o sun3.md/remote.o sun3.md/source.o sun3.md/stack.o sun3.md/symmisc.o sun3.md/symtab.o sun3.md/utils.o sun3.md/valarith.o sun3.md/valops.o sun3.md/valprint.o sun3.md/values.o sun3.md/version.o sun3.md/core.o sun3.md/dep.o sun3.md/initialized_all_files.o sun3.md/pinsn.o sun3.md/readline.o sun3.md/expread.o sun3.md/history.o sun3.md/copying.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
