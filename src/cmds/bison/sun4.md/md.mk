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
# Thu Dec 17 16:46:28 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= LR0.c allocate.c closure.c conflicts.c derives.c files.c getargs.c gram.c lalr.c lex.c main.c nullable.c output.c print.c reader.c symtab.c warshall.c getopt.c
HDRS		= files.h gram.h lex.h machine.h new.h state.h symtab.h types.h
MDPUBHDRS	= 
OBJS		= sun4.md/LR0.o sun4.md/allocate.o sun4.md/closure.o sun4.md/conflicts.o sun4.md/derives.o sun4.md/files.o sun4.md/getargs.o sun4.md/getopt.o sun4.md/gram.o sun4.md/lalr.o sun4.md/lex.o sun4.md/main.o sun4.md/nullable.o sun4.md/output.o sun4.md/print.o sun4.md/reader.o sun4.md/symtab.o sun4.md/warshall.o
CLEANOBJS	= sun4.md/LR0.o sun4.md/allocate.o sun4.md/closure.o sun4.md/conflicts.o sun4.md/derives.o sun4.md/files.o sun4.md/getargs.o sun4.md/gram.o sun4.md/lalr.o sun4.md/lex.o sun4.md/main.o sun4.md/nullable.o sun4.md/output.o sun4.md/print.o sun4.md/reader.o sun4.md/symtab.o sun4.md/warshall.o sun4.md/getopt.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
