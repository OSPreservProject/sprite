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
# Fri Aug  2 17:42:56 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/dbgAsm.s ds5000.md/dbgDis.c ds5000.md/dbgIP.c ds5000.md/dbgMain.c ds5000.md/dbgMainDbx.c
HDRS		= ds5000.md/dbg.h ds5000.md/dbgDbxInt.h ds5000.md/dbgInt.h
MDPUBHDRS	= ds5000.md/dbg.h
OBJS		= ds5000.md/dbgAsm.o ds5000.md/dbgDis.o ds5000.md/dbgIP.o ds5000.md/dbgMain.o ds5000.md/dbgMainDbx.o
CLEANOBJS	= ds5000.md/dbgAsm.o ds5000.md/dbgDis.o ds5000.md/dbgIP.o ds5000.md/dbgMain.o ds5000.md/dbgMainDbx.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
