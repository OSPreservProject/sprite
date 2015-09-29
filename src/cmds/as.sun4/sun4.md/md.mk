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
# Tue Dec 15 17:55:37 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= app.c append.c as.c atof-generic.c atof-m68k.c bignum-copy.c expr.c flonum-const.c flonum-copy.c flonum-mult.c frags.c gdb-blocks.c gdb-file.c gdb-lines.c gdb-symbols.c gdb.c hash.c hex-value.c input-file.c input-scrub.c messages.c obstack.c output-file.c read.c sparc.c strstr.c subsegs.c symbols.c version.c write.c xmalloc.c xrealloc.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/app.o sun4.md/append.o sun4.md/atof-generic.o sun4.md/atof-m68k.o sun4.md/bignum-copy.o sun4.md/flonum-const.o sun4.md/flonum-copy.o sun4.md/flonum-mult.o sun4.md/gdb-blocks.o sun4.md/gdb-file.o sun4.md/gdb-lines.o sun4.md/gdb.o sun4.md/hash.o sun4.md/hex-value.o sun4.md/input-file.o sun4.md/input-scrub.o sun4.md/messages.o sun4.md/obstack.o sun4.md/output-file.o sun4.md/strstr.o sun4.md/xmalloc.o sun4.md/xrealloc.o sun4.md/as.o sun4.md/expr.o sun4.md/frags.o sun4.md/gdb-symbols.o sun4.md/read.o sun4.md/sparc.o sun4.md/subsegs.o sun4.md/symbols.o sun4.md/version.o sun4.md/write.o
CLEANOBJS	= sun4.md/app.o sun4.md/append.o sun4.md/as.o sun4.md/atof-generic.o sun4.md/atof-m68k.o sun4.md/bignum-copy.o sun4.md/expr.o sun4.md/flonum-const.o sun4.md/flonum-copy.o sun4.md/flonum-mult.o sun4.md/frags.o sun4.md/gdb-blocks.o sun4.md/gdb-file.o sun4.md/gdb-lines.o sun4.md/gdb-symbols.o sun4.md/gdb.o sun4.md/hash.o sun4.md/hex-value.o sun4.md/input-file.o sun4.md/input-scrub.o sun4.md/messages.o sun4.md/obstack.o sun4.md/output-file.o sun4.md/read.o sun4.md/sparc.o sun4.md/strstr.o sun4.md/subsegs.o sun4.md/symbols.o sun4.md/version.o sun4.md/write.o sun4.md/xmalloc.o sun4.md/xrealloc.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
