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
# Fri Aug  2 17:42:51 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/dbgDis.c ds3100.md/dbgIP.c ds3100.md/dbgMain.c ds3100.md/dbgAsm.s ds3100.md/dbgMainDbx.c
HDRS		= ds3100.md/dbg.h ds3100.md/dbgDbxInt.h ds3100.md/dbgInt.h
MDPUBHDRS	= ds3100.md/dbg.h
OBJS		= ds3100.md/dbgAsm.o ds3100.md/dbgDis.o ds3100.md/dbgIP.o ds3100.md/dbgMain.o ds3100.md/dbgMainDbx.o
CLEANOBJS	= ds3100.md/dbgDis.o ds3100.md/dbgIP.o ds3100.md/dbgMain.o ds3100.md/dbgAsm.o ds3100.md/dbgMainDbx.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
