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
# Tue Oct 24 00:35:14 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= Hash.c HashChainSearch.c Hash_CreateEntry.c Hash_DeleteEntry.c Hash_DeleteTable.c Hash_EnumFirst.c Hash_EnumNext.c Hash_FindEntry.c Hash_InitTable.c Hash_PrintStats.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/Hash.o ds3100.md/HashChainSearch.o ds3100.md/Hash_CreateEntry.o ds3100.md/Hash_DeleteEntry.o ds3100.md/Hash_DeleteTable.o ds3100.md/Hash_EnumFirst.o ds3100.md/Hash_EnumNext.o ds3100.md/Hash_FindEntry.o ds3100.md/Hash_InitTable.o ds3100.md/Hash_PrintStats.o
CLEANOBJS	= ds3100.md/Hash.o ds3100.md/HashChainSearch.o ds3100.md/Hash_CreateEntry.o ds3100.md/Hash_DeleteEntry.o ds3100.md/Hash_DeleteTable.o ds3100.md/Hash_EnumFirst.o ds3100.md/Hash_EnumNext.o ds3100.md/Hash_FindEntry.o ds3100.md/Hash_InitTable.o ds3100.md/Hash_PrintStats.o
DISTDIR        ?= @(DISTDIR)
