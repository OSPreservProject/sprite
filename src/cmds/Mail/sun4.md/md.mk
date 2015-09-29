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
# Thu Dec 17 16:40:01 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c edit.c fio.c getname.c head.c lex.c list.c main.c names.c popen.c quit.c send.c strings.c temp.c tty.c vars.c v7.local.c version.c
HDRS		= def.h glob.h local.h rcv.h v7.local.h
MDPUBHDRS	= 
OBJS		= sun4.md/aux.o sun4.md/cmd1.o sun4.md/cmd2.o sun4.md/cmd3.o sun4.md/cmdtab.o sun4.md/collect.o sun4.md/edit.o sun4.md/fio.o sun4.md/getname.o sun4.md/head.o sun4.md/lex.o sun4.md/list.o sun4.md/main.o sun4.md/names.o sun4.md/popen.o sun4.md/quit.o sun4.md/send.o sun4.md/strings.o sun4.md/temp.o sun4.md/tty.o sun4.md/v7.local.o sun4.md/vars.o sun4.md/version.o
CLEANOBJS	= sun4.md/aux.o sun4.md/cmd1.o sun4.md/cmd2.o sun4.md/cmd3.o sun4.md/cmdtab.o sun4.md/collect.o sun4.md/edit.o sun4.md/fio.o sun4.md/getname.o sun4.md/head.o sun4.md/lex.o sun4.md/list.o sun4.md/main.o sun4.md/names.o sun4.md/popen.o sun4.md/quit.o sun4.md/send.o sun4.md/strings.o sun4.md/temp.o sun4.md/tty.o sun4.md/vars.o sun4.md/v7.local.o sun4.md/version.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
