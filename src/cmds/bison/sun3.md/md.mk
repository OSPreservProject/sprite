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
# Thu Dec 17 16:46:22 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= LR0.c allocate.c closure.c conflicts.c derives.c files.c getargs.c gram.c lalr.c lex.c main.c nullable.c output.c print.c reader.c symtab.c warshall.c getopt.c
HDRS		= files.h gram.h lex.h machine.h new.h state.h symtab.h types.h
MDPUBHDRS	= 
OBJS		= sun3.md/LR0.o sun3.md/allocate.o sun3.md/closure.o sun3.md/conflicts.o sun3.md/derives.o sun3.md/files.o sun3.md/getargs.o sun3.md/gram.o sun3.md/lalr.o sun3.md/lex.o sun3.md/main.o sun3.md/nullable.o sun3.md/output.o sun3.md/print.o sun3.md/reader.o sun3.md/symtab.o sun3.md/warshall.o sun3.md/getopt.o
CLEANOBJS	= sun3.md/LR0.o sun3.md/allocate.o sun3.md/closure.o sun3.md/conflicts.o sun3.md/derives.o sun3.md/files.o sun3.md/getargs.o sun3.md/gram.o sun3.md/lalr.o sun3.md/lex.o sun3.md/main.o sun3.md/nullable.o sun3.md/output.o sun3.md/print.o sun3.md/reader.o sun3.md/symtab.o sun3.md/warshall.o sun3.md/getopt.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
