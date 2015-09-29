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
# Thu Dec 17 17:06:24 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= scan.c ccl.c dfa.c ecs.c yylex.c gen.c main.c misc.c nfa.c parse.y parse.c sym.c tblcmp.c
HDRS		= flexdef.h parse.h
MDPUBHDRS	= 
OBJS		= ds3100.md/ccl.o ds3100.md/dfa.o ds3100.md/ecs.o ds3100.md/gen.o ds3100.md/main.o ds3100.md/misc.o ds3100.md/nfa.o ds3100.md/parse.o ds3100.md/scan.o ds3100.md/sym.o ds3100.md/tblcmp.o ds3100.md/yylex.o
CLEANOBJS	= ds3100.md/scan.o ds3100.md/ccl.o ds3100.md/dfa.o ds3100.md/ecs.o ds3100.md/yylex.o ds3100.md/gen.o ds3100.md/main.o ds3100.md/misc.o ds3100.md/nfa.o ds3100.md/parse.o ds3100.md/parse.o ds3100.md/sym.o ds3100.md/tblcmp.o parse.c
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
