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
# Thu Aug 23 02:11:45 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ccl.c dfa.c ecs.c gen.c main.c misc.c nfa.c parse.c parse.y scan.c sym.c tblcmp.c yylex.c
HDRS		= flexdef.h parse.h
MDPUBHDRS	= 
OBJS		= sun4.md/ccl.o sun4.md/dfa.o sun4.md/ecs.o sun4.md/gen.o sun4.md/main.o sun4.md/misc.o sun4.md/nfa.o sun4.md/parse.o sun4.md/parse.o sun4.md/scan.o sun4.md/sym.o sun4.md/tblcmp.o sun4.md/yylex.o
CLEANOBJS	= sun4.md/ccl.o sun4.md/dfa.o sun4.md/ecs.o sun4.md/gen.o sun4.md/main.o sun4.md/misc.o sun4.md/nfa.o sun4.md/parse.o sun4.md/parse.o sun4.md/scan.o sun4.md/sym.o sun4.md/tblcmp.o sun4.md/yylex.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
