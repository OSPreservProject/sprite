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
# Tue Dec 15 14:56:29 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= c-convert.c c-decl.c toplev.c c-parse.tab.c c-typeck.c caller-save.c combine.c cse.c dbxout.c emit-rtl.c explow.c expmed.c expr.c final.c flow.c fold-const.c global-alloc.c gnulib.c integrate.c jump.c local-alloc.c loop.c recog.c obstack.c optabs.c print-tree.c regclass.c reload.c reload1.c rtl.c sdbout.c stmt.c stor-layout.c stupid.c symout.c tree.c varasm.c version.c rtlanal.c
HDRS		= aux-output.h config.h insn-codes.h insn-config.h insn-flags.h tm.h
MDPUBHDRS	= 
OBJS		= sun4.md/c-convert.o sun4.md/c-decl.o sun4.md/c-parse.tab.o sun4.md/c-typeck.o sun4.md/caller-save.o sun4.md/combine.o sun4.md/cse.o sun4.md/dbxout.o sun4.md/emit-rtl.o sun4.md/explow.o sun4.md/expmed.o sun4.md/expr.o sun4.md/final.o sun4.md/flow.o sun4.md/fold-const.o sun4.md/global-alloc.o sun4.md/gnulib.o sun4.md/insn-emit.o sun4.md/insn-extract.o sun4.md/insn-output.o sun4.md/insn-peep.o sun4.md/insn-recog.o sun4.md/integrate.o sun4.md/jump.o sun4.md/local-alloc.o sun4.md/loop.o sun4.md/obstack.o sun4.md/optabs.o sun4.md/print-tree.o sun4.md/recog.o sun4.md/regclass.o sun4.md/reload.o sun4.md/reload1.o sun4.md/rtl.o sun4.md/rtlanal.o sun4.md/sdbout.o sun4.md/stmt.o sun4.md/stor-layout.o sun4.md/stupid.o sun4.md/symout.o sun4.md/toplev.o sun4.md/tree.o sun4.md/varasm.o sun4.md/version.o
CLEANOBJS	= sun4.md/c-convert.o sun4.md/c-decl.o sun4.md/toplev.o sun4.md/c-parse.tab.o sun4.md/c-typeck.o sun4.md/caller-save.o sun4.md/combine.o sun4.md/cse.o sun4.md/dbxout.o sun4.md/emit-rtl.o sun4.md/explow.o sun4.md/expmed.o sun4.md/expr.o sun4.md/final.o sun4.md/flow.o sun4.md/fold-const.o sun4.md/global-alloc.o sun4.md/gnulib.o sun4.md/integrate.o sun4.md/jump.o sun4.md/local-alloc.o sun4.md/loop.o sun4.md/recog.o sun4.md/obstack.o sun4.md/optabs.o sun4.md/print-tree.o sun4.md/regclass.o sun4.md/reload.o sun4.md/reload1.o sun4.md/rtl.o sun4.md/sdbout.o sun4.md/stmt.o sun4.md/stor-layout.o sun4.md/stupid.o sun4.md/symout.o sun4.md/tree.o sun4.md/varasm.o sun4.md/version.o sun4.md/rtlanal.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= sun4.md/insn-emit.o sun4.md/insn-extract.o sun4.md/insn-output.o sun4.md/insn-peep.o sun4.md/insn-recog.o
