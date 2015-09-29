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
# Wed Oct 25 18:12:53 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= buf.c color.c connect.c fileIO.c fonttype.c hosts.c notused.c osfonts.c osglobals.c osinit.c pdev.c scheduler.c tcp.c utilities.c
HDRS		= buf.h fonttype.h spriteos.h
MDPUBHDRS	= 
OBJS		= sun3.md/buf.o sun3.md/color.o sun3.md/connect.o sun3.md/fileIO.o sun3.md/fonttype.o sun3.md/hosts.o sun3.md/notused.o sun3.md/osfonts.o sun3.md/osglobals.o sun3.md/osinit.o sun3.md/pdev.o sun3.md/scheduler.o sun3.md/tcp.o sun3.md/utilities.o
CLEANOBJS	= sun3.md/buf.o sun3.md/color.o sun3.md/connect.o sun3.md/fileIO.o sun3.md/fonttype.o sun3.md/hosts.o sun3.md/notused.o sun3.md/osfonts.o sun3.md/osglobals.o sun3.md/osinit.o sun3.md/pdev.o sun3.md/scheduler.o sun3.md/tcp.o sun3.md/utilities.o
DISTDIR        ?= @(DISTDIR)
