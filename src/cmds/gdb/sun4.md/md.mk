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
# Tue Jul  2 17:37:21 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/dep.c sun4.md/pinsn.c sun4.md/procDebugRegs.c blockframe.c breakpoint.c command.c copying.c dbxread.c environ.c eval.c expprint.c core.c expread.tab.c findvar.c history.c infcmd.c inflow.c infrun.c keymaps.c main.c obstack.c printcmd.c readline.c regex.c remote.c source.c stack.c symmisc.c symtab.c utils.c valarith.c valops.c valprint.c values.c version.c funmap.c initialized_all_files.c
HDRS		= sun4.md/m-sparc.h sun4.md/opcode.h sun4.md/param.h sun4.md/sparc-opcode.h emacs_keymap.c.h termio.h
MDPUBHDRS	= 
OBJS		= sun4.md/blockframe.o sun4.md/breakpoint.o sun4.md/command.o sun4.md/copying.o sun4.md/core.o sun4.md/dbxread.o sun4.md/dep.o sun4.md/environ.o sun4.md/eval.o sun4.md/expprint.o sun4.md/expread.tab.o sun4.md/findvar.o sun4.md/funmap.o sun4.md/history.o sun4.md/infcmd.o sun4.md/inflow.o sun4.md/infrun.o sun4.md/initialized_all_files.o sun4.md/keymaps.o sun4.md/main.o sun4.md/obstack.o sun4.md/pinsn.o sun4.md/printcmd.o sun4.md/procDebugRegs.o sun4.md/ptrace.o sun4.md/readline.o sun4.md/regex.o sun4.md/remote.o sun4.md/source.o sun4.md/stack.o sun4.md/symmisc.o sun4.md/symtab.o sun4.md/utils.o sun4.md/valarith.o sun4.md/valops.o sun4.md/valprint.o sun4.md/values.o sun4.md/version.o
CLEANOBJS	= sun4.md/dep.o sun4.md/pinsn.o sun4.md/procDebugRegs.o sun4.md/blockframe.o sun4.md/breakpoint.o sun4.md/command.o sun4.md/copying.o sun4.md/dbxread.o sun4.md/environ.o sun4.md/eval.o sun4.md/expprint.o sun4.md/core.o sun4.md/expread.tab.o sun4.md/findvar.o sun4.md/history.o sun4.md/infcmd.o sun4.md/inflow.o sun4.md/infrun.o sun4.md/keymaps.o sun4.md/main.o sun4.md/obstack.o sun4.md/printcmd.o sun4.md/readline.o sun4.md/regex.o sun4.md/remote.o sun4.md/source.o sun4.md/stack.o sun4.md/symmisc.o sun4.md/symtab.o sun4.md/utils.o sun4.md/valarith.o sun4.md/valops.o sun4.md/valprint.o sun4.md/values.o sun4.md/version.o sun4.md/funmap.o sun4.md/initialized_all_files.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= sun4.md/ptrace.o
