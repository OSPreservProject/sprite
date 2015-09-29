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
# Thu Dec 17 16:47:56 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= obstack.c toplev.c c-convert.c c-decl.c c-parse.tab.c c-typeck.c caller-save.c combine.c rtlanal.c cse.c dbxout.c emit-rtl.c explow.c expmed.c expr.c final.c flow.c fold-const.c global-alloc.c gnulib.c integrate.c jump.c local-alloc.c loop.c optabs.c print-tree.c recog.c regclass.c reload.c reload1.c rtl.c sdbout.c stmt.c stor-layout.c stupid.c symout.c tree.c varasm.c version.c
HDRS		= aux-output.h config.h insn-codes.h insn-config.h insn-flags.h tm.h
MDPUBHDRS	= 
OBJS		= ds3100.md/c-convert.o ds3100.md/c-decl.o ds3100.md/c-parse.tab.o ds3100.md/c-typeck.o ds3100.md/caller-save.o ds3100.md/combine.o ds3100.md/cse.o ds3100.md/dbxout.o ds3100.md/emit-rtl.o ds3100.md/explow.o ds3100.md/expmed.o ds3100.md/expr.o ds3100.md/final.o ds3100.md/flow.o ds3100.md/fold-const.o ds3100.md/global-alloc.o ds3100.md/gnulib.o ds3100.md/insn-emit.o ds3100.md/insn-extract.o ds3100.md/insn-output.o ds3100.md/insn-peep.o ds3100.md/insn-recog.o ds3100.md/integrate.o ds3100.md/jump.o ds3100.md/local-alloc.o ds3100.md/loop.o ds3100.md/obstack.o ds3100.md/optabs.o ds3100.md/print-tree.o ds3100.md/recog.o ds3100.md/regclass.o ds3100.md/reload.o ds3100.md/reload1.o ds3100.md/rtl.o ds3100.md/rtlanal.o ds3100.md/sdbout.o ds3100.md/stmt.o ds3100.md/stor-layout.o ds3100.md/stupid.o ds3100.md/symout.o ds3100.md/toplev.o ds3100.md/tree.o ds3100.md/varasm.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/obstack.o ds3100.md/toplev.o ds3100.md/c-convert.o ds3100.md/c-decl.o ds3100.md/c-parse.tab.o ds3100.md/c-typeck.o ds3100.md/caller-save.o ds3100.md/combine.o ds3100.md/rtlanal.o ds3100.md/cse.o ds3100.md/dbxout.o ds3100.md/emit-rtl.o ds3100.md/explow.o ds3100.md/expmed.o ds3100.md/expr.o ds3100.md/final.o ds3100.md/flow.o ds3100.md/fold-const.o ds3100.md/global-alloc.o ds3100.md/gnulib.o ds3100.md/integrate.o ds3100.md/jump.o ds3100.md/local-alloc.o ds3100.md/loop.o ds3100.md/optabs.o ds3100.md/print-tree.o ds3100.md/recog.o ds3100.md/regclass.o ds3100.md/reload.o ds3100.md/reload1.o ds3100.md/rtl.o ds3100.md/sdbout.o ds3100.md/stmt.o ds3100.md/stor-layout.o ds3100.md/stupid.o ds3100.md/symout.o ds3100.md/tree.o ds3100.md/varasm.o ds3100.md/version.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= ds3100.md/insn-emit.o ds3100.md/insn-extract.o ds3100.md/insn-output.o ds3100.md/insn-peep.o ds3100.md/insn-recog.o
