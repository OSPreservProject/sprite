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
# Thu Dec 17 17:06:29 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= scan.c ccl.c dfa.c ecs.c yylex.c gen.c main.c misc.c nfa.c parse.y parse.c sym.c tblcmp.c
HDRS		= flexdef.h parse.h
MDPUBHDRS	= 
OBJS		= sun3.md/scan.o sun3.md/ccl.o sun3.md/dfa.o sun3.md/ecs.o sun3.md/yylex.o sun3.md/gen.o sun3.md/main.o sun3.md/misc.o sun3.md/nfa.o sun3.md/parse.o sun3.md/parse.o sun3.md/sym.o sun3.md/tblcmp.o
CLEANOBJS	= sun3.md/scan.o sun3.md/ccl.o sun3.md/dfa.o sun3.md/ecs.o sun3.md/yylex.o sun3.md/gen.o sun3.md/main.o sun3.md/misc.o sun3.md/nfa.o sun3.md/parse.o sun3.md/parse.o sun3.md/sym.o sun3.md/tblcmp.o parse.c
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
