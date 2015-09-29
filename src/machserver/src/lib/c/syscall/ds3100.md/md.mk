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
# Tue Dec 10 17:08:14 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Fs_ReadVector.c Fs_Truncate.c Fs_WriteVector.c Ioc_Reposition.c Ioc_Truncate.c Ioc_ClearBits.c Ioc_GetFlags.c Ioc_GetOwner.c Ioc_Lock.c Ioc_Map.c Ioc_NumReadable.c Ioc_SetBits.c Ioc_SetFlags.c Ioc_SetOwner.c Ioc_Unlock.c Ioc_WriteBack.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/Fs_ReadVector.o ds3100.md/Fs_Truncate.o ds3100.md/Fs_WriteVector.o ds3100.md/Ioc_Reposition.o ds3100.md/Ioc_Truncate.o ds3100.md/Ioc_ClearBits.o ds3100.md/Ioc_GetFlags.o ds3100.md/Ioc_GetOwner.o ds3100.md/Ioc_Lock.o ds3100.md/Ioc_Map.o ds3100.md/Ioc_NumReadable.o ds3100.md/Ioc_SetBits.o ds3100.md/Ioc_SetFlags.o ds3100.md/Ioc_SetOwner.o ds3100.md/Ioc_Unlock.o ds3100.md/Ioc_WriteBack.o
CLEANOBJS	= ds3100.md/Fs_ReadVector.o ds3100.md/Fs_Truncate.o ds3100.md/Fs_WriteVector.o ds3100.md/Ioc_Reposition.o ds3100.md/Ioc_Truncate.o ds3100.md/Ioc_ClearBits.o ds3100.md/Ioc_GetFlags.o ds3100.md/Ioc_GetOwner.o ds3100.md/Ioc_Lock.o ds3100.md/Ioc_Map.o ds3100.md/Ioc_NumReadable.o ds3100.md/Ioc_SetBits.o ds3100.md/Ioc_SetFlags.o ds3100.md/Ioc_SetOwner.o ds3100.md/Ioc_Unlock.o ds3100.md/Ioc_WriteBack.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
