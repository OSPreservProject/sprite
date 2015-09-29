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
# Mon Jun  8 14:26:37 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Gate_ByDesc.c Gate_ByInetAddr.c Gate_ByNetAddr.c Gate_End.c Gate_Next.c Gate_SetFile.c Gate_Start.c
HDRS		= gateInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Gate_ByDesc.o ds3100.md/Gate_ByInetAddr.o ds3100.md/Gate_ByNetAddr.o ds3100.md/Gate_End.o ds3100.md/Gate_Next.o ds3100.md/Gate_SetFile.o ds3100.md/Gate_Start.o
CLEANOBJS	= ds3100.md/Gate_ByDesc.o ds3100.md/Gate_ByInetAddr.o ds3100.md/Gate_ByNetAddr.o ds3100.md/Gate_End.o ds3100.md/Gate_Next.o ds3100.md/Gate_SetFile.o ds3100.md/Gate_Start.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
