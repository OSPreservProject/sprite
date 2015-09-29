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
# Fri Jan 24 19:59:56 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= WaitFor.c fonttype.c utils.c fontdir.c io.c oscolor.c osinit.c access.c genalloca.c osfonts.c pdev.c xdmcp.c connection.c auth.c mitauth.c
HDRS		= fonttype.h osdep.h
MDPUBHDRS	= 
OBJS		= sun3.md/WaitFor.o sun3.md/access.o sun3.md/connection.o sun3.md/fontdir.o sun3.md/fonttype.o sun3.md/genalloca.o sun3.md/io.o sun3.md/oscolor.o sun3.md/osfonts.o sun3.md/osinit.o sun3.md/pdev.o sun3.md/utils.o sun3.md/xdmcp.o sun3.md/auth.o sun3.md/mitauth.o
CLEANOBJS	= sun3.md/WaitFor.o sun3.md/fonttype.o sun3.md/utils.o sun3.md/fontdir.o sun3.md/io.o sun3.md/oscolor.o sun3.md/osinit.o sun3.md/access.o sun3.md/genalloca.o sun3.md/osfonts.o sun3.md/pdev.o sun3.md/xdmcp.o sun3.md/connection.o sun3.md/auth.o sun3.md/mitauth.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	=
