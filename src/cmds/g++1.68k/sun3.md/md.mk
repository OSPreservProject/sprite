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
# Thu Dec 17 17:11:55 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= case.c cplus-class.c cplus-cvt.c cplus-decl.c cplus-decl2.c cplus-dem.c cplus-except.c cplus-expr.c cplus-init.c cplus-lex.c cplus-method.c cplus-ptree.c cplus-search.c cplus-tree.c expr.c cplus-type2.c cplus-typeck.c dbxout.c toplev.c obstack.c rtl.c jump.c integrate.c lastfile.c print-tree.c stmt.c stor-layout.c symout.c tree.c caller-save.c combine.c cse.c emit-rtl.c explow.c expmed.c final.c flow.c fold-const.c global-alloc.c local-alloc.c loop.c optabs.c recog.c regclass.c reload.c reload1.c rtlanal.c sdbout.c stupid.c varasm.c version.c cplus-tab.c
HDRS		= assert.h aux-output.h basic-block.h c-tree.h conditions.h config.h cplus-decl.h cplus-parse.h cplus-tab.h cplus-tree.h expr.h flags.h gdbfiles.h getpagesize.h gvarargs.h hard-reg-set.h input.h insn-codes.h insn-config.h insn-flags.h limits.h obstack.h output.h real.h recog.h regs.h reload.h rtl.h stab.h stack.h stdarg.h stddef.h symseg.h tm.h tree.h typeclass.h
MDPUBHDRS	= 
OBJS		= sun3.md/case.o sun3.md/cplus-class.o sun3.md/cplus-cvt.o sun3.md/cplus-decl.o sun3.md/cplus-decl2.o sun3.md/cplus-dem.o sun3.md/cplus-except.o sun3.md/cplus-expr.o sun3.md/cplus-init.o sun3.md/cplus-lex.o sun3.md/cplus-method.o sun3.md/cplus-ptree.o sun3.md/cplus-search.o sun3.md/cplus-tree.o sun3.md/expr.o sun3.md/cplus-type2.o sun3.md/cplus-typeck.o sun3.md/dbxout.o sun3.md/toplev.o sun3.md/obstack.o sun3.md/rtl.o sun3.md/jump.o sun3.md/integrate.o sun3.md/lastfile.o sun3.md/print-tree.o sun3.md/stmt.o sun3.md/stor-layout.o sun3.md/symout.o sun3.md/tree.o sun3.md/caller-save.o sun3.md/combine.o sun3.md/cse.o sun3.md/emit-rtl.o sun3.md/explow.o sun3.md/expmed.o sun3.md/final.o sun3.md/flow.o sun3.md/fold-const.o sun3.md/global-alloc.o sun3.md/local-alloc.o sun3.md/loop.o sun3.md/optabs.o sun3.md/recog.o sun3.md/regclass.o sun3.md/reload.o sun3.md/reload1.o sun3.md/rtlanal.o sun3.md/sdbout.o sun3.md/stupid.o sun3.md/varasm.o sun3.md/version.o sun3.md/cplus-tab.o
CLEANOBJS	= sun3.md/case.o sun3.md/cplus-class.o sun3.md/cplus-cvt.o sun3.md/cplus-decl.o sun3.md/cplus-decl2.o sun3.md/cplus-dem.o sun3.md/cplus-except.o sun3.md/cplus-expr.o sun3.md/cplus-init.o sun3.md/cplus-lex.o sun3.md/cplus-method.o sun3.md/cplus-ptree.o sun3.md/cplus-search.o sun3.md/cplus-tree.o sun3.md/expr.o sun3.md/cplus-type2.o sun3.md/cplus-typeck.o sun3.md/dbxout.o sun3.md/toplev.o sun3.md/obstack.o sun3.md/rtl.o sun3.md/jump.o sun3.md/integrate.o sun3.md/lastfile.o sun3.md/print-tree.o sun3.md/stmt.o sun3.md/stor-layout.o sun3.md/symout.o sun3.md/tree.o sun3.md/caller-save.o sun3.md/combine.o sun3.md/cse.o sun3.md/emit-rtl.o sun3.md/explow.o sun3.md/expmed.o sun3.md/final.o sun3.md/flow.o sun3.md/fold-const.o sun3.md/global-alloc.o sun3.md/local-alloc.o sun3.md/loop.o sun3.md/optabs.o sun3.md/recog.o sun3.md/regclass.o sun3.md/reload.o sun3.md/reload1.o sun3.md/rtlanal.o sun3.md/sdbout.o sun3.md/stupid.o sun3.md/varasm.o sun3.md/version.o sun3.md/cplus-tab.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
