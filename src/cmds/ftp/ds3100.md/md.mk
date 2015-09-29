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
# Thu Dec 17 17:10:57 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cmds.c cmdtab.c domacro.c ftp.c glob.c main.c ruserpass.c
HDRS		= ftp_var.h
MDPUBHDRS	= 
OBJS		= ds3100.md/cmds.o ds3100.md/cmdtab.o ds3100.md/domacro.o ds3100.md/ftp.o ds3100.md/glob.o ds3100.md/main.o ds3100.md/ruserpass.o
CLEANOBJS	= ds3100.md/cmds.o ds3100.md/cmdtab.o ds3100.md/domacro.o ds3100.md/ftp.o ds3100.md/glob.o ds3100.md/main.o ds3100.md/ruserpass.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
