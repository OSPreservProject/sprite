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
# Tue Jul  2 17:37:44 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/coffread.c ds3100.md/dep.c ds3100.md/pinsn.c ds3100.md/ptrace.c ds3100.md/initialized_all_files.c blockframe.c breakpoint.c command.c copying.c dbxread.c environ.c eval.c expprint.c core.c expread.tab.c findvar.c history.c infcmd.c inflow.c infrun.c keymaps.c main.c obstack.c printcmd.c readline.c regex.c remote.c source.c stack.c symmisc.c symtab.c utils.c valarith.c valops.c valprint.c values.c version.c funmap.c
HDRS		= ds3100.md/m-mips.h ds3100.md/mips-opcode.h ds3100.md/opcode.h ds3100.md/param.h emacs_keymap.c.h termio.h
MDPUBHDRS	= 
OBJS		= ds3100.md/blockframe.o ds3100.md/breakpoint.o ds3100.md/coffread.o ds3100.md/command.o ds3100.md/copying.o ds3100.md/core.o ds3100.md/dbxread.o ds3100.md/dep.o ds3100.md/environ.o ds3100.md/eval.o ds3100.md/expprint.o ds3100.md/expread.tab.o ds3100.md/findvar.o ds3100.md/funmap.o ds3100.md/history.o ds3100.md/infcmd.o ds3100.md/inflow.o ds3100.md/infrun.o ds3100.md/initialized_all_files.o ds3100.md/keymaps.o ds3100.md/main.o ds3100.md/obstack.o ds3100.md/pinsn.o ds3100.md/printcmd.o ds3100.md/ptrace.o ds3100.md/readline.o ds3100.md/regex.o ds3100.md/remote.o ds3100.md/source.o ds3100.md/stack.o ds3100.md/symmisc.o ds3100.md/symtab.o ds3100.md/utils.o ds3100.md/valarith.o ds3100.md/valops.o ds3100.md/valprint.o ds3100.md/values.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/coffread.o ds3100.md/dep.o ds3100.md/pinsn.o ds3100.md/ptrace.o ds3100.md/initialized_all_files.o ds3100.md/blockframe.o ds3100.md/breakpoint.o ds3100.md/command.o ds3100.md/copying.o ds3100.md/dbxread.o ds3100.md/environ.o ds3100.md/eval.o ds3100.md/expprint.o ds3100.md/core.o ds3100.md/expread.tab.o ds3100.md/findvar.o ds3100.md/history.o ds3100.md/infcmd.o ds3100.md/inflow.o ds3100.md/infrun.o ds3100.md/keymaps.o ds3100.md/main.o ds3100.md/obstack.o ds3100.md/printcmd.o ds3100.md/readline.o ds3100.md/regex.o ds3100.md/remote.o ds3100.md/source.o ds3100.md/stack.o ds3100.md/symmisc.o ds3100.md/symtab.o ds3100.md/utils.o ds3100.md/valarith.o ds3100.md/valops.o ds3100.md/valprint.o ds3100.md/values.o ds3100.md/version.o ds3100.md/funmap.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
