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
# Mon Jun  8 14:28:57 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Host_Next.c Host_ByName.c Host_End.c Host_Start.c Host_ByID.c Host_ByInetAddr.c Host_ByNetAddr.c Host_SetFile.c Host_Stat.c
HDRS		= hostInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Host_ByID.o ds3100.md/Host_ByInetAddr.o ds3100.md/Host_ByName.o ds3100.md/Host_ByNetAddr.o ds3100.md/Host_End.o ds3100.md/Host_Next.o ds3100.md/Host_SetFile.o ds3100.md/Host_Start.o ds3100.md/Host_Stat.o
CLEANOBJS	= ds3100.md/Host_Next.o ds3100.md/Host_ByName.o ds3100.md/Host_End.o ds3100.md/Host_Start.o ds3100.md/Host_ByID.o ds3100.md/Host_ByInetAddr.o ds3100.md/Host_ByNetAddr.o ds3100.md/Host_SetFile.o ds3100.md/Host_Stat.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile tags
SACREDOBJS	= 
