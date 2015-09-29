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
# Wed Oct 25 18:12:59 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= buf.c color.c connect.c fileIO.c fonttype.c hosts.c notused.c osfonts.c osglobals.c osinit.c pdev.c scheduler.c tcp.c utilities.c
HDRS		= buf.h fonttype.h spriteos.h
MDPUBHDRS	= 
OBJS		= sun4.md/buf.o sun4.md/color.o sun4.md/connect.o sun4.md/fileIO.o sun4.md/fonttype.o sun4.md/hosts.o sun4.md/notused.o sun4.md/osfonts.o sun4.md/osglobals.o sun4.md/osinit.o sun4.md/pdev.o sun4.md/scheduler.o sun4.md/tcp.o sun4.md/utilities.o
CLEANOBJS	= sun4.md/buf.o sun4.md/color.o sun4.md/connect.o sun4.md/fileIO.o sun4.md/fonttype.o sun4.md/hosts.o sun4.md/notused.o sun4.md/osfonts.o sun4.md/osglobals.o sun4.md/osinit.o sun4.md/pdev.o sun4.md/scheduler.o sun4.md/tcp.o sun4.md/utilities.o
DISTDIR        ?= @(DISTDIR)
