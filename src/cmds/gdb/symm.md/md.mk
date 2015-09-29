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
# Wed Nov 21 13:26:33 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= symm.md/dep.c symm.md/pinsn.c symm.md/procDebugRegs.c blockframe.c breakpoint.c command.c copying.c dbxread.c environ.c eval.c expprint.c core.c expread.tab.c findvar.c history.c infcmd.c inflow.c infrun.c keymaps.c main.c obstack.c printcmd.c ptrace.c readline.c regex.c remote.c source.c stack.c symmisc.c symtab.c utils.c valarith.c valops.c valprint.c values.c version.c funmap.c initialized_all_files.c
HDRS		= symm.md/param.h emacs_keymap.c.h termio.h
MDPUBHDRS	= 
OBJS		= symm.md/blockframe.o symm.md/breakpoint.o symm.md/command.o symm.md/copying.o symm.md/core.o symm.md/dbxread.o symm.md/dep.o symm.md/environ.o symm.md/eval.o symm.md/expprint.o symm.md/expread.tab.o symm.md/findvar.o symm.md/funmap.o symm.md/history.o symm.md/infcmd.o symm.md/inflow.o symm.md/infrun.o symm.md/initialized_all_files.o symm.md/keymaps.o symm.md/main.o symm.md/obstack.o symm.md/pinsn.o symm.md/printcmd.o symm.md/procDebugRegs.o symm.md/ptrace.o symm.md/readline.o symm.md/regex.o symm.md/remote.o symm.md/source.o symm.md/stack.o symm.md/symmisc.o symm.md/symtab.o symm.md/utils.o symm.md/valarith.o symm.md/valops.o symm.md/valprint.o symm.md/values.o symm.md/version.o
CLEANOBJS	= symm.md/dep.o symm.md/pinsn.o symm.md/procDebugRegs.o symm.md/blockframe.o symm.md/breakpoint.o symm.md/command.o symm.md/copying.o symm.md/dbxread.o symm.md/environ.o symm.md/eval.o symm.md/expprint.o symm.md/core.o symm.md/expread.tab.o symm.md/findvar.o symm.md/history.o symm.md/infcmd.o symm.md/inflow.o symm.md/infrun.o symm.md/keymaps.o symm.md/main.o symm.md/obstack.o symm.md/printcmd.o symm.md/ptrace.o symm.md/readline.o symm.md/regex.o symm.md/remote.o symm.md/source.o symm.md/stack.o symm.md/symmisc.o symm.md/symtab.o symm.md/utils.o symm.md/valarith.o symm.md/valops.o symm.md/valprint.o symm.md/values.o symm.md/version.o symm.md/funmap.o symm.md/initialized_all_files.o
INSTFILES	= symm.md/md.mk Makefile local.mk
SACREDOBJS	= 
