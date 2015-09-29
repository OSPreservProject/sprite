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
# Thu Dec 17 17:11:02 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= cmds.c cmdtab.c domacro.c ftp.c glob.c main.c ruserpass.c
HDRS		= ftp_var.h
MDPUBHDRS	= 
OBJS		= sun3.md/cmds.o sun3.md/cmdtab.o sun3.md/domacro.o sun3.md/ftp.o sun3.md/glob.o sun3.md/main.o sun3.md/ruserpass.o
CLEANOBJS	= sun3.md/cmds.o sun3.md/cmdtab.o sun3.md/domacro.o sun3.md/ftp.o sun3.md/glob.o sun3.md/main.o sun3.md/ruserpass.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
