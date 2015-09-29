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
# Mon Jun  8 14:28:35 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Hash.c Hash_DeleteEntry.c Hash_DeleteTable.c Hash_EnumFirst.c Hash_EnumNext.c Hash_FindEntry.c Hash_InitTable.c Hash_PrintStats.c HashChainSearch.c Hash_CreateEntry.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/Hash.o symm.md/Hash_DeleteEntry.o symm.md/Hash_DeleteTable.o symm.md/Hash_EnumFirst.o symm.md/Hash_EnumNext.o symm.md/Hash_FindEntry.o symm.md/Hash_InitTable.o symm.md/Hash_PrintStats.o symm.md/HashChainSearch.o symm.md/Hash_CreateEntry.o
CLEANOBJS	= symm.md/Hash.o symm.md/Hash_DeleteEntry.o symm.md/Hash_DeleteTable.o symm.md/Hash_EnumFirst.o symm.md/Hash_EnumNext.o symm.md/Hash_FindEntry.o symm.md/Hash_InitTable.o symm.md/Hash_PrintStats.o symm.md/HashChainSearch.o symm.md/Hash_CreateEntry.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile tags
SACREDOBJS	= 
