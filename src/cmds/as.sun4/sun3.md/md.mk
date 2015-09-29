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
# Tue Dec 15 17:55:27 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= app.c append.c as.c atof-generic.c atof-m68k.c bignum-copy.c expr.c flonum-const.c flonum-copy.c flonum-mult.c frags.c gdb-blocks.c gdb-file.c gdb-lines.c gdb-symbols.c gdb.c hash.c hex-value.c input-file.c input-scrub.c messages.c obstack.c output-file.c read.c sparc.c strstr.c subsegs.c symbols.c version.c write.c xmalloc.c xrealloc.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/app.o sun3.md/append.o sun3.md/as.o sun3.md/atof-generic.o sun3.md/atof-m68k.o sun3.md/bignum-copy.o sun3.md/expr.o sun3.md/flonum-const.o sun3.md/flonum-copy.o sun3.md/flonum-mult.o sun3.md/frags.o sun3.md/gdb-blocks.o sun3.md/gdb-file.o sun3.md/gdb-lines.o sun3.md/gdb-symbols.o sun3.md/gdb.o sun3.md/hash.o sun3.md/hex-value.o sun3.md/input-file.o sun3.md/input-scrub.o sun3.md/messages.o sun3.md/obstack.o sun3.md/output-file.o sun3.md/read.o sun3.md/sparc.o sun3.md/strstr.o sun3.md/subsegs.o sun3.md/symbols.o sun3.md/version.o sun3.md/write.o sun3.md/xmalloc.o sun3.md/xrealloc.o
CLEANOBJS	= sun3.md/app.o sun3.md/append.o sun3.md/as.o sun3.md/atof-generic.o sun3.md/atof-m68k.o sun3.md/bignum-copy.o sun3.md/expr.o sun3.md/flonum-const.o sun3.md/flonum-copy.o sun3.md/flonum-mult.o sun3.md/frags.o sun3.md/gdb-blocks.o sun3.md/gdb-file.o sun3.md/gdb-lines.o sun3.md/gdb-symbols.o sun3.md/gdb.o sun3.md/hash.o sun3.md/hex-value.o sun3.md/input-file.o sun3.md/input-scrub.o sun3.md/messages.o sun3.md/obstack.o sun3.md/output-file.o sun3.md/read.o sun3.md/sparc.o sun3.md/strstr.o sun3.md/subsegs.o sun3.md/symbols.o sun3.md/version.o sun3.md/write.o sun3.md/xmalloc.o sun3.md/xrealloc.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
