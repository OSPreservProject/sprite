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
# Thu Dec 17 16:46:16 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= LR0.c allocate.c closure.c conflicts.c derives.c files.c getargs.c gram.c lalr.c lex.c main.c nullable.c output.c print.c reader.c symtab.c warshall.c getopt.c
HDRS		= files.h gram.h lex.h machine.h new.h state.h symtab.h types.h
MDPUBHDRS	= 
OBJS		= ds3100.md/LR0.o ds3100.md/allocate.o ds3100.md/closure.o ds3100.md/conflicts.o ds3100.md/derives.o ds3100.md/getargs.o ds3100.md/getopt.o ds3100.md/gram.o ds3100.md/lalr.o ds3100.md/lex.o ds3100.md/main.o ds3100.md/nullable.o ds3100.md/output.o ds3100.md/print.o ds3100.md/reader.o ds3100.md/symtab.o ds3100.md/warshall.o ds3100.md/files.o
CLEANOBJS	= ds3100.md/LR0.o ds3100.md/allocate.o ds3100.md/closure.o ds3100.md/conflicts.o ds3100.md/derives.o ds3100.md/files.o ds3100.md/getargs.o ds3100.md/gram.o ds3100.md/lalr.o ds3100.md/lex.o ds3100.md/main.o ds3100.md/nullable.o ds3100.md/output.o ds3100.md/print.o ds3100.md/reader.o ds3100.md/symtab.o ds3100.md/warshall.o ds3100.md/getopt.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
