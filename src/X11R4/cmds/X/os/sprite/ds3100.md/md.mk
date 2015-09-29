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
# Thu Jan 23 17:46:03 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= WaitFor.c fonttype.c utils.c fontdir.c io.c oscolor.c osinit.c access.c genalloca.c osfonts.c pdev.c xdmcp.c connection.c auth.c mitauth.c
HDRS		= fonttype.h osdep.h
MDPUBHDRS	= 
OBJS		= ds3100.md/WaitFor.o ds3100.md/access.o ds3100.md/auth.o ds3100.md/connection.o ds3100.md/fontdir.o ds3100.md/fonttype.o ds3100.md/genalloca.o ds3100.md/io.o ds3100.md/oscolor.o ds3100.md/osfonts.o ds3100.md/osinit.o ds3100.md/pdev.o ds3100.md/utils.o ds3100.md/xdmcp.o ds3100.md/mitauth.o
CLEANOBJS	= ds3100.md/WaitFor.o ds3100.md/fonttype.o ds3100.md/utils.o ds3100.md/fontdir.o ds3100.md/io.o ds3100.md/oscolor.o ds3100.md/osinit.o ds3100.md/access.o ds3100.md/genalloca.o ds3100.md/osfonts.o ds3100.md/pdev.o ds3100.md/xdmcp.o ds3100.md/connection.o ds3100.md/auth.o ds3100.md/mitauth.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	=
