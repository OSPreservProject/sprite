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
# Tue Jul  2 13:49:41 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/memAsm.s memSubr.c memory.c
HDRS		= mem.h memInt.h
MDPUBHDRS	= 
OBJS		= ds5000.md/memAsm.o ds5000.md/memSubr.o ds5000.md/memory.o
CLEANOBJS	= ds5000.md/memAsm.o ds5000.md/memSubr.o ds5000.md/memory.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
