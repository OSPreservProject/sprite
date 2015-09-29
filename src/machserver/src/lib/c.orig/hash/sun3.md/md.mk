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
# Tue Oct 24 00:35:25 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= Hash.c HashChainSearch.c Hash_CreateEntry.c Hash_DeleteEntry.c Hash_DeleteTable.c Hash_EnumFirst.c Hash_EnumNext.c Hash_FindEntry.c Hash_InitTable.c Hash_PrintStats.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/Hash.o sun3.md/HashChainSearch.o sun3.md/Hash_CreateEntry.o sun3.md/Hash_DeleteEntry.o sun3.md/Hash_DeleteTable.o sun3.md/Hash_EnumFirst.o sun3.md/Hash_EnumNext.o sun3.md/Hash_FindEntry.o sun3.md/Hash_InitTable.o sun3.md/Hash_PrintStats.o
CLEANOBJS	= sun3.md/Hash.o sun3.md/HashChainSearch.o sun3.md/Hash_CreateEntry.o sun3.md/Hash_DeleteEntry.o sun3.md/Hash_DeleteTable.o sun3.md/Hash_EnumFirst.o sun3.md/Hash_EnumNext.o sun3.md/Hash_FindEntry.o sun3.md/Hash_InitTable.o sun3.md/Hash_PrintStats.o
DISTDIR        ?= @(DISTDIR)
