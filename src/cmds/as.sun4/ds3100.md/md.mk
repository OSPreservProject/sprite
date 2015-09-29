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
# Tue Dec 15 17:55:12 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= app.c append.c as.c atof-generic.c atof-m68k.c bignum-copy.c expr.c flonum-const.c flonum-copy.c flonum-mult.c frags.c gdb-blocks.c gdb-file.c gdb-lines.c gdb-symbols.c gdb.c hash.c hex-value.c input-file.c input-scrub.c messages.c obstack.c output-file.c read.c sparc.c strstr.c subsegs.c symbols.c version.c write.c xmalloc.c xrealloc.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/app.o ds3100.md/append.o ds3100.md/gdb-file.o ds3100.md/hex-value.o ds3100.md/output-file.o ds3100.md/strstr.o ds3100.md/xmalloc.o ds3100.md/xrealloc.o ds3100.md/as.o ds3100.md/atof-generic.o ds3100.md/atof-m68k.o ds3100.md/bignum-copy.o ds3100.md/expr.o ds3100.md/flonum-const.o ds3100.md/flonum-copy.o ds3100.md/flonum-mult.o ds3100.md/frags.o ds3100.md/gdb-blocks.o ds3100.md/gdb-lines.o ds3100.md/gdb-symbols.o ds3100.md/gdb.o ds3100.md/hash.o ds3100.md/input-file.o ds3100.md/input-scrub.o ds3100.md/messages.o ds3100.md/obstack.o ds3100.md/read.o ds3100.md/sparc.o ds3100.md/subsegs.o ds3100.md/symbols.o ds3100.md/version.o ds3100.md/write.o
CLEANOBJS	= ds3100.md/app.o ds3100.md/append.o ds3100.md/as.o ds3100.md/atof-generic.o ds3100.md/atof-m68k.o ds3100.md/bignum-copy.o ds3100.md/expr.o ds3100.md/flonum-const.o ds3100.md/flonum-copy.o ds3100.md/flonum-mult.o ds3100.md/frags.o ds3100.md/gdb-blocks.o ds3100.md/gdb-file.o ds3100.md/gdb-lines.o ds3100.md/gdb-symbols.o ds3100.md/gdb.o ds3100.md/hash.o ds3100.md/hex-value.o ds3100.md/input-file.o ds3100.md/input-scrub.o ds3100.md/messages.o ds3100.md/obstack.o ds3100.md/output-file.o ds3100.md/read.o ds3100.md/sparc.o ds3100.md/strstr.o ds3100.md/subsegs.o ds3100.md/symbols.o ds3100.md/version.o ds3100.md/write.o ds3100.md/xmalloc.o ds3100.md/xrealloc.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
