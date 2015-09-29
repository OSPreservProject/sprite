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
# Tue Dec 15 14:56:19 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= c-convert.c c-decl.c toplev.c c-parse.tab.c c-typeck.c caller-save.c combine.c cse.c dbxout.c emit-rtl.c explow.c expmed.c expr.c final.c flow.c fold-const.c global-alloc.c gnulib.c integrate.c jump.c local-alloc.c loop.c recog.c obstack.c optabs.c print-tree.c regclass.c reload.c reload1.c rtl.c sdbout.c stmt.c stor-layout.c stupid.c symout.c tree.c varasm.c version.c rtlanal.c
HDRS		= aux-output.h config.h insn-codes.h insn-config.h insn-flags.h tm.h
MDPUBHDRS	= 
OBJS		= sun3.md/c-convert.o sun3.md/c-decl.o sun3.md/toplev.o sun3.md/c-parse.tab.o sun3.md/c-typeck.o sun3.md/caller-save.o sun3.md/combine.o sun3.md/cse.o sun3.md/dbxout.o sun3.md/emit-rtl.o sun3.md/explow.o sun3.md/expmed.o sun3.md/expr.o sun3.md/final.o sun3.md/flow.o sun3.md/fold-const.o sun3.md/global-alloc.o sun3.md/gnulib.o sun3.md/integrate.o sun3.md/jump.o sun3.md/local-alloc.o sun3.md/loop.o sun3.md/recog.o sun3.md/obstack.o sun3.md/optabs.o sun3.md/print-tree.o sun3.md/regclass.o sun3.md/reload.o sun3.md/reload1.o sun3.md/rtl.o sun3.md/sdbout.o sun3.md/stmt.o sun3.md/stor-layout.o sun3.md/stupid.o sun3.md/symout.o sun3.md/tree.o sun3.md/varasm.o sun3.md/version.o sun3.md/rtlanal.o
CLEANOBJS	= sun3.md/c-convert.o sun3.md/c-decl.o sun3.md/toplev.o sun3.md/c-parse.tab.o sun3.md/c-typeck.o sun3.md/caller-save.o sun3.md/combine.o sun3.md/cse.o sun3.md/dbxout.o sun3.md/emit-rtl.o sun3.md/explow.o sun3.md/expmed.o sun3.md/expr.o sun3.md/final.o sun3.md/flow.o sun3.md/fold-const.o sun3.md/global-alloc.o sun3.md/gnulib.o sun3.md/integrate.o sun3.md/jump.o sun3.md/local-alloc.o sun3.md/loop.o sun3.md/recog.o sun3.md/obstack.o sun3.md/optabs.o sun3.md/print-tree.o sun3.md/regclass.o sun3.md/reload.o sun3.md/reload1.o sun3.md/rtl.o sun3.md/sdbout.o sun3.md/stmt.o sun3.md/stor-layout.o sun3.md/stupid.o sun3.md/symout.o sun3.md/tree.o sun3.md/varasm.o sun3.md/version.o sun3.md/rtlanal.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
