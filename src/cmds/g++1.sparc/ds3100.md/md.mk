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
# Thu Dec 17 17:14:48 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= case.c cplus-class.c cplus-cvt.c cplus-decl.c cplus-decl2.c cplus-dem.c cplus-except.c cplus-expr.c cplus-init.c cplus-lex.c cplus-method.c cplus-ptree.c cplus-search.c cplus-tree.c cplus-type2.c cplus-typeck.c dbxout.c expr.c toplev.c obstack.c rtl.c jump.c integrate.c lastfile.c print-tree.c stmt.c stor-layout.c symout.c tree.c caller-save.c combine.c cse.c emit-rtl.c explow.c expmed.c final.c flow.c fold-const.c global-alloc.c local-alloc.c loop.c optabs.c recog.c regclass.c reload.c reload1.c rtlanal.c sdbout.c stupid.c varasm.c version.c cplus-tab.c
HDRS		= assert.h aux-output.h basic-block.h c-tree.h conditions.h config.h cplus-decl.h cplus-parse.h cplus-tab.h cplus-tree.h expr.h flags.h gdbfiles.h getpagesize.h gvarargs.h hard-reg-set.h input.h insn-codes.h insn-config.h insn-flags.h limits.h obstack.h output.h real.h recog.h regs.h reload.h rtl.h stab.h stack.h stdarg.h stddef.h symseg.h tm.h tree.h typeclass.h
MDPUBHDRS	= 
OBJS		= ds3100.md/case.o ds3100.md/cplus-class.o ds3100.md/cplus-cvt.o ds3100.md/cplus-decl.o ds3100.md/cplus-decl2.o ds3100.md/cplus-dem.o ds3100.md/cplus-except.o ds3100.md/cplus-expr.o ds3100.md/cplus-init.o ds3100.md/cplus-lex.o ds3100.md/cplus-method.o ds3100.md/cplus-ptree.o ds3100.md/cplus-search.o ds3100.md/cplus-tree.o ds3100.md/cplus-type2.o ds3100.md/cplus-typeck.o ds3100.md/dbxout.o ds3100.md/expr.o ds3100.md/toplev.o ds3100.md/obstack.o ds3100.md/rtl.o ds3100.md/jump.o ds3100.md/integrate.o ds3100.md/lastfile.o ds3100.md/print-tree.o ds3100.md/stmt.o ds3100.md/stor-layout.o ds3100.md/symout.o ds3100.md/tree.o ds3100.md/caller-save.o ds3100.md/combine.o ds3100.md/cse.o ds3100.md/emit-rtl.o ds3100.md/explow.o ds3100.md/expmed.o ds3100.md/final.o ds3100.md/flow.o ds3100.md/fold-const.o ds3100.md/global-alloc.o ds3100.md/local-alloc.o ds3100.md/loop.o ds3100.md/optabs.o ds3100.md/recog.o ds3100.md/regclass.o ds3100.md/reload.o ds3100.md/reload1.o ds3100.md/rtlanal.o ds3100.md/sdbout.o ds3100.md/stupid.o ds3100.md/varasm.o ds3100.md/version.o ds3100.md/cplus-tab.o
CLEANOBJS	= ds3100.md/case.o ds3100.md/cplus-class.o ds3100.md/cplus-cvt.o ds3100.md/cplus-decl.o ds3100.md/cplus-decl2.o ds3100.md/cplus-dem.o ds3100.md/cplus-except.o ds3100.md/cplus-expr.o ds3100.md/cplus-init.o ds3100.md/cplus-lex.o ds3100.md/cplus-method.o ds3100.md/cplus-ptree.o ds3100.md/cplus-search.o ds3100.md/cplus-tree.o ds3100.md/cplus-type2.o ds3100.md/cplus-typeck.o ds3100.md/dbxout.o ds3100.md/expr.o ds3100.md/toplev.o ds3100.md/obstack.o ds3100.md/rtl.o ds3100.md/jump.o ds3100.md/integrate.o ds3100.md/lastfile.o ds3100.md/print-tree.o ds3100.md/stmt.o ds3100.md/stor-layout.o ds3100.md/symout.o ds3100.md/tree.o ds3100.md/caller-save.o ds3100.md/combine.o ds3100.md/cse.o ds3100.md/emit-rtl.o ds3100.md/explow.o ds3100.md/expmed.o ds3100.md/final.o ds3100.md/flow.o ds3100.md/fold-const.o ds3100.md/global-alloc.o ds3100.md/local-alloc.o ds3100.md/loop.o ds3100.md/optabs.o ds3100.md/recog.o ds3100.md/regclass.o ds3100.md/reload.o ds3100.md/reload1.o ds3100.md/rtlanal.o ds3100.md/sdbout.o ds3100.md/stupid.o ds3100.md/varasm.o ds3100.md/version.o ds3100.md/cplus-tab.o
INSTFILES	= ds3100.md/md.mk Makefile local.mk
SACREDOBJS	= 
