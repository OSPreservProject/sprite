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
# Fri Aug  2 17:43:01 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/dbgIP.c sun3.md/dbgMain.c sun3.md/dbgTrap.s
HDRS		= sun3.md/dbg.h sun3.md/dbgAsm.h sun3.md/dbgInt.h sun3.md/vmInt.h sun3.md/vmMachInt.h
MDPUBHDRS	= sun3.md/dbg.h sun3.md/dbgAsm.h
OBJS		= sun3.md/dbgIP.o sun3.md/dbgMain.o sun3.md/dbgTrap.o
CLEANOBJS	= sun3.md/dbgIP.o sun3.md/dbgMain.o sun3.md/dbgTrap.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
