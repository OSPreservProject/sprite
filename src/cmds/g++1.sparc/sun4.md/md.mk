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
# Thu Dec 17 17:15:19 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= case.c cplus-class.c cplus-cvt.c cplus-decl.c cplus-decl2.c cplus-dem.c cplus-except.c cplus-expr.c cplus-init.c cplus-lex.c cplus-method.c cplus-ptree.c cplus-search.c cplus-tree.c cplus-type2.c cplus-typeck.c dbxout.c expr.c toplev.c obstack.c rtl.c jump.c integrate.c lastfile.c print-tree.c stmt.c stor-layout.c symout.c tree.c caller-save.c combine.c cse.c emit-rtl.c explow.c expmed.c final.c flow.c fold-const.c global-alloc.c local-alloc.c loop.c optabs.c recog.c regclass.c reload.c reload1.c rtlanal.c sdbout.c stupid.c varasm.c version.c cplus-tab.c
HDRS		= assert.h aux-output.h basic-block.h c-tree.h conditions.h config.h cplus-decl.h cplus-parse.h cplus-tab.h cplus-tree.h expr.h flags.h gdbfiles.h getpagesize.h gvarargs.h hard-reg-set.h input.h insn-codes.h insn-config.h insn-flags.h limits.h obstack.h output.h real.h recog.h regs.h reload.h rtl.h stab.h stack.h stdarg.h stddef.h symseg.h tm.h tree.h typeclass.h
MDPUBHDRS	= 
OBJS		= sun4.md/caller-save.o sun4.md/case.o sun4.md/combine.o sun4.md/cplus-class.o sun4.md/cplus-cvt.o sun4.md/cplus-decl.o sun4.md/cplus-decl2.o sun4.md/cplus-dem.o sun4.md/cplus-except.o sun4.md/cplus-expr.o sun4.md/cplus-init.o sun4.md/cplus-lex.o sun4.md/cplus-method.o sun4.md/cplus-ptree.o sun4.md/cplus-search.o sun4.md/cplus-tab.o sun4.md/cplus-tree.o sun4.md/cplus-type2.o sun4.md/cplus-typeck.o sun4.md/cse.o sun4.md/dbxout.o sun4.md/emit-rtl.o sun4.md/explow.o sun4.md/expmed.o sun4.md/expr.o sun4.md/final.o sun4.md/flow.o sun4.md/fold-const.o sun4.md/global-alloc.o sun4.md/insn-emit.o sun4.md/insn-extract.o sun4.md/insn-output.o sun4.md/insn-peep.o sun4.md/insn-recog.o sun4.md/integrate.o sun4.md/jump.o sun4.md/lastfile.o sun4.md/local-alloc.o sun4.md/loop.o sun4.md/obstack.o sun4.md/optabs.o sun4.md/print-tree.o sun4.md/recog.o sun4.md/regclass.o sun4.md/reload.o sun4.md/reload1.o sun4.md/rtl.o sun4.md/rtlanal.o sun4.md/sdbout.o sun4.md/stmt.o sun4.md/stor-layout.o sun4.md/stupid.o sun4.md/symout.o sun4.md/toplev.o sun4.md/tree.o sun4.md/varasm.o sun4.md/version.o
CLEANOBJS	= sun4.md/case.o sun4.md/cplus-class.o sun4.md/cplus-cvt.o sun4.md/cplus-decl.o sun4.md/cplus-decl2.o sun4.md/cplus-dem.o sun4.md/cplus-except.o sun4.md/cplus-expr.o sun4.md/cplus-init.o sun4.md/cplus-lex.o sun4.md/cplus-method.o sun4.md/cplus-ptree.o sun4.md/cplus-search.o sun4.md/cplus-tree.o sun4.md/cplus-type2.o sun4.md/cplus-typeck.o sun4.md/dbxout.o sun4.md/expr.o sun4.md/toplev.o sun4.md/obstack.o sun4.md/rtl.o sun4.md/jump.o sun4.md/integrate.o sun4.md/lastfile.o sun4.md/print-tree.o sun4.md/stmt.o sun4.md/stor-layout.o sun4.md/symout.o sun4.md/tree.o sun4.md/caller-save.o sun4.md/combine.o sun4.md/cse.o sun4.md/emit-rtl.o sun4.md/explow.o sun4.md/expmed.o sun4.md/final.o sun4.md/flow.o sun4.md/fold-const.o sun4.md/global-alloc.o sun4.md/local-alloc.o sun4.md/loop.o sun4.md/optabs.o sun4.md/recog.o sun4.md/regclass.o sun4.md/reload.o sun4.md/reload1.o sun4.md/rtlanal.o sun4.md/sdbout.o sun4.md/stupid.o sun4.md/varasm.o sun4.md/version.o sun4.md/cplus-tab.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= sun4.md/insn-emit.o sun4.md/insn-extract.o sun4.md/insn-output.o sun4.md/insn-peep.o sun4.md/insn-recog.o
