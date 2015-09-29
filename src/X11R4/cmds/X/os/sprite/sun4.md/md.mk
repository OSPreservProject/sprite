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
# Fri Jan 24 19:59:09 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= WaitFor.c fonttype.c utils.c fontdir.c io.c oscolor.c osinit.c access.c genalloca.c osfonts.c pdev.c xdmcp.c connection.c auth.c mitauth.c
HDRS		= fonttype.h osdep.h
MDPUBHDRS	= 
OBJS		= sun4.md/WaitFor.o sun4.md/access.o sun4.md/auth.o sun4.md/connection.o sun4.md/fontdir.o sun4.md/fonttype.o sun4.md/genalloca.o sun4.md/io.o sun4.md/oscolor.o sun4.md/osfonts.o sun4.md/osinit.o sun4.md/pdev.o sun4.md/utils.o sun4.md/xdmcp.o sun4.md/mitauth.o
CLEANOBJS	= sun4.md/WaitFor.o sun4.md/fonttype.o sun4.md/utils.o sun4.md/fontdir.o sun4.md/io.o sun4.md/oscolor.o sun4.md/osinit.o sun4.md/access.o sun4.md/genalloca.o sun4.md/osfonts.o sun4.md/pdev.o sun4.md/xdmcp.o sun4.md/connection.o sun4.md/auth.o sun4.md/mitauth.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	=
